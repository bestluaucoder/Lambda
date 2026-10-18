#include "LagCompensation.h"
#include "AnimationSystem.h"
#include "../Misc/Prediction.h"
#include "../../SDK/Interfaces.h"
#include "../../SDK/Globals.h"
#include "../AntiAim/AntiAim.h"
#include "../../Utils/Utils.h"
#include <algorithm>
#include "../../SDK/NetMessages.h"
#include "../Visuals/ESP.h"
#include "Exploits.h"

LagRecord* CLagCompensation::BackupData(CBasePlayer* player) {
	if (!player)
		return nullptr;

	int idx = player->EntIndex();
	if (idx < 0 || idx >= 65)
		return nullptr;

	LagRecord* record = &backup_records[idx];
	record->player = player;
	record->m_vecAbsOrigin = player->GetAbsOrigin();
	RecordDataIntoTrack(player, record);
	memcpy(record->aim_matrix, record->bone_matrix, sizeof(matrix3x4_t) * 128);
	memcpy(record->clamped_matrix, record->bone_matrix, sizeof(matrix3x4_t) * 128);

	return record;
}

void CLagCompensation::RecordDataIntoTrack(CBasePlayer* player, LagRecord* record) {
	record->player = player;

	record->m_angEyeAngles = player->m_angEyeAngles();
	record->m_flSimulationTime = player->m_flSimulationTime();
	record->m_vecOrigin = player->m_vecOrigin();
	record->m_fFlags = player->m_fFlags();
	record->m_flCycle = player->m_flCycle();
	record->m_nSequence = player->m_nSequence();
	record->m_flDuckAmout = player->m_flDuckAmount();
	record->m_flDuckSpeed = player->m_flDuckSpeed();
	record->m_vecMaxs = player->m_vecMaxs();
	record->m_vecMins = player->m_vecMins();
	record->m_vecVelocity = player->m_vecVelocity();
	record->m_vecAbsAngles = player->GetAbsAngles();

	if (!record->bone_matrix_filled) {
		memcpy(record->bone_matrix, player->GetCachedBoneData().Base(), sizeof(matrix3x4_t) * player->GetCachedBoneData().Count());
		memcpy(record->aim_matrix, record->bone_matrix, sizeof(matrix3x4_t) * 128);
		record->bone_matrix_filled = true;
	}

	memcpy(record->animlayers, player->GetAnimlayers(), sizeof(AnimationLayer) * 13);
}

void CLagCompensation::BacktrackEntity(LagRecord* record, bool copy_matrix, bool use_aim_matrix) {
	CBasePlayer* player = record->player;

	
	player->m_vecOrigin() = record->m_vecOrigin;
	player->SetAbsOrigin(record->m_vecAbsOrigin);
	player->m_fFlags() = record->m_fFlags;
	player->m_flCycle() = record->m_flCycle;
	player->m_nSequence() = record->m_nSequence;
	player->m_flDuckAmount() = record->m_flDuckAmout;
	player->m_flDuckSpeed() = record->m_flDuckSpeed;
	player->m_vecVelocity() = record->m_vecVelocity;
	player->SetAbsAngles(record->m_vecAbsAngles);
	player->ForceBoneCache();

	player->SetCollisionBounds(record->m_vecMins, record->m_vecMaxs);

	if (copy_matrix) {
		if (use_aim_matrix) {
			memcpy(player->GetCachedBoneData().Base(), record->clamped_matrix, player->GetCachedBoneData().Count() * sizeof(matrix3x4_t));
		}
		else {
			memcpy(player->GetCachedBoneData().Base(), record->bone_matrix, player->GetCachedBoneData().Count() * sizeof(matrix3x4_t));
		}
	}
}

void LagRecord::BuildMatrix() {
	if (!player)
		return;

	bool aim_valid = false;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 4; j++) {
			if (aim_matrix[0][i][j] != 0.f) {
				aim_valid = true;
				break;
			}
		}
		if (aim_valid) break;
	}

	if (!aim_valid)
		memcpy(aim_matrix, bone_matrix, 128 * sizeof(matrix3x4_t));

	memcpy(clamped_matrix, aim_matrix, 128 * sizeof(matrix3x4_t));

	bool opp_valid = false;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 4; j++) {
			if (opposite_matrix[0][i][j] != 0.f) {
				opp_valid = true;
				break;
			}
		}
		if (opp_valid) break;
	}

	if (!opp_valid)
		memcpy(opposite_matrix, aim_matrix, 128 * sizeof(matrix3x4_t));

	memcpy(safe_matrix, opposite_matrix, 128 * sizeof(matrix3x4_t));

	if (config.antiaim.angles.legacy_desync->get())
		return;

	auto backup_eye_angle = player->m_angEyeAngles();

	player->ClampBonesInBBox(safe_matrix, BONE_USED_BY_HITBOX);

	if (config.ragebot.aimbot.roll_resolver->get())
		player->m_angEyeAngles().roll = config.ragebot.aimbot.roll_angle->get() * (resolver_data.side != 0 ? resolver_data.side : 1);
	
	player->ClampBonesInBBox(clamped_matrix, BONE_USED_BY_ANYTHING);

	player->m_angEyeAngles() = backup_eye_angle;
}

void CLagCompensation::OnNetUpdate() {
	if (!Cheat.InGame)
		return;

	INetChannel* nc = ClientState->m_NetChannel;
	auto nci = EngineClient->GetNetChannelInfo();

	for (int i = 1; i <= ClientState->m_nMaxClients && i <= 64; i++) {
		CBasePlayer* pl = (CBasePlayer*)EntityList->GetClientEntity(i);

		if (!pl || !pl->IsAlive() || pl == Cheat.LocalPlayer || pl->m_bDormant() || pl->m_iHealth() <= 0 || pl->m_lifeState() != LIFE_ALIVE)
			continue;

		auto& records = lag_records[i];

		if (!records.empty() && pl->m_flSimulationTime() == pl->m_flOldSimulationTime())
			continue;

		LagRecord* prev_record = !records.empty() ? &records.back() : nullptr;

		if (prev_record && prev_record->player != pl) {
			records.clear();
			prev_record = nullptr;
			max_tickbase[i] = 0;
			is_in_defensive[i] = false;
		}

		if (prev_record && prev_record->animlayers[ANIMATION_LAYER_ALIVELOOP].m_flCycle == pl->GetAnimlayers()[ANIMATION_LAYER_ALIVELOOP].m_flCycle) {
			pl->m_flOldSimulationTime() = pl->m_flSimulationTime();
			continue;
		}

		int current_tickbase = TIME_TO_TICKS(pl->m_flSimulationTime());
		
		if (std::abs(current_tickbase - max_tickbase[i]) > 64) {
			max_tickbase[i] = 0;
			is_in_defensive[i] = false;
		}

		if (current_tickbase > max_tickbase[i]) {
			max_tickbase[i] = current_tickbase;
			is_in_defensive[i] = false;
		}
		else if (max_tickbase[i] > current_tickbase) {
			int defensive_ticks = (std::min)(14, (std::max)(0, max_tickbase[i] - current_tickbase - 1));
			is_in_defensive[i] = defensive_ticks > 0;
		}

		LagRecord* new_record = &records.emplace_back();

		new_record->prev_record = prev_record;
		new_record->update_tick = GlobalVars->tickcount;			
		new_record->m_flSimulationTime = pl->m_flSimulationTime();
		new_record->m_flServerTime = EngineClient->GetLastTimeStamp();

		new_record->shifting_tickbase = max_simulation_time[i] >= new_record->m_flSimulationTime;

		if (new_record->m_flSimulationTime > max_simulation_time[i] || std::abs(max_simulation_time[i] - new_record->m_flSimulationTime) > 3.f)
			max_simulation_time[i] = new_record->m_flSimulationTime;

		last_update_tick[i] = GlobalVars->tickcount;

		AnimationSystem->UpdateAnimations(pl, new_record, records);
		RecordDataIntoTrack(pl, new_record);

        LagRecord* prev_valid = nullptr;
        for (int j = records.size() - 2; j > 0; j--) {
            auto r = &records[j];
            if (r->shifting_tickbase)
                continue;
            prev_valid = r;
            break;
        }

		if (prev_valid) {
			static constexpr float teleport_dist_sq = 64.f * 64.f;

			float dist_2d_sq = (prev_valid->m_vecOrigin - new_record->m_vecOrigin).Length2DSqr();

			bool force_break = false;
			if (prev_valid->shifting_tickbase && new_record->shifting_tickbase) {
				if (!(prev_valid->m_fFlags & FL_ONGROUND) || !(new_record->m_fFlags & FL_ONGROUND))
					force_break = true;
			}

			new_record->breaking_lag_comp = dist_2d_sq > teleport_dist_sq;

			if (force_break) {
				float vel_sq      = new_record->m_vecVelocity.LengthSqr();
				float prev_vel_sq = prev_valid->m_vecVelocity.LengthSqr();
				if (dist_2d_sq > 3000.f || vel_sq > 62500.f || prev_vel_sq > 62500.f)
					new_record->breaking_lag_comp = true;
			}
		}

		if (config.visuals.esp.shared_esp->get() && !EngineClient->IsVoiceRecording() && nc) {
			if (config.visuals.esp.share_with_enemies->get() || !pl->IsTeammate()) {
				SharedESP_t msg;

				player_info_t pinfo;
				EngineClient->GetPlayerInfo(i, &pinfo);

				msg.m_iPlayer = pinfo.userId;
				msg.m_ActiveWeapon = pl->GetActiveWeapon() ? pl->GetActiveWeapon()->m_iItemDefinitionIndex() : 0;
				msg.m_iHealth = pl->m_iHealth();
				msg.m_vecOrigin = new_record->m_vecOrigin;

				NetMessages->SendNetMessage((SharedVoiceData_t*)&msg);
			}
		}

		while (records.size() > (pl->IsTeammate() ? 4 : TIME_TO_TICKS(cvars.sv_maxunlag->GetFloat()) + 3)) 
			records.pop_front();

		
		if (!records.empty())
			records.front().prev_record = nullptr;

		if (config.visuals.esp.show_server_hitboxes->get() && nci && nci->IsLoopback())
			pl->DrawServerHitboxes(GlobalVars->interval_per_tick, true);
	}
}

LagRecord* CLagCompensation::ExtrapolateRecord(LagRecord* record, int ticks) {
	if (ticks <= 0 || !record || !record->player)
		return nullptr;

	auto& records = extrapolated_records[record->player->EntIndex()];

	while (records.size() > 4)
		records.pop_front();

	LagRecord* new_record = &records.emplace_back();
	*new_record = *record;
	new_record->m_flSimulationTime += TICKS_TO_TIME(ticks);

	const float gravity  = cvars.sv_gravity ? cvars.sv_gravity->GetFloat() : 800.f;
	const float ival     = GlobalVars->interval_per_tick;

	Vector origin   = record->m_vecOrigin;
	Vector velocity = record->m_vecVelocity;
	int    flags    = record->m_fFlags;
	bool   was_on_ground = (flags & FL_ONGROUND) != 0;

	const Vector mins = record->player->GetCollideable()->OBBMins();
	const Vector maxs = record->player->GetCollideable()->OBBMaxs();

	for (int i = 0; i < ticks; i++) {
		if (!(flags & FL_ONGROUND))
			velocity.z -= ival * gravity;
		else if (!was_on_ground)
			velocity.z = cvars.sv_jump_impulse->GetFloat() - ival * gravity;

		Vector src = origin;
		Vector end = src + (velocity * ival);

		Ray_t ray;
		ray.Init(src, end, mins, maxs);

		CGameTrace trace;
		CTraceFilterWorldAndPropsOnly filter;
		EngineTrace->TraceRay(ray, MASK_PLAYERSOLID, &filter, &trace);

		if (trace.fraction != 1.f) {
			for (int j = 0; j < 2; j++) {
				velocity -= trace.plane.normal * velocity.Dot(trace.plane.normal);

				float dot = velocity.Dot(trace.plane.normal);
				if (dot < 0.f) {
					velocity.x -= dot * trace.plane.normal.x;
					velocity.y -= dot * trace.plane.normal.y;
					velocity.z -= dot * trace.plane.normal.z;
				}

				end = trace.endpos + (velocity * (ival * (1.f - trace.fraction)));
				ray.Init(trace.endpos, end, mins, maxs);
				EngineTrace->TraceRay(ray, MASK_PLAYERSOLID, &filter, &trace);

				if (trace.fraction == 1.f)
					break;
			}
		}

		origin = trace.endpos;
		was_on_ground = (flags & FL_ONGROUND) != 0;

		Vector ground_end = origin;
		ground_end.z -= 2.f;
		ray.Init(origin, ground_end, mins, maxs);
		EngineTrace->TraceRay(ray, MASK_PLAYERSOLID, &filter, &trace);

		flags &= ~FL_ONGROUND;
		if (trace.DidHit() && trace.plane.normal.z > 0.7f)
			flags |= FL_ONGROUND;
	}

	new_record->m_vecOrigin    = origin;
	new_record->m_vecAbsOrigin = origin;
	new_record->m_vecVelocity  = velocity;
	new_record->m_fFlags       = flags;

	Utils::MatrixMove(new_record->aim_matrix,      128, record->m_vecOrigin, origin);
	Utils::MatrixMove(new_record->opposite_matrix, 128, record->m_vecOrigin, origin);
	Utils::MatrixMove(new_record->bone_matrix,     128, record->m_vecOrigin, origin);

	new_record->BuildMatrix();

	return new_record;
}

float CLagCompensation::GetLerpTime() {
	static const auto cl_interp = CVar->FindVar("cl_interp");
	static const auto cl_updaterate = CVar->FindVar("cl_updaterate");
	static const auto sv_minupdaterate = CVar->FindVar("sv_minupdaterate");
	static const auto sv_maxupdaterate = CVar->FindVar("sv_maxupdaterate");
	static const auto cl_interp_ratio = CVar->FindVar("cl_interp_ratio");
	static const auto sv_min_interp_ratio = CVar->FindVar("sv_client_min_interp_ratio");
	static const auto sv_max_interp_ratio = CVar->FindVar("sv_client_max_interp_ratio");

	const float update_rate = std::clamp<float>(cl_updaterate->GetFloat(), sv_minupdaterate->GetFloat(), sv_maxupdaterate->GetFloat());
	const float interp_ratio = std::clamp<float>(cl_interp_ratio->GetFloat(), sv_min_interp_ratio->GetFloat(), sv_max_interp_ratio->GetFloat());

	return std::clamp<float>(interp_ratio / update_rate, cl_interp->GetFloat(), 1.f);
}

bool CLagCompensation::ValidRecord(LagRecord* record) {
	if (!record || !record->player || record->invalid)
		return false;

	INetChannelInfo* nci = EngineClient->GetNetChannelInfo();
	if (!nci)
		return false;

	if (!cvars.cl_lagcompensation || !cvars.cl_lagcompensation->GetInt())
		return false;

	float rtt = nci->GetLatency(FLOW_OUTGOING) + nci->GetLatency(FLOW_INCOMING);
	float correct = rtt + GetLerpTime();
	correct = std::clamp(correct, 0.0f, cvars.sv_maxunlag->GetFloat());

	int last_server_tick = ClientState->m_ClockDriftMgr.m_nServerTick;
	float server_time = TICKS_TO_TIME(last_server_tick);

	float deadtime = server_time + rtt - cvars.sv_maxunlag->GetFloat();
	if (record->m_flSimulationTime <= deadtime)
		return false;

	int possible_future_tick = last_server_tick + TIME_TO_TICKS(rtt) + 8;
	if (TIME_TO_TICKS(record->m_flSimulationTime + GetLerpTime()) > possible_future_tick)
		return false;

	float delta_time = correct - (server_time - record->m_flSimulationTime);

	if (Exploits->IsShifting() && ctx.tickbase_shift > 2) {
		float dt1 = correct - (server_time - GlobalVars->interval_per_tick - record->m_flSimulationTime);
		float dt2 = correct - (server_time + GlobalVars->interval_per_tick - record->m_flSimulationTime);
		return std::abs(dt1) < 0.2f && std::abs(dt2) < 0.2f;
	}

	return std::abs(delta_time) < 0.2f;
}

LagRecord* CLagCompensation::GetLastRecord(int idx) {
	LagRecord* record = nullptr;
	auto& records = lag_records[idx];
	for (auto it = records.rbegin(); it != records.rend(); it++) {
		if (!ValidRecord(&*it)) {
			if (it->breaking_lag_comp || it->invalid)
				break;

			continue;
		}
		record = &*it;
	}

	return record;
}

void CLagCompensation::Reset(int index) {
	if (index != -1) {
		lag_records[index].clear();
		extrapolated_records[index].clear();
		max_simulation_time[index] = 0.f;
		last_update_tick[index] = 0;
		max_tickbase[index] = 0;
		is_in_defensive[index] = false;
	}
	else {
		for (int i = 0; i < (int)lag_records.size(); i++) {
			lag_records[i].clear();
			extrapolated_records[i].clear();
			max_simulation_time[i] = 0.f;
			last_update_tick[i] = 0;
			max_tickbase[i] = 0;
			is_in_defensive[i] = false;
		}
	}
}

void CLagCompensation::Invalidate(int index) {
	for (auto& record : lag_records[index])
		record.invalid = true;
}

CLagCompensation* LagCompensation = new CLagCompensation;