#pragma once

#include "../Misc/CUserCmd.h"
#include "../Misc/checksum_crc.h"
#include "../../Utils/VitualFunction.h"

#define MULTIPLAYER_BACKUP 150

class bf_write;
class bf_read;

class CVerifiedUserCmd
{
public:
    CUserCmd cmd;
    CRC32_t  crc;
};

class CInput
{
public:
    

    char pad_0000[12]; 
    bool m_fTrackIRAvailable; 
    bool m_fMouseInitialized; 
    bool m_fMouseActive; 
    bool m_fJoystickAdvancedInit; 
    char pad_0010[44]; 
    char* m_pKeys; 
    char pad_0040[48]; 
    int32_t m_nCamCommand; 
    char pad_0074[52]; 
    bool m_fCameraInterceptingMouse; 
    bool m_fCameraInThirdPerson; 
    bool m_fCameraMovingWithMouse; 
    Vector m_vecCameraOffset; 
    bool m_fCameraDistanceMove; 
    char pad_00D1[19]; 
    bool m_CameraIsOrthographic; 
    bool m_CameraIsThirdPersonOverview; 
    char pad_00E6[2]; 
    QAngle* m_angPreviousViewAngles; 
    QAngle* m_angPreviousViewAnglesTilt; 
    char pad_00F0[16]; 
    float m_flLastForwardMove; 
    int32_t m_nClearInputState; 
	CUserCmd* pCommands;				
	CVerifiedUserCmd* pVerifiedCommands;

	inline CUserCmd* GetUserCmd(int sequence_number);
	inline CVerifiedUserCmd* GetVerifiedCmd(int sequence_number);
};

CUserCmd* CInput::GetUserCmd(int sequence_number)
{
    auto cmds = *(CUserCmd**)(reinterpret_cast<uint32_t>(this) + 0xF0);
    return &cmds[sequence_number % MULTIPLAYER_BACKUP];
}

CVerifiedUserCmd* CInput::GetVerifiedCmd(int sequence_number)
{
	auto verifiedCommands = *(CVerifiedUserCmd**)(reinterpret_cast<uint32_t>(this) + 0xF4);
	return &verifiedCommands[sequence_number % MULTIPLAYER_BACKUP];
}