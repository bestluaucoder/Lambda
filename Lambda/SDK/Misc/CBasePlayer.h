#pragma once

#include "CBaseEntity.h"
#include "CBaseCombatWeapon.h"
#include "../../Utils/NetVars.h"
#include "../../Utils/Utils.h"
#include <array>
#include "UtlVector.h"

#pragma region DEFINES

#define	FL_ONGROUND				(1 << 0)
#define FL_DUCKING				(1 << 1)
#define	FL_WATERJUMP			(1 << 3)
#define FL_ONTRAIN				(1 << 4)
#define FL_INRAIN				(1 << 5)
#define FL_FROZEN				(1 << 6)
#define FL_ATCONTROLS			(1 << 7)
#define	FL_CLIENT				(1 << 8)
#define FL_FAKECLIENT			(1 << 9)
#define	FL_INWATER				(1 << 10)
#define FL_HIDEHUD_SCOPE		(1 << 11)

enum EMoveType
{
    MOVETYPE_NONE = 0,	
    MOVETYPE_ISOMETRIC,			
    MOVETYPE_WALK,				
    MOVETYPE_STEP,				
    MOVETYPE_FLY,				
    MOVETYPE_FLYGRAVITY,		
    MOVETYPE_VPHYSICS,			
    MOVETYPE_PUSH,				
    MOVETYPE_NOCLIP,			
    MOVETYPE_LADDER,			
    MOVETYPE_OBSERVER,			
    MOVETYPE_CUSTOM,			

    
    MOVETYPE_LAST = MOVETYPE_CUSTOM,

    MOVETYPE_MAX_BITS = 4
};

enum LifeState_t {
    LIFE_ALIVE,				 
    LIFE_DYING,				 
    LIFE_DEAD,				 
    LIFE_RESPAWNABLE,
    LIFE_DISCARDBODY,
};

enum
{
    EF_BONEMERGE = 0x001,	
    EF_BRIGHTLIGHT = 0x002,	
    EF_DIMLIGHT = 0x004,	
    EF_NOINTERP = 0x008,	
    EF_NOSHADOW = 0x010,	
    EF_NODRAW = 0x020,	
    EF_NORECEIVESHADOW = 0x040,	
    EF_BONEMERGE_FASTCULL = 0x080,	
    
    
    
    
    EF_ITEM_BLINK = 0x100,	
    EF_PARENT_ANIMATES = 0x200,	
    EF_MARKED_FOR_FAST_REFLECTION = 0x400,	
    EF_NOSHADOWDEPTH = 0x800,	
    EF_SHADOWDEPTH_NOCACHE = 0x1000,	
    EF_NOFLASHLIGHT = 0x2000,
    EF_NOCSM = 0x4000,	
    EF_MAX_BITS = 15
};


enum
{
    EFL_KILLME = (1 << 0),	
    EFL_DORMANT = (1 << 1),	
    EFL_NOCLIP_ACTIVE = (1 << 2),	
    EFL_SETTING_UP_BONES = (1 << 3),	
    EFL_KEEP_ON_RECREATE_ENTITIES = (1 << 4), 

    EFL_DIRTY_SHADOWUPDATE = (1 << 5),	
    EFL_NOTIFY = (1 << 6),	

    
    
    
    
    EFL_FORCE_CHECK_TRANSMIT = (1 << 7),

    EFL_BOT_FROZEN = (1 << 8),	
    EFL_SERVER_ONLY = (1 << 9),	
    EFL_NO_AUTO_EDICT_ATTACH = (1 << 10), 

    
    EFL_DIRTY_ABSTRANSFORM = (1 << 11),
    EFL_DIRTY_ABSVELOCITY = (1 << 12),
    EFL_DIRTY_ABSANGVELOCITY = (1 << 13),
    EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS = (1 << 14),
    EFL_DIRTY_SPATIAL_PARTITION = (1 << 15),
    EFL_HAS_PLAYER_CHILD = (1 << 16),	

    EFL_IN_SKYBOX = (1 << 17),	
    
    EFL_USE_PARTITION_WHEN_NOT_SOLID = (1 << 18),	
    EFL_TOUCHING_FLUID = (1 << 19),	

    
    EFL_IS_BEING_LIFTED_BY_BARNACLE = (1 << 20),
    EFL_NO_ROTORWASH_PUSH = (1 << 21),		
    EFL_NO_THINK_FUNCTION = (1 << 22),
    EFL_NO_GAME_PHYSICS_SIMULATION = (1 << 23),

    EFL_CHECK_UNTOUCH = (1 << 24),
    EFL_DONTBLOCKLOS = (1 << 25),		
    EFL_DONTWALKON = (1 << 26),		
    EFL_NO_DISSOLVE = (1 << 27),		
    EFL_NO_MEGAPHYSCANNON_RAGDOLL = (1 << 28),	
    EFL_NO_WATER_VELOCITY_CHANGE = (1 << 29),	
    EFL_NO_PHYSCANNON_INTERACTION = (1 << 30),	
    EFL_NO_DAMAGE_FORCES = (1 << 31),	
};

enum EObsMode
{
    OBS_MODE_NONE = 0,	
    OBS_MODE_DEATHCAM,	
    OBS_MODE_FREEZECAM,	
    OBS_MODE_FIXED,		
    OBS_MODE_IN_EYE,	
    OBS_MODE_CHASE,		
    OBS_MODE_ROAMING,	

    NUM_OBSERVER_MODES,
};

enum InvalidatePhysicsBits_t
{
    POSITION_CHANGED = 0x1,
    ANGLES_CHANGED = 0x2,
    VELOCITY_CHANGED = 0x4,
    ANIMATION_CHANGED = 0x8,		
    BOUNDS_CHANGED = 0x10,		
    SEQUENCE_CHANGED = 0x20,		
};

enum animstate_layer_t
{
    ANIMATION_LAYER_AIMMATRIX = 0,
    ANIMATION_LAYER_WEAPON_ACTION,
    ANIMATION_LAYER_WEAPON_ACTION_RECROUCH,
    ANIMATION_LAYER_ADJUST,
    ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL,
    ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB,
    ANIMATION_LAYER_MOVEMENT_MOVE,
    ANIMATION_LAYER_MOVEMENT_STRAFECHANGE,
    ANIMATION_LAYER_WHOLE_BODY,
    ANIMATION_LAYER_FLASHED,
    ANIMATION_LAYER_FLINCH,
    ANIMATION_LAYER_ALIVELOOP,
    ANIMATION_LAYER_LEAN,
    ANIMATION_LAYER_COUNT,
};

enum PoseParam_t {
    STRAFE_YAW,
    STAND,
    LEAN_YAW,
    SPEED,
    LADDER_YAW,
    LADDER_SPEED,
    JUMP_FALL,
    MOVE_YAW,
    MOVE_BLEND_CROUCH,
    MOVE_BLEND_WALK,
    MOVE_BLEND_RUN,
    BODY_YAW,
    BODY_PITCH,
    AIM_BLEND_STAND_IDLE,
    AIM_BLEND_STAND_WALK,
    AIM_BLEND_STAND_RUN,
    AIM_BLEND_COURCH_IDLE,
    AIM_BLEND_CROUCH_WALK,
    DEATH_YAW
};

enum ETeamNumber {
    TEAM_SPECTATOR = 1,
    TEAM_TERRORIST = 2,
    TEAM_CT = 3
};

enum CSGOActivities {
    ACT_CSGO_JUMP = 985,
    ACT_CSGO_FALL,
    ACT_CSGO_CLIMB_LADDER,
    ACT_CSGO_LAND_LIGHT,
    ACT_CSGO_LAND_HEAVY,
    ACT_CSGO_EXIT_LADDER_TOP,
    ACT_CSGO_EXIT_LADDER_BOTTOM,
};

#pragma endregion

class CBasePlayer;
class CBaseCombatWeapon;

struct CCSGOPlayerAnimationState
{
    char	pad0[0x60];
    CBasePlayer* pEntity;
    CBaseCombatWeapon* pWeapon;
    CBaseCombatWeapon* pWeaponLast;
    float		flLastUpdateTime;
    int			nLastUpdateFrame;
    float		flLastUpdateIncrement;
    float		flEyeYaw;
    float		flEyePitch;
    float		flFootYaw;
    float		flLastFootYaw;
    float		flMoveYaw;
    float		flMoveYawIdeal;
    float		flMoveYawCurrentToIdeal;
    float       flTimeToAlignLowerBody;
    float		flPrimaryCycle;
    float		flMoveWeight;
    float		flMoveWeightSmoothed;
    float		flDuckAmount;
    float		flDuckAdditional;
    float		flRecrouchWeight;
    Vector		vecOrigin;
    Vector		vecLastOrigin;
    Vector		vecVelocity;
    Vector		vecVelocityNormalized;
    Vector		vecVelocityNormalizedNonZero;
    float		flVelocityLenght2D;
    float		flVelocityZ;
    float		flRunSpeedNormalized;
    float		flWalkSpeedNormalized;
    float		flCrouchSpeedNormalized;
    float		flDurationMoving;
    float		flDurationStill;
    bool		bOnGround;
    bool		bLanding;
    float       flJumpToFall;
    float		flDurationInAir;
    float		flLeftGroundHeight;
    float		flHitGroundWeight;
    float		flWalkToRunTransition;
    char	    __pad3[0x4];
    float		flInAirSmoothValue;
    bool        bOnLadder;
    float       flLadderWeights;
    float       flLadderSpeed;
    bool        bWalkToRunTransitionState;
    bool        bDefuseStarted;
    bool        bPlantAnimStarted;
    bool        bTwitchAnimStarted;
    bool        bAdjustStarted;
    char        vecActivityModifiers[20];
    float       flNextTwitchTime;
    float       flTimeOfLastKnownInjury;
    float       flLastVelocityTestTime;
    Vector      vecVelocityLast;
    Vector      vecTargetAcceleration;
    Vector      vecAcceleration;
    float       flAccelerationWeight;
    float       flAimMatrixTransition;
    float       flAimMatrixTransitionDelay;
    bool        bFlashed;
    float       flStrafeChangeWeight;
    float       flStrafeChangeTargetWeight;
    float       flStrafeChangeCycle;
    int         nStrafeSequence;
    bool        bStrafeChanging;
    float       flDurationStrafing;
    float       flFootLerp;
    bool        bFeetCrossed;
    bool        bPlayerIsAccelerating;
    char        __pad4[0x178];
    float       flCameraSmoothHeight;
    bool        bSmoothHeightValid;
    float		flLastTimeVelocityOverTen;
    float		flAimYawMin;
    float		flAimYawMax;
    float		flAimPitchMin;
    float		flAimPitchMax;
    int         iAnimsetVersion;

    void        Update(const QAngle& angles, bool bForce = false);
    void        ForceUpdate();
};
static_assert(sizeof(CCSGOPlayerAnimationState) == 0x348);

struct AnimationLayer {
    bool m_bClientBlend;		 
    float m_flBlendIn;			 
    void* m_pStudioHdr;			 
    int m_nDispatchSequence;     
    int m_nDispatchSequence_2;   
    uint32_t m_nOrder = 0;           
    uint32_t m_nSequence = 0;        
    float m_flPrevCycle = 0.f;       
    float m_flWeight = 0.f;          
    float m_flWeightDeltaRate = 0.f; 
    float m_flPlaybackRate = 0.f;    
    float m_flCycle = 0.f;           
    CBasePlayer* m_pOwner = nullptr;       
    char pad_0038[4];            

    void set_data(const AnimationLayer& other) {
        if (m_pOwner != other.m_pOwner)
            return;

        m_flWeight = other.m_flWeight;
        m_flPlaybackRate = other.m_flPlaybackRate;
        m_nSequence = other.m_nSequence;
        m_flCycle = other.m_flCycle;
    }
};

class CUserCmd;
class C_BaseAnimating;


class CBoneAccessor
{
public:
    C_BaseAnimating* m_pAnimating;

    matrix3x4_t* m_pBones;

    int m_ReadableBones;		
    int m_WritableBones;		
};

class CStudioHdr;

#define MAX_WEAPONS 64
#define MAX_VIEWMODELS 2
#define INVALID_EHANDLE_INDEX	0xFFFFFFFF

class CBasePlayer : public CBaseEntity {
public:
    NETVAR(m_iHealth, int, "DT_BasePlayer", "m_iHealth")
    NETVAR(m_fFlags, int, "DT_CSPlayer", "m_fFlags")
    NETVAR(m_vecViewOffset, Vector, "DT_BasePlayer", "m_vecViewOffset[0]")
    NETVAR(m_iTeamNum, int, "DT_BaseEntity", "m_iTeamNum")
    NETVAR(m_bIsScoped, bool, "DT_CSPlayer", "m_bIsScoped")
    NETVAR(m_bResumeZoom, bool, "DT_CSPlayer", "m_bResumeZoom")
    NETVAR_O(m_MoveType, int, "DT_BaseEntity", "m_nRenderMode", 1)
    NETVAR(m_flDuckAmount, float, "DT_BasePlayer", "m_flDuckAmount")
    NETVAR(m_flDuckSpeed, float, "DT_BasePlayer", "m_flDuckSpeed")
    NETVAR(m_ArmorValue, int, "DT_CSPlayer", "m_ArmorValue")
    NETVAR(m_bHasHelmet, bool, "DT_CSPlayer", "m_bHasHelmet")
    NETVAR(m_bIsDefusing, bool, "DT_CSPlayer", "m_bIsDefusing")
    NETVAR(m_hActiveWeapon, unsigned long, "DT_BaseCombatCharacter", "m_hActiveWeapon")
    PNETVAR_O(m_hMyWeapons, unsigned long, "DT_BaseCombatCharacter", "m_hActiveWeapon", -256)
    NETVAR(m_nTickBase, int, "DT_BasePlayer", "m_nTickBase")
    NETVAR(m_lifeState, int, "DT_BasePlayer", "m_lifeState")
    NETVAR(m_iShotsFired, int, "DT_CSPlayer", "m_iShotsFired")
    NETVAR(m_nHitboxSet, int, "DT_BasePlayer", "m_nHitboxSet")
    NETVAR(m_angEyeAngles, QAngle, "DT_CSPlayer", "m_angEyeAngles")
    NETVAR(m_viewPunchAngle, QAngle, "DT_BasePlayer", "m_viewPunchAngle")
    NETVAR(m_aimPunchAngle, QAngle, "DT_BasePlayer", "m_aimPunchAngle")
    NETVAR(m_aimPunchAngleVel, QAngle, "DT_BasePlayer", "m_aimPunchAngleVel")
    NETVAR(m_flNextAttack, float, "DT_BaseCombatCharacter", "m_flNextAttack")
    NETVAR(m_flVelocityModifier, float, "DT_CSPlayer", "m_flVelocityModifier")
    NETVAR(m_flFlashDuration, float, "DT_CSPlayer", "m_flFlashDuration")
    NETVAR(m_hObserverTarget, unsigned long, "DT_BasePlayer", "m_hObserverTarget")
    NETVAR(m_flLowerBodyYawTarget, float, "DT_CSPlayer", "m_flLowerBodyYawTarget")
    NETVAR(m_iObserverMode, int, "DT_BasePlayer", "m_iObserverMode")
    NETVAR(m_nSequence, int, "DT_BaseAnimating", "m_nSequence")
    NETVAR(m_flThirdpersonRecoil, float, "DT_CSPlayer", "m_flThirdpersonRecoil")
    NETVAR(m_bStrafing, bool, "DT_CSPlayer", "m_bStrafing")
    NETVAR(m_nNextThinkTick, int, "DT_BasePlayer", "m_nNextThinkTick")
    NETVAR(m_flCycle, float, "DT_BaseAnimating", "m_flCycle")
    NETVAR(m_bGunGameImmunity, bool, "DT_CSPlayer", "m_bGunGameImmunity")
    NETVAR(m_iAddonBits, int, "DT_CSPlayer", "m_iAddonBits")
    NETVAR(m_bSpotted, bool, "DT_BaseEntity", "m_bSpotted")
    OFFSET(m_flFallVelocity, float, 0x58)
    OFFSET(m_nSimulationTick, int, 0x2AC)
    NETVAR_O(m_nFinalPredictedTick, int, "DT_CSPlayer", "m_nTickBase", 0x4)
    NETVAR(m_bClientSideAnimation, bool, "DT_BaseAnimating", "m_bClientSideAnimation")
    NETVAR(m_hVehicle, unsigned long, "DT_BasePlayer", "m_hVehicle")
    PNETVAR(m_hViewModel, unsigned long, "DT_BasePlayer", "m_hViewModel[0]")
    PPRED_DESC_MAP(m_nButtons, int, "m_nButtons")
    NETVAR(m_nOldButtons, int, "DT_BasePlayer", "m_nOldButtons")
    PRED_DESC_MAP(m_afButtonsLast, int, "m_afButtonLast")
    PRED_DESC_MAP(m_afButtonsPressed, int, "m_afButtonPressed")
    PRED_DESC_MAP(m_afButtonsReleased, int, "m_afButtonReleased")
    PRED_DESC_MAP(m_fEffects, int, "m_fEffects")
    PRED_DESC_MAP(m_vecAbsVelocity, Vector, "m_vecAbsVelocity")
    NETVAR_O(GetAnimstate, CCSGOPlayerAnimationState*, "DT_CSPlayer", "m_bIsScoped", -0x14)
    NETVAR(deadflag, bool, "DT_BasePlayer", "deadflag")
    NETVAR_O(v_angle, QAngle, "DT_BasePlayer", "deadflag", 0x4)
    OFFSET(m_nOcclusionFlags, uint32_t, 0xA28)
    OFFSET(m_nOcclusionFrame, int, 0xA30)
    OFFSET(m_BoneAccessor, CBoneAccessor, 0x26a4)
    NETVAR(m_bIsWalking, bool, "DT_CSPlayer", "m_bIsWalking")
    NETVAR_O(m_iMostRecentModelBoneCounter, unsigned long, "DT_BaseAnimating", "m_nForceBone", 0x4)
    PRED_DESC_MAP(m_nSkin, int, "m_nSkin")
    PRED_DESC_MAP(m_nBody, int, "m_nBody")
    NETVAR_O(m_surfaceFriction, float, "DT_BasePlayer", "m_flWaterJumpTime", 17)
    NETVAR(m_iAccount, int, "DT_CSPlayer", "m_iAccount")
    PRED_DESC_MAP(m_vecNetworkOrigin, Vector, "m_vecNetworkOrigin")
    NETVAR(m_hGroundEntity, unsigned int, "DT_BasePlayer", "m_hGroundEntity")
    NETVAR(m_flMaxspeed, float, "DT_BasePlayer", "m_flMaxspeed")

    bool& m_bMaintainSequenceTransitions();
    bool& m_bUseNewAnimstate();
    float& m_flLastCollisionChangeTime();

    void SetModelIndex( int modelIndex );

    inline bool IsArmored(const int iHitGroup)
    {
        

        bool bIsArmored = false;

        if (this->m_ArmorValue() > 0)
        {
            switch (iHitGroup)
            {
            case HITGROUP_GENERIC:
            case HITGROUP_CHEST:
            case HITGROUP_STOMACH:
            case HITGROUP_LEFTARM:
            case HITGROUP_RIGHTARM:
            case HITGROUP_NECK:
                bIsArmored = true;
                break;
            case HITGROUP_HEAD:
                if (this->m_bHasHelmet())
                    bIsArmored = true;
                [[fallthrough]];
            case HITGROUP_LEFTLEG:
            case HITGROUP_RIGHTLEG:
                break;
            default:
                break;
            }
        }

        return bIsArmored;
    }

    void SetSequence(int iSequence)
    {
        CallVFunction<void(__thiscall*)(CBasePlayer*, int)>(this, 219)(this, iSequence);
    }

    void StudioFrameAdvance()
    {
        CallVFunction<void(__thiscall*)(CBasePlayer*)>(this, 220)(this);
    }

    void PreThink()
    {
        CallVFunction<void(__thiscall*)(CBasePlayer*)>(this, 318)(this);
    }

    void UpdateCollisionBounds()
    {
        CallVFunction<void(__thiscall*)(CBasePlayer*)>(this, 340)(this);
    }

    void InvalidatePhysicsRecursive(int flags) {
        static auto m_uInvalidatePhysics = Utils::PatternScan("client.dll", "55 8B EC 83 E4 F8 83 EC 0C 53 8B 5D 08 8B C3 56 83 E0 04");
        reinterpret_cast <void(__thiscall*)(void*, int)> (m_uInvalidatePhysics)(this, flags);
    }

    std::string         GetName();
    bool                IsTeammate();
    bool                IsEnemy();
    float               GetMaxDesyncDelta();
    bool                IsAlive();
    Vector              GetBonePosition(int bone);
    Vector              GetHitboxCenter(int hitbox, matrix3x4_t* matrix = nullptr);
    CUtlVector<matrix3x4_t> GetCachedBoneData();
    float               ScaleDamage(int hitgroup, CCSWeaponData* weaponData, float& damage);

    Vector              GetEyePosition();
    void                ModifyEyePosition(Vector& eye_position);
    Vector              GetShootPosition();
    void                UpdateClientSideAnimation();
    AnimationLayer*     GetAnimlayers();
    CBaseCombatWeapon*  GetActiveWeapon();
    bool                SetupBones(matrix3x4_t* boneToWorld, int maxBones, int mask, float curTime = 0.f);
    void                SetupBones_AttachmentHelper();
    std::array<float, 24>& m_flPoseParameter();
    C_CommandContext*   GetCommandContext(); 
    void                SetAbsVelocity(const Vector& vecAbsVelocity);
    float               GetMaxSpeed();
    void                CopyBones(matrix3x4_t* boneMatrix);
    void                ClampBonesInBBox(matrix3x4_t* bones, int bone_mask);
    CBasePlayer*        GetObserverTarget();

    int&                m_nImpulse();
    int                 GetButtonForced();
    int                 GetButtonDisabled();
    CUserCmd**          GetCurrentCommand();
    CUserCmd&           GetLastCommand();
    void                Think();
    bool                PhysicsRunThink(int nThinkMetod);
    void                PostThink();
    void                SelectItem(const char* string, int subtype);
    bool                UsingStandardWeaponsInVehicle();
    void                UpdateButtonState(int button);
    bool                IsHitboxArmored(int hitbox);
    void                SetIcon(int level);

    void                DrawServerHitboxes(float duration, bool monoColor);

    float& m_flLastBoneSetupTime();
    void InvalidateBoneCache();
    void ForceBoneCache();
    CStudioHdr* GetStudioHdr();
};