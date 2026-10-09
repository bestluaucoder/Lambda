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
	LagRecord* record = new LagRecord;

	record->player = player;
	record->m_vecAbsOrigin = player->GetAbsOrigin();
	RecordDataIntoTrack(player, record);

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
	memcpy(clamped_matrix, aim_matrix, 128 * sizeof(matrix3x4_t));
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

	for (int i = 0; i < ClientState->m_nMaxClients; i++) {
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
			float dist_sq = (prev_valid->m_vecOrigin - new_record->m_vecOrigin).LengthSqr();
			float time_delta = new_record->m_flSimulationTime - prev_valid->m_flSimulationTime;
			
			float max_move = 64.f;
			
			if (time_delta > GlobalVars->interval_per_tick)
				max_move *= (time_delta / GlobalVars->interval_per_tick);
			
			float max_move_sq = max_move * max_move;
			
			new_record->breaking_lag_comp = dist_sq > max_move_sq;
			
			Vector velocity_delta = new_record->m_vecVelocity - prev_valid->m_vecVelocity;
			float velocity_change = velocity_delta.Length() / time_delta;
			if (velocity_change > 320.f)
				new_record->breaking_lag_comp = true;
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

		
		if (config.menu_misc.experimental_lagcomp && config.menu_misc.experimental_lagcomp->get()) {
			auto& rvec = lag_records_vec[i];
			rvec.push_back(records.back());
			const size_t max_vec = pl->IsTeammate() ? 4 : TIME_TO_TICKS(cvars.sv_maxunlag->GetFloat()) + 3;
			if (rvec.size() > max_vec)
				rvec.erase(rvec.begin());
		}

		INetChannelInfo* nci = EngineClient->GetNetChannelInfo();
		if (config.visuals.esp.show_server_hitboxes->get() && nci && nci->IsLoopback())
			pl->DrawServerHitboxes(GlobalVars->interval_per_tick, true);
	}
}

LagRecord* CLagCompensation::ExtrapolateRecord(LagRecord* record, int ticks) {
	if (ticks <= 0)
		return nullptr;

	const float time = TICKS_TO_TIME(ticks);

	auto& records = extrapolated_records[record->player->EntIndex()];

	while (records.size() > 64)
		records.pop_front();

	LagRecord* new_record = &records.emplace_back();

	*new_record = *record;
	new_record->m_flSimulationTime += time;
	new_record->update_tick += ticks;

	const float gravity = cvars.sv_gravity->GetFloat();
	const float ival = GlobalVars->interval_per_tick;
	const float friction = 4.f;
	const float stop_speed = 100.f;

	Vector velocity = new_record->m_vecVelocity;
	int flags = new_record->m_fFlags;
	bool on_ground = (flags & FL_ONGROUND) != 0;

	float current_speed = velocity.Length2D();
	float max_speed = 250.f;
	
	if (record->player->m_bIsScoped())
		max_speed = 100.f;
	else if ((flags & FL_DUCKING) || new_record->m_flDuckAmout > 0.5f)
		max_speed = 100.f;

	for (int i = 0; i < ticks; i++) {
		if (!on_ground) {
			velocity.z -= gravity * ival * 0.5f;

			if (velocity.z < -gravity * 0.5f)
				velocity.z = -gravity * 0.5f;
		}
		else {
			velocity.z = 0.f;
			
			float speed = velocity.Length2D();
			if (speed > 0.1f) {
				float drop = 0.f;
				float control = speed < stop_speed ? stop_speed : speed;
				drop = control * friction * ival;
				
				float new_speed = speed - drop;
				if (new_speed < 0.f) new_speed = 0.f;
				
				if (speed > 0.f) {
					new_speed /= speed;
					velocity.x *= new_speed;
					velocity.y *= new_speed;
				}
			}
		}

		Vector next_origin = new_record->m_vecOrigin + velocity * ival;

		if (!on_ground) {
			CGameTrace tr;
			CTraceFilterWorldOnly filter;
			Ray_t ray;
			ray.Init(next_origin + Vector(0, 0, 2.f), next_origin - Vector(0, 0, 8.f));
			EngineTrace->TraceRay(ray, MASK_PLAYERSOLID_BRUSHONLY, &filter, &tr);

			if (tr.fraction < 1.f && tr.plane.normal.z > 0.7f) {
				next_origin.z = tr.endpos.z + 2.f;
				velocity.z = 0.f;
				on_ground = true;
				flags |= FL_ONGROUND;
			}
		}
		else {
			CGameTrace tr;
			CTraceFilterWorldOnly filter;
			Ray_t ray;
			ray.Init(next_origin, next_origin - Vector(0, 0, 2.f));
			EngineTrace->TraceRay(ray, MASK_PLAYERSOLID_BRUSHONLY, &filter, &tr);

			if (tr.fraction >= 1.f || tr.plane.normal.z <= 0.7f) {
				on_ground = false;
				flags &= ~FL_ONGROUND;
			}
		}

		new_record->m_vecOrigin = next_origin;
	}

	new_record->m_vecVelocity = velocity;
	new_record->m_fFlags = flags;
	new_record->m_vecAbsOrigin = new_record->m_vecOrigin;

	Utils::MatrixMove(new_record->aim_matrix, 128, record->m_vecOrigin, new_record->m_vecOrigin);
	Utils::MatrixMove(new_record->opposite_matrix, 128, record->m_vecOrigin, new_record->m_vecOrigin);
	Utils::MatrixMove(new_record->bone_matrix, 128, record->m_vecOrigin, new_record->m_vecOrigin);

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
	if (!record || !record->player || record->shifting_tickbase || record->breaking_lag_comp || record->invalid)
		return false;

	INetChannelInfo* nci = EngineClient->GetNetChannelInfo();
	
	float correct = 0.0f;
	if (nci)
		correct = nci->GetLatency(FLOW_OUTGOING) + nci->GetLatency(FLOW_INCOMING);

	correct += GetLerpTime();
	correct = std::clamp(correct, 0.0f, cvars.sv_maxunlag->GetFloat());

	float record_age = TICKS_TO_TIME(ctx.corrected_tickbase) - record->m_flSimulationTime;
	
	if (record_age < 0.f || record_age > cvars.sv_maxunlag->GetFloat())
		return false;

	float deltaTime = correct - record_age;
	float choke_tolerance = TICKS_TO_TIME(record->m_nChokedTicks);
	float tolerance = 0.2f + choke_tolerance - (ctx.tickbase_shift > 0 ? GlobalVars->interval_per_tick * 0.5f : 0.f);

	if (std::abs(deltaTime) >= tolerance)
		return false;

	int tick_diff = GlobalVars->tickcount - record->update_tick;
	if (tick_diff > TIME_TO_TICKS(cvars.sv_maxunlag->GetFloat()) + 2)
		return false;

	return true;
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
		lag_records_vec[index].clear();
		max_simulation_time[index] = 0.f;
		last_update_tick[index] = 0;
		max_tickbase[index] = 0;
		is_in_defensive[index] = false;
	}
	else {
		for (int i = 0; i < (int)lag_records.size(); i++) {
			lag_records[i].clear();
			lag_records_vec[i].clear();
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