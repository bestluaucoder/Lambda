#include "Resolver.h"

#include <algorithm>
#include <cmath>
#include "../../SDK/Interfaces.h"
#include "../../SDK/Misc/CBasePlayer.h"
#include "LagCompensation.h"
#include "../../SDK/Globals.h"
#include "AnimationSystem.h"
#include "../../Utils/Console.h"

CResolver* Resolver = new CResolver;

static float ValveAngleDiff(float destAngle, float srcAngle)
{
	float delta = fmodf(destAngle - srcAngle, 360.f);
	if (destAngle > srcAngle) {
		if (delta >= 180.f)  delta -= 360.f;
	} else {
		if (delta <= -180.f) delta += 360.f;
	}
	return delta;
}

static float FindAvgYaw(const std::deque<LagRecord>& records, int count = 8)
{
	float sin_sum = 0.f, cos_sum = 0.f;
	int   samples = 0;

	for (int i = (int)records.size() - 2; i >= 0 && samples < count; --i, ++samples) {
		float y = records[i].m_angEyeAngles.yaw;
		sin_sum += std::sinf(DEG2RAD(y));
		cos_sum += std::cosf(DEG2RAD(y));
	}

	return RAD2DEG(std::atan2f(sin_sum, cos_sum));
}

float CResolver::GetTime()
{
	if (!Cheat.LocalPlayer)
		return GlobalVars->curtime;
	return TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase());
}

float CResolver::GetLatency()
{
	INetChannelInfo* nci = EngineClient->GetNetChannelInfo();
	if (!nci)
		return 0.f;
	return nci->GetLatency(FLOW_OUTGOING) + nci->GetLatency(FLOW_INCOMING);
}

void CResolver::Reset(CBasePlayer* pl)
{
	if (pl) {
		resolver_data[pl->EntIndex()].reset();
		return;
	}
	for (int i = 0; i < 64; ++i)
		resolver_data[i].reset();
}

R_PlayerState CResolver::DetectPlayerState(CBasePlayer* player, AnimationLayer* animlayers)
{
	if (!(player->m_fFlags() & FL_ONGROUND))
		return R_PlayerState::AIR;

	CCSGOPlayerAnimationState* animstate = player->GetAnimstate();

	if (player->m_vecVelocity().Length2DSqr() > 256.f
	    && animstate->flWalkToRunTransition > 0.8f
	    && animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate > 0.0001f)
		return R_PlayerState::MOVING;

	return R_PlayerState::STANDING;
}

R_AntiAimType CResolver::DetectAntiAim(CBasePlayer* player, const std::deque<LagRecord>& records)
{
	if (records.size() < 12)
		return R_AntiAimType::NONE;

	int   jittered = 0, staticr = 0;
	float avgDelta = 0.f;
	float prevYaw  = player->m_angEyeAngles().yaw;

	int limit = (int)records.size() - 2;
	int end   = (std::max)(limit - 8, -1);

	for (int i = limit; i > end; --i) {
		float yaw   = records[i].m_angEyeAngles.yaw;
		float delta = std::abs(Math::AngleDiff(yaw, prevYaw));
		avgDelta   += delta;
		if (delta > 32.f) ++jittered;
		else              ++staticr;
		prevYaw = yaw;
	}

	if (jittered > staticr)
		return R_AntiAimType::JITTER;

	if (avgDelta * 0.5f < 30.f)
		return R_AntiAimType::STATIC;

	return R_AntiAimType::UNKNOWN;
}

void CResolver::UpdateLBYPrediction(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* pdata)
{
	float lby    = player->m_flLowerBodyYawTarget();
	float eyeYaw = record->m_angEyeAngles.yaw;

	float lbyDiff = std::abs(Math::AngleDiff(lby, pdata->lby_value));
	if (lbyDiff > 2.f) {
		pdata->lby_delta       = Math::AngleDiff(lby, eyeYaw);
		pdata->lby_last_update = record->m_flSimulationTime;
		pdata->lby_value       = lby;
		pdata->lby_updated     = true;
	} else {
		pdata->lby_updated = false;
	}
}

int CResolver::PredictLBYSide(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* pdata)
{
	if (record->resolver_data.player_state != R_PlayerState::STANDING)
		return 0;

	float latency   = GetLatency();
	float lbyWindow = TICKS_TO_TIME(8) + latency;

	float lby    = player->m_flLowerBodyYawTarget();
	float eyeYaw = record->m_angEyeAngles.yaw;
	float delta  = Math::AngleDiff(lby, eyeYaw);

	float timeSinceUpdate = record->m_flSimulationTime - pdata->lby_last_update;
	if (timeSinceUpdate < lbyWindow) {
		if (pdata->lby_delta > 1.f)  return  1;
		if (pdata->lby_delta < -1.f) return -1;
	}

	if (std::abs(delta) > 26.f)
		return delta < 0.f ? 1 : -1;

	return 0;
}

void CResolver::UpdateJitterHistory(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* pdata)
{
	float eyeYaw = record->m_angEyeAngles.yaw;
	pdata->eye_yaw_history[pdata->eye_yaw_head] = eyeYaw;
	pdata->eye_yaw_head = (pdata->eye_yaw_head + 1) % ResolverDataStatic_t::JITTER_HISTORY;
	if (pdata->eye_yaw_count < ResolverDataStatic_t::JITTER_HISTORY)
		++pdata->eye_yaw_count;
}

int CResolver::PredictJitterSide(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* pdata)
{
	if (record->resolver_data.antiaim_type != R_AntiAimType::JITTER || pdata->eye_yaw_count < 4)
		return 0;

	float sum_pos = 0.f, sum_neg = 0.f;
	int   cnt_pos = 0,   cnt_neg = 0;

	float avgYaw = FindAvgYaw(LagCompensation->records(player->EntIndex()), 8);
	float curYaw = record->m_angEyeAngles.yaw;

	for (int i = 0; i < pdata->eye_yaw_count; ++i) {
		int   idx = (pdata->eye_yaw_head - 1 - i + ResolverDataStatic_t::JITTER_HISTORY) % ResolverDataStatic_t::JITTER_HISTORY;
		float d   = Math::AngleDiff(pdata->eye_yaw_history[idx], avgYaw);
		if (d >= 0.f) { sum_pos += d; ++cnt_pos; }
		else          { sum_neg += d; ++cnt_neg; }
	}

	if (!cnt_pos || !cnt_neg)
		return 0;

	float avg_pos   = sum_pos / cnt_pos;
	float avg_neg   = sum_neg / cnt_neg;
	float cur_delta = Math::AngleDiff(curYaw, avgYaw);

	return std::abs(cur_delta - avg_pos) < std::abs(cur_delta - avg_neg) ? 1 : -1;
}

void CResolver::UpdateVelocitySide(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* pdata)
{
	Vector vel   = record->m_vecVelocity;
	float  speed = vel.Length2D();

	if (speed < 5.f) {
		pdata->prev_speed = speed;
		return;
	}

	float velYaw = RAD2DEG(std::atan2f(vel.y, vel.x));
	float relYaw = Math::AngleDiff(velYaw, record->m_angEyeAngles.yaw);
	float dSpeed = speed - pdata->prev_speed;

	if (std::abs(dSpeed) > 10.f)
		pdata->accel_side = relYaw > 0.f ? 1 : -1;

	pdata->prev_speed = speed;
}

void CResolver::UpdateMoveYawSide(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* pdata)
{
	CCSGOPlayerAnimationState* animstate = player->GetAnimstate();
	if (!animstate)
		return;

	if (!(record->m_fFlags & FL_ONGROUND) || record->m_vecVelocity.Length2DSqr() < 256.f)
		return;

	float moveYaw = animstate->flMoveYaw;

	pdata->move_yaw_delta_sum += moveYaw;
	++pdata->move_yaw_samples;

	if (pdata->move_yaw_samples > 12) {
		pdata->move_yaw_delta_sum -= moveYaw;
		pdata->move_yaw_samples    = 12;
	}

	float bias = pdata->move_yaw_samples > 0 ? pdata->move_yaw_delta_sum / pdata->move_yaw_samples : 0.f;

	if      (bias >  8.f) pdata->move_yaw_side =  1;
	else if (bias < -8.f) pdata->move_yaw_side = -1;
	else                  pdata->move_yaw_side  =  0;
}

void CResolver::DetectTickbaseShift(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* pdata)
{
	record->resolver_data.is_shifting_tickbase = false;
	record->resolver_data.detected_shift_ticks = 0;

	if (!record->prev_record) {
		pdata->last_sim_time = record->m_flSimulationTime;
		return;
	}

	const float ival  = GlobalVars->interval_per_tick;
	const float dt    = record->m_flSimulationTime - pdata->last_sim_time;
	int         extra = std::clamp(static_cast<int>(std::roundf(dt / ival)) - 1, 0, 16);

	if (extra >= 2) {
		record->resolver_data.is_shifting_tickbase = true;
		record->resolver_data.detected_shift_ticks = extra;

		if (pdata->shift_ticks_observed == 0) {
			pdata->shift_side_votes = 0;
			pdata->shift_vote_count = 0;
			pdata->shift_first_seen = record->m_flSimulationTime;
		}
		pdata->shift_ticks_observed = extra;
	} else {
		if (pdata->shift_ticks_observed > 0)
			pdata->shift_ticks_observed = 0;
	}

	pdata->last_sim_time = record->m_flSimulationTime;
}

int CResolver::PredictTickbaseSide(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* pdata)
{
	if (!record->resolver_data.is_shifting_tickbase)
		return 0;

	float latency = GetLatency();

	{
		float lby    = player->m_flLowerBodyYawTarget();
		float eyeYaw = record->m_angEyeAngles.yaw;
		float delta  = Math::AngleDiff(lby, eyeYaw);

		float lbyThreshold = 20.f + TIME_TO_TICKS(latency) * 0.5f;

		if (std::abs(delta) > lbyThreshold) {
			pdata->shift_side_votes += delta < 0.f ? 1 : -1;
			++pdata->shift_vote_count;
		}
	}

	{
		float min_delta   = FLT_MAX;
		float best_desync = 0.f;

		for (auto& layer : record->resolver_data.layers) {
			if (layer.delta < min_delta) {
				min_delta    = layer.delta;
				best_desync  = layer.desync;
			}
		}

		if (best_desync != 0.f && min_delta < record->resolver_data.max_desync_delta * 8.f) {
			pdata->shift_side_votes += best_desync < 0.f ? -1 : 1;
			++pdata->shift_vote_count;
		}
	}

	{
		Vector vel   = record->m_vecVelocity;
		float  speed = vel.Length2D();

		if (speed > 10.f) {
			float velYaw = RAD2DEG(std::atan2f(vel.y, vel.x));
			float relYaw = Math::AngleDiff(velYaw, record->m_angEyeAngles.yaw);
			pdata->shift_side_votes += relYaw > 0.f ? 1 : -1;
		}
	}

	if (pdata->shift_vote_count >= 2) {
		int side = pdata->shift_side_votes > 0 ? 1 : -1;
		pdata->tickbase_side = side;
		return side;
	}

	float staleness = TICKS_TO_TIME(32) + latency;
	if (pdata->tickbase_side != 0 && record->m_flSimulationTime - pdata->shift_first_seen < staleness)
		return pdata->tickbase_side;

	return 0;
}

void CResolver::SetupLayer(LagRecord* record, int idx, float delta)
{
	CCSGOPlayerAnimationState* animstate = record->player->GetAnimstate();
	QAngle angles = record->player->m_angEyeAngles();

	animstate->flFootYaw = Math::AngleNormalize(angles.yaw + delta);

	Vector vel           = record->player->m_vecVelocity();
	float  flRawYawIdeal = atan2f(-vel.y, -vel.x) * 180.f / M_PI;
	if (flRawYawIdeal < 0.f) flRawYawIdeal += 360.f;

	animstate->flMoveYaw = Math::AngleNormalize(ValveAngleDiff(flRawYawIdeal, animstate->flFootYaw));

	if (record->prev_record)
		memcpy(record->player->GetAnimlayers(), record->prev_record->animlayers, sizeof(AnimationLayer) * 13);

	animstate->ForceUpdate();
	animstate->Update(angles);

	auto& layer  = record->resolver_data.layers[idx];
	layer.desync = delta;

	float tickScale = 1.f;
	if (record->resolver_data.is_shifting_tickbase && record->resolver_data.detected_shift_ticks > 1)
		tickScale = 1.f / static_cast<float>(record->resolver_data.detected_shift_ticks);

	float moveDelta = std::abs(
		record->animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate
		- record->player->GetAnimlayers()[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate) * 1000.f * tickScale;

	float leanDelta = std::abs(
		record->animlayers[ANIMATION_LAYER_LEAN].m_flWeight
		- record->player->GetAnimlayers()[ANIMATION_LAYER_LEAN].m_flWeight) * 500.f * tickScale;

	float aliveloopDelta = std::abs(
		record->animlayers[ANIMATION_LAYER_ALIVELOOP].m_flCycle
		- record->player->GetAnimlayers()[ANIMATION_LAYER_ALIVELOOP].m_flCycle) * 300.f * tickScale;

	layer.delta = moveDelta + leanDelta * 0.5f + aliveloopDelta * 0.25f;

	*animstate = *AnimationSystem->GetUnupdatedAnimstate(record->player->EntIndex());
}

void CResolver::SetupResolverLayers(CBasePlayer* player, LagRecord* record)
{
	float d = record->resolver_data.max_desync_delta;

	SetupLayer(record, 0,  0.f);
	SetupLayer(record, 1,  d);
	SetupLayer(record, 2, -d);
	SetupLayer(record, 3,  d * 0.5f);
	SetupLayer(record, 4, -d * 0.5f);
}

void CResolver::DetectFreestand(CBasePlayer* player, LagRecord* record, const std::deque<LagRecord>& records)
{
	if (records.size() < 16)
		return;

	Vector eyePos = player->m_vecOrigin() + Vector(0, 0, 64.f - player->m_flDuckAmount() * 16.f);

	float notModifiedYaw = player->m_angEyeAngles().yaw;
	if (record->resolver_data.antiaim_type != R_AntiAimType::STATIC)
		notModifiedYaw = FindAvgYaw(records);

	Vector right = Math::AngleVectors(QAngle(5.f, notModifiedYaw + 90.f, 0));
	Vector fwd   = (Cheat.LocalPlayer->m_vecOrigin() - player->m_vecOrigin()).Normalized();

	Vector negPos = eyePos - right * 23.f;
	Vector posPos = eyePos + right * 23.f;

	CTraceFilterWorldAndPropsOnly filter;
	Ray_t rayNeg(negPos, negPos + fwd * 128.f);
	Ray_t rayPos(posPos, posPos + fwd * 128.f);
	CGameTrace negTrace, posTrace;

	EngineTrace->TraceRay(rayNeg, MASK_SHOT_HULL | CONTENTS_GRATE, &filter, &negTrace);
	EngineTrace->TraceRay(rayPos, MASK_SHOT_HULL | CONTENTS_GRATE, &filter, &posTrace);

	if (negTrace.startsolid && posTrace.startsolid) {
		record->resolver_data.side         = 0;
		record->resolver_data.resolver_type = ResolverType::NONE;
		return;
	}
	if (negTrace.startsolid) {
		record->resolver_data.side         = -1;
		record->resolver_data.resolver_type = ResolverType::FREESTAND;
		return;
	}
	if (posTrace.startsolid) {
		record->resolver_data.side         =  1;
		record->resolver_data.resolver_type = ResolverType::FREESTAND;
		return;
	}
	if (negTrace.fraction == 1.f && posTrace.fraction == 1.f) {
		record->resolver_data.side         = 0;
		record->resolver_data.resolver_type = ResolverType::NONE;
		return;
	}

	record->resolver_data.side         = negTrace.fraction < posTrace.fraction ? -1 : 1;
	record->resolver_data.resolver_type = ResolverType::FREESTAND;
}

void CResolver::Apply(LagRecord* record)
{
	if (record->resolver_data.side == 0)
		return;

	auto* state      = record->player->GetAnimstate();
	state->flFootYaw = Math::AngleNormalize(state->flEyeYaw + record->resolver_data.max_desync_delta * record->resolver_data.side);
}

void CResolver::Run(CBasePlayer* player, LagRecord* record, std::deque<LagRecord>& records)
{
	if (GameRules()->IsFreezePeriod() || player->m_fFlags() & FL_FROZEN || !Cheat.LocalPlayer->IsAlive())
		return;

	record->resolver_data.max_desync_delta = player->GetMaxDesyncDelta();

	// when the player is shooting their desync resets — resolve to 0 so the
	// animlayer scoring gets a clean baseline rather than a stale side
	if (record->shooting) {
		record->resolver_data.side         = 0;
		record->resolver_data.resolver_type = ResolverType::NONE;
		Apply(record);
		return;
	}

	if (!record->m_nChokedTicks || player->m_bIsDefusing()) {
		record->resolver_data.side         = 0;
		record->resolver_data.resolver_type = ResolverType::NONE;
		return;
	}

	record->resolver_data.player_state = DetectPlayerState(player, record->animlayers);
	record->resolver_data.antiaim_type = DetectAntiAim(player, records);

	auto*       pdata   = &resolver_data[player->EntIndex()];
	const float curtime = GetTime();
	const float latency = GetLatency();

	UpdateLBYPrediction(player, record, pdata);
	UpdateJitterHistory(player, record, pdata);
	UpdateVelocitySide (player, record, pdata);
	UpdateMoveYawSide  (player, record, pdata);
	DetectTickbaseShift(player, record, pdata);

	SetupResolverLayers(player, record);

	record->resolver_data.resolver_type = ResolverType::NONE;
	record->resolver_data.side          = 0;

	float min_delta = FLT_MAX;
	for (auto& layer : record->resolver_data.layers) {
		if (layer.delta < min_delta) {
			min_delta = layer.delta;
			if (layer.desync != 0.f) {
				record->resolver_data.side         = layer.desync < 0.f ? -1 : 1;
				record->resolver_data.resolver_type = ResolverType::ANIM;
			}
		}
	}

	float animThreshold = 10.f + TIME_TO_TICKS(latency) * 0.3f;
	if (min_delta > animThreshold)
		record->resolver_data.resolver_type = ResolverType::NONE;

	// LBY — standing only, most reliable
	{
		int lbySide = PredictLBYSide(player, record, pdata);
		if (lbySide != 0) {
			record->resolver_data.side         = lbySide;
			record->resolver_data.resolver_type = ResolverType::LBY;
		}
	}

	// tickbase — overrides everything when active
	if (record->resolver_data.is_shifting_tickbase) {
		int tbSide = PredictTickbaseSide(player, record, pdata);
		if (tbSide != 0) {
			record->resolver_data.side         = tbSide;
			record->resolver_data.resolver_type = ResolverType::TICKBASE;
		}
	}

	// apply miss-based side flip — after enough misses, invert
	// only applies when we don't have a high-confidence source
	if (pdata->missed_shots >= 2
	    && record->resolver_data.resolver_type != ResolverType::LBY
	    && record->resolver_data.resolver_type != ResolverType::TICKBASE
	    && record->resolver_data.resolver_type != ResolverType::FREESTAND)
	{
		if (pdata->missed_shots % 2 == 0 && pdata->last_side != 0)
			record->resolver_data.side = -pdata->last_side;
	}

	bool isMoving = record->resolver_data.player_state == R_PlayerState::MOVING
	             && (record->m_fFlags & FL_ONGROUND)
	             && record->animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flWeight > 0.f;

	if (isMoving) {
		if (pdata->move_yaw_side != 0) {
			record->resolver_data.side         = pdata->move_yaw_side;
			record->resolver_data.resolver_type = ResolverType::MOVEANGLE;
		}
		if (pdata->accel_side != 0) {
			record->resolver_data.side         = pdata->accel_side;
			record->resolver_data.resolver_type = ResolverType::VELOCITY;
		}
	}

	// standing/air path — static AA always goes to freestand, jitter to logic
	if (!isMoving || record->resolver_data.resolver_type == ResolverType::NONE || !(record->m_fFlags & FL_ONGROUND)) {
		if (record->resolver_data.antiaim_type == R_AntiAimType::JITTER && records.size() > 8) {
			int jitterSide = PredictJitterSide(player, record, pdata);
			if (jitterSide != 0) {
				record->resolver_data.side         = jitterSide;
				record->resolver_data.resolver_type = ResolverType::LOGIC;
			} else {
				float eyeYaw = player->m_angEyeAngles().yaw;
				float avgYaw = FindAvgYaw(records, 8);
				float d      = Math::AngleDiff(eyeYaw, avgYaw);
				record->resolver_data.side         = d < 0.f ? 1 : -1;
				record->resolver_data.resolver_type = ResolverType::LOGIC;
			}
		} else {
			// static / unknown — freestand is more reliable than logic here
			if (record->resolver_data.resolver_type == ResolverType::NONE
			    || record->resolver_data.resolver_type == ResolverType::ANIM)
				DetectFreestand(player, record, records);
		}
	}

	if (record->resolver_data.resolver_type != ResolverType::NONE) {
		pdata->last_resolved = curtime;
		pdata->last_side     = record->resolver_data.side;
		pdata->res_type_last = record->resolver_data.resolver_type;
	} else {
		record->resolver_data.resolver_type = ResolverType::MEMORY;
		record->resolver_data.side          = pdata->last_side;
	}

	if (record->resolver_data.side == 0) {
		record->resolver_data.side         = -1;
		record->resolver_data.resolver_type = ResolverType::DEFAULT;
	}

	Apply(record);
}

void CResolver::OnMiss(CBasePlayer* player, LagRecord* record)
{
	auto* pdata = &resolver_data[player->EntIndex()];

	if (record->resolver_data.is_shifting_tickbase) {
		pdata->shift_side_votes     = 0;
		pdata->shift_vote_count     = 0;
		pdata->tickbase_side        = 0;
		pdata->shift_ticks_observed = 0;
	} else {
		pdata->accel_side         = 0;
		pdata->move_yaw_delta_sum = 0.f;
		pdata->move_yaw_samples   = 0;
		pdata->move_yaw_side      = 0;
	}

	++pdata->missed_shots;
}

void CResolver::OnHit(CBasePlayer* player, LagRecord* record)
{
	auto* pdata         = &resolver_data[player->EntIndex()];
	pdata->missed_shots  = 0;
	pdata->accel_side    = record->resolver_data.side;
}
