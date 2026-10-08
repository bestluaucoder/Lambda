#pragma once
#include <cstdint>
#include "../Misc/QAngle.h"
#include "../../Utils/VitualFunction.h"

class INetMessage;

class CGlobalVarsBase
{
public:
    float     realtime;                     
    int       framecount;                   
    float     absoluteframetime;            
    float     absoluteframestarttimestddev; 
    float     curtime;                      
    float     frametime;                    
    int       max_clients;                   
    int       tickcount;                    
    float     interval_per_tick;            
    float     interpolation_amount;         
    int       simTicksThisFrame;            
    int       network_protocol;             
    void* pSaveData;                    
    bool      m_bClient;                    
    bool      m_bRemoteClient;              

    inline void store();
    inline void restore();
private:
    
    int       nTimestampNetworkingBase;
    
    
    int       nTimestampRandomizeWindow;

};

inline CGlobalVarsBase s_RestoreGlobalsBase;

inline void CGlobalVarsBase::store() {
    memcpy(&s_RestoreGlobalsBase, this, sizeof(CGlobalVarsBase));
}

inline void CGlobalVarsBase::restore() {
    memcpy(this, &s_RestoreGlobalsBase, sizeof(CGlobalVarsBase));
}

class CClockDriftMgr
{
public:
    float m_ClockOffsets[16];   
    uint32_t m_iCurClockOffset; 
    uint32_t m_nServerTick;     
    uint32_t m_nClientTick;     
};

class INetChannel
{
public:
    byte	pad0[0x14];				
    bool		m_bProcessingMessages;	
    bool		m_bShouldDelete;			
    bool		m_bStopProcessing;		
    byte	pad1[0x1];				
    int			m_nOutSequenceNr;			
    int			m_nInSequenceNr;			
    int			m_nOutSequenceNrAck;		
    int			m_iOutReliableState;		
    int			m_iInReliableState;		
    int			m_nChokedPackets;			
    byte	pad2[0x414];			

    int SendDatagram();
    bool SendNetMsg(void* msg, bool bForceReliable, bool bVoice);
};

class InterfaceReg
{
private:
    using InstantiateInterfaceFn = void* ( * )( );
public:
    InstantiateInterfaceFn m_CreateFn;
    const char* m_pName;
    InterfaceReg* m_pNext;
};

class CClientState
{
public:
    void ForceFullUpdate()
    {
        m_nDeltaTick = -1;
    }

    char pad_0000[156];
    INetChannel* m_NetChannel;
    int m_nChallengeNr;
    char pad_00A4[100];
    int m_nSignonState;
    int signon_pads[2];
    float m_flNextCmdTime;
    int m_nServerCount;
    int m_nCurrentSequence;
    int musor_pads[2];
    CClockDriftMgr m_ClockDriftMgr;
    int m_nDeltaTick;
    bool m_bPaused;
    char paused_align[3];
    int m_nViewEntity;
    int m_nPlayerSlot;
    int bruh;
    char m_szLevelName[260];
    char m_szLevelNameShort[80];
    char m_szGroupName[80];
    char pad_032[92];
    int m_nMaxClients;
    char pad_0314[18828];
    float m_nLastServerTickTime;
    bool m_bInSimulation;
    char pad_4C9D[3];
    int m_nOldTickCount;
    float m_flTickReminder;
    float m_flFrametime;
    int m_nLastOutgoingCommand;
    int m_nChokedCommands;
    int m_nLastCommandAck;
    int m_nPacketEndTickUpdate;
    int m_nCommandAck;
    int m_nSoundSequence;
    char pad_4CCD[76];
    QAngle viewangles;

    int GetChokedCommands() {
        return *(int*)((uintptr_t)this + 0x4D30);
    }
};