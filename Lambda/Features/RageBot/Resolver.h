#pragma once

#include <deque>
#include <array>

#include "../../SDK/Misc/CBasePlayer.h"

struct LagRecord;

enum class R_PlayerState {
	STANDING,
	MOVING,
	AIR
};

enum class R_AntiAimType {
	NONE,
	STATIC,
	JITTER,
	UNKNOWN,
};

enum class ResolverType {
	NONE,
	FREESTAND,
	LOGIC,
	ANIM,
	MEMORY,
	DEFAULT,
	LBY,
	VELOCITY,
	MOVEANGLE,
	TICKBASE,
};

struct ResolverLayer_t {
	float desync = 0.f;
	float delta  = 0.f;
};

#define RESOLVER_DEBUG 1

struct ResolverData_t {
	R_PlayerState player_state  = R_PlayerState::STANDING;
	R_AntiAimType antiaim_type  = R_AntiAimType::UNKNOWN;
	ResolverType  resolver_type = ResolverType::NONE;

	ResolverLayer_t layers[5];

	float max_desync_delta     = 0.f;
	int   side                 = 0;
	bool  is_shifting_tickbase = false;
	int   detected_shift_ticks = 0;
	float resolved_body_yaw    = 0.f;
};

struct ResolverDataStatic_t {
	int          brute_side     = 0;
	float        brute_time     = 0.f;
	float        last_resolved  = 0.f;
	ResolverType res_type_last  = ResolverType::NONE;
	int          last_side      = 0;
	int          missed_shots   = 0;

	float lby_delta       = 0.f;
	float lby_last_update = 0.f;
	float lby_value       = 0.f;

	static constexpr int JITTER_HISTORY = 20;
	std::array<float, JITTER_HISTORY> eye_yaw_history = {};
	int eye_yaw_head  = 0;
	int eye_yaw_count = 0;

	float prev_speed    = 0.f;
	int   accel_side    = 0;

	float move_yaw_delta_sum = 0.f;
	int   move_yaw_samples   = 0;
	int   move_yaw_side      = 0;

	float last_sim_time        = 0.f;
	int   shift_ticks_observed = 0;
	float shift_first_seen     = 0.f;
	int   shift_side_votes     = 0;
	int   shift_vote_count     = 0;
	int   tickbase_side        = 0;

	float resolved_body_yaw    = 0.f;

	void reset() {
		brute_side           = 0;
		brute_time           = 0.f;
		last_resolved        = 0.f;
		res_type_last        = ResolverType::NONE;
		last_side            = 0;
		missed_shots         = 0;
		lby_delta            = 0.f;
		lby_last_update      = 0.f;
		lby_value            = 0.f;
		eye_yaw_history.fill(0.f);
		eye_yaw_head         = 0;
		eye_yaw_count        = 0;
		prev_speed           = 0.f;
		accel_side           = 0;
		move_yaw_delta_sum   = 0.f;
		move_yaw_samples     = 0;
		move_yaw_side        = 0;
		last_sim_time        = 0.f;
		shift_ticks_observed = 0;
		shift_first_seen     = 0.f;
		shift_side_votes     = 0;
		shift_vote_count     = 0;
		tickbase_side        = 0;
		resolved_body_yaw    = 0.f;
	}
};

class CResolver {
	ResolverDataStatic_t resolver_data[64];

	float GetTime();
	float GetLatency();

	R_AntiAimType DetectAntiAim      (CBasePlayer* player, const std::deque<LagRecord>& records);
	R_PlayerState DetectPlayerState  (CBasePlayer* player, AnimationLayer* animlayers);

	void  UpdateLBY          (CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p);
	void  UpdateJitterHistory(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p);
	void  UpdateVelocity     (CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p);
	void  UpdateMoveYaw      (CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p);
	void  DetectTickbaseShift(CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p);

	int   ResolveLBY         (CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p, float& out_body_yaw);
	int   ResolveTickbase    (CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p);
	int   ResolveAnim        (CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p);
	int   ResolveJitter      (CBasePlayer* player, LagRecord* record, ResolverDataStatic_t* p);
	int   ResolveFreestand   (CBasePlayer* player, LagRecord* record, const std::deque<LagRecord>& records);

	void  SetupLayer         (LagRecord* record, int idx, float delta);
	void  SetupResolverLayers(CBasePlayer* player, LagRecord* record);

public:
	void  Apply              (LagRecord* record);

public:
	CResolver() {
		for (int i = 0; i < 64; ++i)
			resolver_data[i].reset();
	}

	void Reset(CBasePlayer* pl = nullptr);
	void Run  (CBasePlayer* player, LagRecord* record, std::deque<LagRecord>& records);
	void OnMiss(CBasePlayer* player, LagRecord* record);
	void OnHit (CBasePlayer* player, LagRecord* record);
};

extern CResolver* Resolver;
