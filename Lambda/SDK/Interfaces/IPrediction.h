#pragma once

#include "../Misc/QAngle.h"
#include "../Misc/Vector.h"
#include "../Misc/CUserCmd.h"
#include "../../Utils/VitualFunction.h"
#include "../Misc/CBaseEntity.h"

class CMoveData
{
public:
	bool			bFirstRunOfFunctions : 1;
	bool			bGameCodeMovedPlayer : 1;
	bool			bNoAirControl : 1;
	unsigned long	hPlayerHandle;		
	int				nImpulseCommand;	
	QAngle			angViewAngles;		
	QAngle			angAbsViewAngles;	
	int				nButtons;			
	int				nOldButtons;		
	float			flForwardMove;
	float			flSideMove;
	float			flUpMove;
	float			flMaxSpeed;
	float			flClientMaxSpeed;
	Vector			vecVelocity;		
	Vector			vecTrailingVelocity;
	float			flTrailingVelocityTime;
	Vector			vecAngles;			
	Vector			vecOldAngles;
	float			flOutStepHeight;	
	Vector			vecOutWishVel;		
	Vector			vecOutJumpVel;		
	Vector			vecConstraintCenter;
	float			flConstraintRadius;
	float			flConstraintWidth;
	float			flConstraintSpeedFactor;
	bool			bConstraintPastRadius;
	Vector			vecAbsOrigin;
};

class IPhysicsSurfaceProps;
class CGameTrace;
enum ESoundLevel;
class IMoveHelper
{
public:
	virtual	const char* GetName(void* hEntity) const = 0;
	virtual void				SetHost(CBaseEntity* pHost) = 0;
	virtual void				ResetTouchList() = 0;
	virtual bool				AddToTouched(const CGameTrace& trace, const Vector& vecImpactVelocity) = 0;
	virtual void				ProcessImpacts() = 0;
	virtual void				Con_NPrintf(int nIndex, char const* fmt, ...) = 0;
	virtual void				StartSound(const Vector& vecOrigin, int iChannel, char const* szSample, float flVolume, ESoundLevel soundlevel, int fFlags, int iPitch) = 0;
	virtual void				StartSound(const Vector& vecOrigin, const char* soundname) = 0;
	virtual void				PlaybackEventFull(int fFlags, int nClientIndex, unsigned short uEventIndex, float flDelay, Vector& vecOrigin, Vector& vecAngles, float flParam1, float flParam2, int iParam1, int iParam2, int bParam1, int bParam2) = 0;
	virtual bool				PlayerFallingDamage() = 0;
	virtual void				PlayerSetAnimation(int playerAnimation) = 0;
	virtual IPhysicsSurfaceProps* GetSurfaceProps() = 0;
	virtual bool				IsWorldEntity(const unsigned long& hEntity) = 0;
};

class IGameMovement
{
public:
	virtual						~IGameMovement() { }
	virtual void				ProcessMovement(CBaseEntity* pEntity, CMoveData* pMove) = 0;
	virtual void				Reset() = 0;
	virtual void				StartTrackPredictionErrors(CBaseEntity* pEntity) = 0;
	virtual void				FinishTrackPredictionErrors(CBaseEntity* pEntity) = 0;
	virtual void				DiffPrint(char const* fmt, ...) = 0;
	virtual Vector const& GetPlayerMins(bool bDucked) const = 0;
	virtual Vector const& GetPlayerMaxs(bool bDucked) const = 0;
	virtual Vector const& GetPlayerViewOffset(bool bDucked) const = 0;
	virtual bool				IsMovingPlayerStuck() const = 0;
	virtual CBaseEntity* GetMovingPlayer() const = 0;
	virtual void				UnblockPusher(CBaseEntity* pEntity, CBaseEntity* pPusher) = 0;
	virtual void				SetupMovementBounds(CMoveData* pMove) = 0;
};

class IPrediction
{
public:
	char		    pad0[0x4];						
	unsigned long	hLastGround;					
	bool			bInPrediction;					
	bool			bIsFirstTimePredicted;			
	bool			bEnginePaused;					
	bool			bOldCLPredictValue;				
	int				iPreviousStartFrame;			
	int				nIncomingPacketNumber;			
	float			flLastServerWorldTimeStamp;		

	struct Split_t
	{
		bool		bIsFirstTimePredicted;			
		char    	pad0[0x3];						
		int			nCommandsPredicted;				
		int			nServerCommandsAcknowledged;	
		int			iPreviousAckHadErrors;			
		float		flIdealPitch;					
		int			iLastCommandAcknowledged;		
		bool		bPreviousAckErrorTriggersFullLatchReset; 
		void* vecEntitiesWithPredictionErrorsInLastAck; 
		bool		bPerformedTickShift;			
	};

	Split_t			Split[1];						
	

public:
	void Update(int iStartFrame, bool bValidFrame, int nIncomingAcknowledged, int nOutgoingCommand)
	{
		CallVFunction<void(__thiscall*)(void*, int, bool, int, int)>(this, 3)(this, iStartFrame, bValidFrame, nIncomingAcknowledged, nOutgoingCommand);
	}

	void GetLocalViewAngles(QAngle& angView)
	{
		CallVFunction<void(__thiscall*)(void*, QAngle&)>(this, 12)(this, angView);
	}

	void SetLocalViewAngles(QAngle& angView)
	{
		CallVFunction<void(__thiscall*)(void*, QAngle&)>(this, 13)(this, angView);
	}

	void CheckMovingGround(CBaseEntity* pEntity, double dbFrametime)
	{
		CallVFunction<void(__thiscall*)(void*, CBaseEntity*, double)>(this, 18)(this, pEntity, dbFrametime);
	}

	void SetupMove(CBaseEntity* pEntity, CUserCmd* pCmd, IMoveHelper* pHelper, CMoveData* pMoveData)
	{
		CallVFunction<void(__thiscall*)(void*, CBaseEntity*, CUserCmd*, IMoveHelper*, CMoveData*)>(this, 20)(this, pEntity, pCmd, pHelper, pMoveData);
	}

	void FinishMove(CBaseEntity * pEntity, CUserCmd * pCmd, CMoveData * pMoveData)
	{
		CallVFunction<void(__thiscall*)(void*, CBaseEntity*, CUserCmd*, CMoveData*)>(this, 21)(this, pEntity, pCmd, pMoveData);
	}
};