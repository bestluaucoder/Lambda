#include "Resolver.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include "../../SDK/Interfaces.h"
#include "../../SDK/Misc/CBasePlayer.h"
#include "LagCompensation.h"
#include "../../SDK/Globals.h"
#include "AnimationSystem.h"
#include "../../Utils/Console.h"

CResolver* Resolver = new CResolver;

static float ValveAngleDiff(float dest, float src)
{
	float d = fmodf(dest - src, 360.f);
	if (dest > src) { if (d >= 180.f)  d -= 360.f; }
	else            { if (d <= -180.f) d += 360.f; }
	return d;
}

static float CircularMean(const std::deque<LagRecord>& records, int count = 8)
{
	float sx = 0.f, cx = 0.f;
	int n = 0;
	for (int i = (int)records.size() - 2; i >= 0 && n < count; --i, ++n) {
		float y = records[i].m_angEyeAngles.yaw;
		sx += std::sinf(DEG2RAD(y));
		cx += std::cosf(DEG2RAD(y));
	}
	return RAD2DEG(std::atan2f(sx, cx));
}

float CResolver::GetTime()
{
	return Cheat.LocalPlayer ? TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase()) : GlobalVars->curtime;
}

float CResolver::GetLatency()
{
	INetChannelInfo* nci = EngineClient->GetNetChannelInfo();
	return nci ? nci->GetLatency(FLOW_OUTGOING) + nci->GetLatency(FLOW_INCOMING) : 0.f;
}

void CResolver::Reset(CBasePlayer* pl)
{
	if (pl) { resolver_data[pl->EntIndex()].reset(); return; }
	for (int i = 0; i < 64; ++i) resolver_data[i].reset();
}

R_PlayerState CResolver::DetectPlayerState(CBasePlayer* player, AnimationLayer* animlayers)
{
	if (!(player->m_fFlags() & FL_ONGROUND))
		return R_PlayerState::AIR;

	CCSGOPlayerAnimationState* as = player->GetAnimstate();
	if (player->m_vecVelocity().Length2DSqr() > 256.f
	    && as->flWalkToRunTransition > 0.8f
	    && animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate > 0.0001f)
		return R_PlayerState::MOVING;

	return R_PlayerState::STANDING;
}

R_AntiAimType CResolver::DetectAntiAim(CBasePlayer* player, const std::deque<LagRecord>& records)
{
	if (records.size() < 12) return R_AntiAimType::NONE;

	int jitter = 0, stat = 0;
	float total = 0.f, prev = player->m_angEyeAngles().yaw;
	float max_delta = 0.f;
	int limit = (int)records.size() - 2;
	int end   = (std::max)(limit - 10, -1);

	for (int i = limit; i > end; --i) {
		float d = std::abs(Math::AngleDiff(records[i].m_angEyeAngles.yaw, prev));
		total += d;
		if (d > max_delta) max_delta = d;
		d > 26.f ? ++jitter : ++stat;
		prev = records[i].m_angEyeAngles.yaw;
	}

	int samples = limit - end;
	float avg = samples > 0 ? total / samples : 0.f;

	if (jitter > stat && max_delta > 35.f) return R_AntiAimType::JITTER;
	if (avg < 18.f)                        return R_AntiAimType::STATIC;
	return R_AntiAimType::UNKNOWN;
}

void CResolver::UpdateLBY(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p)
{
	float lby    = player->m_flLowerBodyYawTarget();
	float eye    = record->m_angEyeAngles.yaw;
	float diff   = std::abs(Math::AngleDiff(lby, p->lby_value));

	if (diff > 1.5f) {
		p->lby_delta       = Math::AngleDiff(lby, eye);
		p->lby_last_update = record->m_flSimulationTime;
		p->lby_value       = lby;
	}
}

int CResolver::ResolveLBY(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p, float& out_body_yaw)
{
	if (record->resolver_data.player_state != R_PlayerState::STANDING)
		return 0;

	float latency = GetLatency();
	float lby     = player->m_flLowerBodyYawTarget();
	float eye     = record->m_angEyeAngles.yaw;
	float gap     = Math::AngleDiff(lby, eye);
	float since   = record->m_flSimulationTime - p->lby_last_update;
	float window  = TICKS_TO_TIME(8) + latency;

	if (since < window && std::abs(p->lby_delta) > 1.f) {
		out_body_yaw = -p->lby_delta;
		return p->lby_delta > 0.f ? 1 : -1;
	}

	if (std::abs(gap) > 15.f) {
		out_body_yaw = -gap;
		return gap < 0.f ? 1 : -1;
	}

	return 0;
}

void CResolver::UpdateJitterHistory(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p)
{
	p->eye_yaw_history[p->eye_yaw_head] = record->m_angEyeAngles.yaw;
	p->eye_yaw_head = (p->eye_yaw_head + 1) % ResolverDataStatic_t::JITTER_HISTORY;
	if (p->eye_yaw_count < ResolverDataStatic_t::JITTER_HISTORY) ++p->eye_yaw_count;
}

int CResolver::ResolveJitter(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p)
{
	if (record->resolver_data.antiaim_type != R_AntiAimType::JITTER || p->eye_yaw_count < 6)
		return 0;

	auto& recs = LagCompensation->records(player->EntIndex());
	float mean = CircularMean(recs, 12);
	float cur  = record->m_angEyeAngles.yaw;

	float sum_p = 0.f, sum_n = 0.f;
	float sq_p  = 0.f, sq_n  = 0.f;
	float max_p = 0.f, max_n = 0.f;
	int   cnt_p = 0,   cnt_n = 0;

	for (int i = 0; i < p->eye_yaw_count; ++i) {
		int   idx = (p->eye_yaw_head - 1 - i + ResolverDataStatic_t::JITTER_HISTORY) % ResolverDataStatic_t::JITTER_HISTORY;
		float d   = Math::AngleDiff(p->eye_yaw_history[idx], mean);
		if (d >= 0.f) {
			sum_p += d; sq_p += d * d;
			if (d > max_p) max_p = d;
			++cnt_p;
		} else {
			sum_n += d; sq_n += d * d;
			if (-d > max_n) max_n = -d;
			++cnt_n;
		}
	}

	if (!cnt_p || !cnt_n) return 0;

	float ap = sum_p / cnt_p;
	float an = sum_n / cnt_n;

	float var_p = (sq_p / cnt_p) - ap * ap;
	float var_n = (sq_n / cnt_n) - an * an;

	float amp_p = max_p;
	float amp_n = max_n;
	float amplitude = (amp_p + amp_n) * 0.5f;

	float var_thresh = amplitude * amplitude * 0.35f;
	if (var_p > var_thresh || var_n > var_thresh) return 0;

	float cur_d = Math::AngleDiff(cur, mean);

	float dp = std::abs(cur_d - ap);
	float dn = std::abs(cur_d - an);

	if (dp < dn) {
		p->jitter_last_cluster = 1;
		return 1;
	} else {
		p->jitter_last_cluster = -1;
		return -1;
	}
}

void CResolver::UpdateVelocity(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p)
{
	float speed = record->m_vecVelocity.Length2D();
	if (speed < 5.f) { p->prev_speed = speed; return; }

	float velYaw = RAD2DEG(std::atan2f(record->m_vecVelocity.y, record->m_vecVelocity.x));
	float relYaw = Math::AngleDiff(velYaw, record->m_angEyeAngles.yaw);
	float dSpeed = speed - p->prev_speed;

	if (std::abs(dSpeed) > 10.f)
		p->accel_side = relYaw > 0.f ? 1 : -1;

	p->prev_speed = speed;
}

void CResolver::UpdateMoveYaw(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p)
{
	CCSGOPlayerAnimationState* as = player->GetAnimstate();
	if (!as || !(record->m_fFlags & FL_ONGROUND) || record->m_vecVelocity.Length2DSqr() < 256.f)
		return;

	p->move_yaw_delta_sum += as->flMoveYaw;
	if (++p->move_yaw_samples > 12) {
		p->move_yaw_delta_sum -= as->flMoveYaw;
		p->move_yaw_samples    = 12;
	}

	float bias = p->move_yaw_delta_sum / p->move_yaw_samples;
	if      (bias >  8.f) p->move_yaw_side =  1;
	else if (bias < -8.f) p->move_yaw_side = -1;
	else                  p->move_yaw_side  =  0;
}

void CResolver::DetectTickbaseShift(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p)
{
	record->resolver_data.is_shifting_tickbase = false;
	record->resolver_data.detected_shift_ticks = 0;

	if (!record->prev_record) { p->last_sim_time = record->m_flSimulationTime; return; }

	const float ival  = GlobalVars->interval_per_tick;
	const float dt    = record->m_flSimulationTime - p->last_sim_time;
	int         extra = std::clamp(static_cast<int>(std::roundf(dt / ival)) - 1, 0, 16);

	if (extra >= 2) {
		record->resolver_data.is_shifting_tickbase = true;
		record->resolver_data.detected_shift_ticks = extra;
		if (p->shift_ticks_observed == 0) {
			p->shift_side_votes = 0;
			p->shift_vote_count = 0;
			p->shift_first_seen = record->m_flSimulationTime;
		}
		p->shift_ticks_observed = extra;
	} else {
		p->shift_ticks_observed = 0;
	}

	p->last_sim_time = record->m_flSimulationTime;
}

int CResolver::ResolveTickbase(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p)
{
	if (!record->resolver_data.is_shifting_tickbase) return 0;

	float latency = GetLatency();
	float lby     = player->m_flLowerBodyYawTarget();
	float eye     = record->m_angEyeAngles.yaw;
	float gap     = Math::AngleDiff(lby, eye);
	float thresh  = 18.f + TIME_TO_TICKS(latency) * 0.5f;

	if (std::abs(gap) > thresh) {
		p->shift_side_votes += gap < 0.f ? 1 : -1;
		++p->shift_vote_count;
	}

	{
		float best_d = FLT_MAX, best_desync = 0.f;
		for (auto& layer : record->resolver_data.layers) {
			if (layer.delta < best_d) { best_d = layer.delta; best_desync = layer.desync; }
		}
		if (best_desync != 0.f && best_d < record->resolver_data.max_desync_delta * 8.f) {
			p->shift_side_votes += best_desync < 0.f ? -1 : 1;
			++p->shift_vote_count;
		}
	}

	{
		float speed = record->m_vecVelocity.Length2D();
		if (speed > 10.f) {
			float relYaw = Math::AngleDiff(RAD2DEG(std::atan2f(record->m_vecVelocity.y, record->m_vecVelocity.x)), eye);
			p->shift_side_votes += relYaw > 0.f ? 1 : -1;
		}
	}

	if (p->shift_vote_count >= 2) {
		p->tickbase_side = p->shift_side_votes > 0 ? 1 : -1;
		return p->tickbase_side;
	}

	float staleness = TICKS_TO_TIME(32) + latency;
	if (p->tickbase_side != 0 && record->m_flSimulationTime - p->shift_first_seen < staleness)
		return p->tickbase_side;

	return 0;
}

void CResolver::SetupLayer(LagRecord* record, int idx, float delta)
{
	CCSGOPlayerAnimationState* as = record->player->GetAnimstate();
	QAngle angles = record->player->m_angEyeAngles();

	as->flFootYaw = Math::AngleNormalize(angles.yaw + delta);

	Vector vel = record->player->m_vecVelocity();
	float ideal = atan2f(-vel.y, -vel.x) * 180.f / M_PI;
	if (ideal < 0.f) ideal += 360.f;
	as->flMoveYaw = Math::AngleNormalize(ValveAngleDiff(ideal, as->flFootYaw));

	if (record->prev_record)
		memcpy(record->player->GetAnimlayers(), record->prev_record->animlayers, sizeof(AnimationLayer) * 13);

	as->ForceUpdate();
	as->Update(angles);

	auto& layer  = record->resolver_data.layers[idx];
	layer.desync = delta;

	float ts = 1.f;
	if (record->resolver_data.is_shifting_tickbase && record->resolver_data.detected_shift_ticks > 1)
		ts = 1.f / static_cast<float>(record->resolver_data.detected_shift_ticks);

	float dMove = std::abs(
		record->animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate
		- record->player->GetAnimlayers()[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate) * 1000.f * ts;

	float pitch_factor = 1.f - (std::min)(std::abs(record->player->m_angEyeAngles().pitch) / 89.f, 1.f);

	float dLean = std::abs(
		record->animlayers[ANIMATION_LAYER_LEAN].m_flWeight
		- record->player->GetAnimlayers()[ANIMATION_LAYER_LEAN].m_flWeight) * 600.f * ts * pitch_factor;

	float dLoop = std::abs(
		record->animlayers[ANIMATION_LAYER_ALIVELOOP].m_flCycle
		- record->player->GetAnimlayers()[ANIMATION_LAYER_ALIVELOOP].m_flCycle) * 400.f;

	float dLand = std::abs(
		record->animlayers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flWeight
		- record->player->GetAnimlayers()[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flWeight) * 300.f;

	layer.delta = dMove + dLean * 0.6f + dLoop * 0.3f + dLand * 0.1f;

	*as = *AnimationSystem->GetUnupdatedAnimstate(record->player->EntIndex());
}

void CResolver::SetupResolverLayers(CBasePlayer* player, LagRecord* record)
{
	float d = record->resolver_data.max_desync_delta;
	
	
	SetupLayer(record, 0,  0.f);
	SetupLayer(record, 1,  d);
	SetupLayer(record, 2, -d);
	SetupLayer(record, 3,  d * 0.5f);
	SetupLayer(record, 4, -d * 0.5f);
	SetupLayer(record, 5,  d * 0.75f);
	SetupLayer(record, 6, -d * 0.75f);
	SetupLayer(record, 7,  d * 0.33f);
	SetupLayer(record, 8,  d * 0.66f);
	SetupLayer(record, 9, -d * 0.66f);
}

int CResolver::ResolveAnim(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p)
{
	float mn = FLT_MAX, mx = 0.f;
	int   best_side = 0;

	for (auto& layer : record->resolver_data.layers) {
		if (layer.delta < mn) {
			mn = layer.delta;
			best_side = layer.desync < 0.f ? -1 : (layer.desync > 0.f ? 1 : 0);
		}
		if (layer.delta > mx) mx = layer.delta;
	}

	
	
	if ((mx - mn) < 0.5f)
		return 0;

	const float latency = GetLatency();

	float ts = 1.f;
	if (record->resolver_data.is_shifting_tickbase && record->resolver_data.detected_shift_ticks > 1)
		ts = 1.f / static_cast<float>(record->resolver_data.detected_shift_ticks);

	float spread_thresh;
	switch (record->resolver_data.player_state) {
	case R_PlayerState::MOVING:   spread_thresh = 10.f; break;
	case R_PlayerState::AIR:      spread_thresh = 18.f; break;
	default:                      spread_thresh =  6.f; break;
	}

	
	float abs_thresh = (6.f + TIME_TO_TICKS(latency) * 0.2f) * ts;

	if (mn > abs_thresh || (mx - mn) < spread_thresh)
		return 0;

	return best_side;
}

int CResolver::ResolveFreestand(CBasePlayer* player, LagRecord* record, const std::deque<LagRecord>& records)
{
	if (records.size() < 8) return 0;

	float eyeZ = player->m_vecOrigin().z + 64.f - player->m_flDuckAmount() * 16.f;
	Vector eye(player->m_vecOrigin().x, player->m_vecOrigin().y, eyeZ);

	float yaw = player->m_angEyeAngles().yaw;
	if (record->resolver_data.antiaim_type != R_AntiAimType::STATIC)
		yaw = CircularMean(records);

	Vector right  = Math::AngleVectors(QAngle(0.f, yaw + 90.f, 0));
	Vector fwd    = (Cheat.LocalPlayer->m_vecOrigin() - player->m_vecOrigin()).Normalized();
	float  scale  = 26.f;

	CTraceFilterWorldAndPropsOnly filter;
	CGameTrace trN, trP;
	Ray_t rN(eye - right * scale, eye - right * scale + fwd * 140.f);
	Ray_t rP(eye + right * scale, eye + right * scale + fwd * 140.f);

	EngineTrace->TraceRay(rN, MASK_SHOT_HULL | CONTENTS_GRATE, &filter, &trN);
	EngineTrace->TraceRay(rP, MASK_SHOT_HULL | CONTENTS_GRATE, &filter, &trP);

	if (trN.startsolid && trP.startsolid) return 0;
	if (trN.startsolid) return -1;
	if (trP.startsolid) return  1;
	if (trN.fraction == 1.f && trP.fraction == 1.f) return 0;

	return trN.fraction < trP.fraction ? -1 : 1;
}

int CResolver::ResolveSafeTick(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p)
{
	if (!record->prev_record)
		return 0;

	const float ival    = GlobalVars->interval_per_tick;
	const float latency = GetLatency();

	const int   prev_choked = record->prev_record->m_nChokedTicks;
	const int   cur_choked  = record->m_nChokedTicks;

	
	bool is_safe_tick = (prev_choked >= 2 && cur_choked <= 1) || 
	                     (prev_choked >= 1 && cur_choked == 0);

	if (!is_safe_tick) {
		
		float staleness = TICKS_TO_TIME(8) + latency * 0.5f;
		if (p->safe_tick_side != 0 && record->m_flSimulationTime - p->safe_tick_simtime < staleness)
			return p->safe_tick_side;
		return 0;
	}

	CCSGOPlayerAnimationState* as = player->GetAnimstate();
	if (!as) return 0;

	float foot_yaw = as->flFootYaw;
	float eye_yaw  = as->flEyeYaw;
	float delta    = Math::AngleDiff(foot_yaw, eye_yaw);

	if (std::abs(delta) < 4.f)
		return 0;

	int side = delta > 0.f ? 1 : -1;

	p->safe_tick_side    = side;
	p->safe_tick_simtime = record->m_flSimulationTime;

	return side;
}

void CResolver::Apply(LagRecord* record)
{
	if (record->resolver_data.side == 0) return;

	auto* as = record->player->GetAnimstate();

	if (std::abs(record->resolver_data.resolved_body_yaw) > 1.f)
		as->flFootYaw = Math::AngleNormalize(as->flEyeYaw + record->resolver_data.resolved_body_yaw);
	else
		as->flFootYaw = Math::AngleNormalize(as->flEyeYaw + record->resolver_data.max_desync_delta * record->resolver_data.side);
}

void CResolver::Run(CBasePlayer* player, LagRecord* record, std::deque<LagRecord>& records)
{
	if (GameRules()->IsFreezePeriod() || player->m_fFlags() & FL_FROZEN || !Cheat.LocalPlayer->IsAlive())
		return;

	record->resolver_data.max_desync_delta        = player->GetMaxDesyncDelta();
	record->resolver_data.resolved_body_yaw       = 0.f;
	record->resolver_data.side                    = 0;
	record->resolver_data.resolver_type           = ResolverType::NONE;

	if (record->shooting) {
		Apply(record);
		return;
	}

	if (!record->m_nChokedTicks || player->m_bIsDefusing())
		return;

	record->resolver_data.player_state = DetectPlayerState(player, record->animlayers);
	record->resolver_data.antiaim_type = DetectAntiAim(player, records);

	auto*       p       = &resolver_data[player->EntIndex()];
	const float curtime = GetTime();
	const float latency = GetLatency();

	UpdateLBY         (player, record, p);
	UpdateJitterHistory(player, record, p);
	UpdateVelocity    (player, record, p);
	UpdateMoveYaw     (player, record, p);
	DetectTickbaseShift(player, record, p);

	SetupResolverLayers(player, record);

	int   resolved_side     = 0;
	ResolverType resolved_type = ResolverType::NONE;
	float resolved_body_yaw = 0.f;

	auto commit = [&](int side, ResolverType type, float body_yaw = 0.f) {
		if (!side) return;
		resolved_side     = side;
		resolved_type     = type;
		resolved_body_yaw = body_yaw;
	};

	float lby_body_yaw = 0.f;
	int lby_side = ResolveLBY(player, record, p, lby_body_yaw);
	
	if (lby_side) commit(lby_side, ResolverType::LBY, lby_body_yaw);

	{
		int st_side = ResolveSafeTick(player, record, p);
		
		if (st_side && resolved_type != ResolverType::LBY) {
			commit(st_side, ResolverType::SAFETICK);
		}
	}

	if (record->resolver_data.is_shifting_tickbase) {
		int tb_side = ResolveTickbase(player, record, p);
		commit(tb_side, ResolverType::TICKBASE);
	}

	bool isMoving = record->resolver_data.player_state == R_PlayerState::MOVING
	             && (record->m_fFlags & FL_ONGROUND)
	             && record->animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flWeight > 0.f;

	if (!isMoving) {
		int fs_side = ResolveFreestand(player, record, records);
		if (fs_side) {
			if (resolved_type == ResolverType::NONE) {
				commit(fs_side, ResolverType::FREESTAND);
			} else if (resolved_type == ResolverType::LBY && fs_side != resolved_side) {
				commit(fs_side, ResolverType::FREESTAND);
			}
		}
	}

	int anim_side = ResolveAnim(player, record, p);
	if (anim_side && resolved_type == ResolverType::NONE)
		commit(anim_side, ResolverType::ANIM);

	if (isMoving) {
		if (p->move_yaw_side) commit(p->move_yaw_side, ResolverType::MOVEANGLE);
		if (p->accel_side)    commit(p->accel_side,    ResolverType::VELOCITY);
	} else if (!isMoving && resolved_type == ResolverType::NONE) {
		if (record->resolver_data.antiaim_type == R_AntiAimType::JITTER && records.size() > 8) {
			int jit = ResolveJitter(player, record, p);
			if (jit) {
				commit(jit, ResolverType::LOGIC);
			} else {
				float d = Math::AngleDiff(player->m_angEyeAngles().yaw, CircularMean(records, 8));
				commit(d < 0.f ? 1 : -1, ResolverType::LOGIC);
			}
		}
	}

	if (!resolved_side) {
		resolved_side = -1;
		resolved_type = ResolverType::DEFAULT;
	}

	record->resolver_data.side             = resolved_side;
	record->resolver_data.resolver_type    = resolved_type;
	record->resolver_data.resolved_body_yaw = resolved_body_yaw;

	Apply(record);
}

void CResolver::OnMiss(CBasePlayer* player, LagRecord* record)
{
	
}

void CResolver::OnHit(CBasePlayer* player, LagRecord* record)
{
	
}
