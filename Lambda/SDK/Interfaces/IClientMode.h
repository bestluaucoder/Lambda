#pragma once

#include "../Misc/Vector.h"
#include "../Misc/QAngle.h"
#include "../Misc/CUserCmd.h"

class IPanel;
class C_BaseEntity;

enum class ClearFlags_t
{
    VIEW_CLEAR_COLOR = 0x1,
    VIEW_CLEAR_DEPTH = 0x2,
    VIEW_CLEAR_FULL_TARGET = 0x4,
    VIEW_NO_DRAW = 0x8,
    VIEW_CLEAR_OBEY_STENCIL = 0x10,
    VIEW_CLEAR_STENCIL = 0x20,
};


enum class MotionBlurMode_t
{
    MOTION_BLUR_DISABLE = 1,
    MOTION_BLUR_GAME = 2,
    MOTION_BLUR_SFM = 3
};

class CViewSetup
{
public:
    int   x;                      
    int   x_old;                  
    int   y;                      
    int   y_old;                  
    int   width;                  
    int   width_old;              
    int   height;                 
    int   height_old;             
    char      pad_0x0020[0x90];   
    float     fov;                
    float     viewmodel_fov;      
    Vector    origin;             
    QAngle    angles;             
    char      pad_0x00D0[0x7C];   

};

class IClientMode
{
public:
    virtual             ~IClientMode() {}
    virtual int         ClientModeCSNormal(void*) = 0;
    virtual void        Init() = 0;
    virtual void        InitViewport() = 0;
    virtual void        Shutdown() = 0;
    virtual void        Enable() = 0;
    virtual void        Disable() = 0;
    virtual void        Layout() = 0;
    virtual IPanel* GetViewport() = 0;
    virtual void* GetViewportAnimationController() = 0;
    virtual void        ProcessInput(bool bActive) = 0;
    virtual bool        ShouldDrawDetailObjects() = 0;
    virtual bool        ShouldDrawEntity(C_BaseEntity* pEnt) = 0;
    virtual bool        ShouldDrawLocalPlayer(C_BaseEntity* pPlayer) = 0;
    virtual bool        ShouldDrawParticles() = 0;
    virtual bool        ShouldDrawFog(void) = 0;
    virtual void        OverrideView(CViewSetup* pSetup) = 0;
    virtual int         KeyInput(int down, int keynum, const char* pszCurrentBinding) = 0;
    virtual void        StartMessageMode(int iMessageModeType) = 0;
    virtual IPanel* GetMessagePanel() = 0;
    virtual void        OverrideMouseInput(float* x, float* y) = 0;
    virtual bool        CreateMove(float flInputSampleTime, CUserCmd* usercmd) = 0;
    virtual void        LevelInit(const char* newmap) = 0;
    virtual void        LevelShutdown(void) = 0;
};