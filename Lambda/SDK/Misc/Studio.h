#pragma once
#include <string>

#include "Vector.h"
#include "Matrix.h"
#include "../Interfaces/IEngineTrace.h"

#define MAXSTUDIOBONES		128		

#define BONE_CALCULATE_MASK			0x1F
#define BONE_PHYSICALLY_SIMULATED	0x01	
#define BONE_PHYSICS_PROCEDURAL		0x02	
#define BONE_ALWAYS_PROCEDURAL		0x04	
#define BONE_SCREEN_ALIGN_SPHERE	0x08	
#define BONE_SCREEN_ALIGN_CYLINDER	0x10	
#define BONE_WORLD_ALIGN			0x20	

#define BONE_USED_MASK				0x000FFF00
#define BONE_USED_BY_ANYTHING		0x000FFF00
#define BONE_USED_BY_HITBOX			0x00000100	
#define BONE_USED_BY_ATTACHMENT		0x00000200	
#define BONE_USED_BY_VERTEX_MASK	0x0003FC00
#define BONE_USED_BY_VERTEX_LOD0	0x00000400	
#define BONE_USED_BY_VERTEX_LOD1	0x00000800	
#define BONE_USED_BY_VERTEX_LOD2	0x00001000  
#define BONE_USED_BY_VERTEX_LOD3	0x00002000
#define BONE_USED_BY_VERTEX_LOD4	0x00004000
#define BONE_USED_BY_VERTEX_LOD5	0x00008000
#define BONE_USED_BY_VERTEX_LOD6	0x00010000
#define BONE_USED_BY_VERTEX_LOD7	0x00020000
#define BONE_USED_BY_BONE_MERGE		0x00040000	
#define BONE_ALWAYS_SETUP			0x00080000

#define BONE_USED_BY_VERTEX_AT_LOD(lod) ( BONE_USED_BY_VERTEX_LOD0 << (lod) )
#define BONE_USED_BY_ANYTHING_AT_LOD(lod) ( ( BONE_USED_BY_ANYTHING & ~BONE_USED_BY_VERTEX_MASK ) | BONE_USED_BY_VERTEX_AT_LOD(lod) )

#define MAX_NUM_LODS 8

#define BONE_TYPE_MASK				0x00F00000
#define BONE_FIXED_ALIGNMENT		0x00100000	

#define BONE_HAS_SAVEFRAME_POS		0x00200000	
#define BONE_HAS_SAVEFRAME_ROT64	0x00400000	
#define BONE_HAS_SAVEFRAME_ROT32	0x00800000	

typedef unsigned long MDLHandle_t;

enum EHitboxes : int
{
    HITBOX_HEAD,
    HITBOX_NECK,
    HITBOX_PELVIS,
    HITBOX_STOMACH,
    HITBOX_LOWER_CHEST,
    HITBOX_CHEST,
    HITBOX_UPPER_CHEST,
    HITBOX_RIGHT_THIGH,
    HITBOX_LEFT_THIGH,
    HITBOX_RIGHT_CALF,
    HITBOX_LEFT_CALF,
    HITBOX_RIGHT_FOOT,
    HITBOX_LEFT_FOOT,
    HITBOX_RIGHT_HAND,
    HITBOX_LEFT_HAND,
    HITBOX_RIGHT_UPPER_ARM,
    HITBOX_RIGHT_FOREARM,
    HITBOX_LEFT_UPPER_ARM,
    HITBOX_LEFT_FOREARM,
    HITBOX_MAX
};

inline std::string GetHitboxName(int hb) {
    static std::string hb_to_s[]{
        "head",
        "neck",
        "pelvis",
        "stomach",
        "lower chest",
        "chest",
        "upper chest",
        "leg",
        "leg",
        "leg",
        "leg",
        "foot",
        "foot",
        "hand",
        "hand",
        "arm",
        "arm",
        "arm",
        "arm"
    };
    
    return hb_to_s[hb];
}

inline int HitboxToHitgroup(int hb) {
    switch (hb)
    {
    case HITBOX_HEAD:
        return HITGROUP_HEAD;
    case HITBOX_NECK:
        return HITGROUP_NECK;
    case HITBOX_PELVIS:
    case HITBOX_STOMACH:
        return HITGROUP_STOMACH;
    case HITBOX_LOWER_CHEST:
    case HITBOX_CHEST:
    case HITBOX_UPPER_CHEST:
        return HITGROUP_CHEST;
    case HITBOX_RIGHT_THIGH:
    case HITBOX_RIGHT_CALF:
    case HITBOX_RIGHT_FOOT:
        return HITGROUP_RIGHTLEG;
    case HITBOX_LEFT_THIGH:
    case HITBOX_LEFT_CALF:
    case HITBOX_LEFT_FOOT:
        return HITGROUP_LEFTLEG;
    case HITBOX_RIGHT_HAND:
    case HITBOX_RIGHT_UPPER_ARM:
    case HITBOX_RIGHT_FOREARM:
        return HITGROUP_RIGHTARM;
    case HITBOX_LEFT_HAND:
    case HITBOX_LEFT_UPPER_ARM:
    case HITBOX_LEFT_FOREARM:
        return HITGROUP_LEFTARM;
    default:
        return HITGROUP_GENERIC;
    }
}

enum EDamageGroup {
    DAMAGEGROUP_HEAD,
    DAMAGEGROUP_CHEST,
    DAMAGEGROUP_STOMACH,
    DAMAGEGROUP_ARM,
    DAMAGEGROUP_LEG,
};

inline int HitboxToDamagegroup(int hitbox) {
    switch (hitbox) {
    case HITBOX_HEAD:
        return DAMAGEGROUP_HEAD;
    case HITBOX_STOMACH:
    case HITBOX_PELVIS:
        return DAMAGEGROUP_STOMACH;
    case HITBOX_LEFT_CALF:
    case HITBOX_LEFT_FOOT:
    case HITBOX_LEFT_THIGH:
    case HITBOX_RIGHT_CALF:
    case HITBOX_RIGHT_FOOT:
    case HITBOX_RIGHT_THIGH:
        return DAMAGEGROUP_LEG;
    case HITBOX_LEFT_FOREARM:
    case HITBOX_LEFT_UPPER_ARM:
    case HITBOX_LEFT_HAND:
    case HITBOX_RIGHT_FOREARM:
    case HITBOX_RIGHT_UPPER_ARM:
    case HITBOX_RIGHT_HAND:
        return DAMAGEGROUP_ARM;
    }

    return DAMAGEGROUP_CHEST;
}

inline int HitgroupToDamagegroup(int hitgroup) {
    switch (hitgroup) {
    case HITGROUP_HEAD:
        return DAMAGEGROUP_HEAD;
    case HITGROUP_STOMACH:
        return DAMAGEGROUP_STOMACH;
    case HITGROUP_LEFTLEG:
    case HITGROUP_RIGHTLEG:
        return DAMAGEGROUP_LEG;
    case HITGROUP_CHEST:
        return DAMAGEGROUP_CHEST;
    }

    return DAMAGEGROUP_ARM;
}

inline std::string GetDamagegroupName(int damagegroup) {
    switch (damagegroup) {
    case DAMAGEGROUP_HEAD:
        return "head";
    case DAMAGEGROUP_CHEST:
        return "chest";
    case DAMAGEGROUP_STOMACH:
        return "stomach";
    case DAMAGEGROUP_ARM:
        return "arm";
    }

    return "leg";
}

struct mstudiobbox_t
{
    int		bone;
    int		group;
    Vector	bbmin;
    Vector	bbmax;
    int		szhitboxnameindex;
    QAngle	angOffsetOrientation;
    float	flCapsuleRadius;
    std::byte	pad0[0x10];

    const char* GetName()
    {
        if (!szhitboxnameindex) return nullptr;
        return (const char*)((uint8_t*)this + szhitboxnameindex);
    }
};
struct mstudiohitboxset_t
{
    int    sznameindex;
    int    numhitboxes;
    int    hitboxindex;

    const char* GetName()
    {
        if (!sznameindex) return nullptr;
        return (const char*)((uint8_t*)this + sznameindex);
    }

    mstudiobbox_t* GetHitbox(int i)
    {
        if (i > numhitboxes) return nullptr;
        return (mstudiobbox_t*)((uint8_t*)this + hitboxindex) + i;
    }
};
typedef float Quaternion[4];
typedef float RadianEuler[3];
struct mstudiobone_t
{
    int sznameindex;
    inline char* const pszName(void) const { return ((char*)this) + sznameindex; }
    int parent;
    int bonecontroller[6];

    Vector pos;
    Quaternion quat;
    RadianEuler rot;

    Vector posscale;
    Vector rotscale;

    matrix3x4_t poseToBone;
    Quaternion qAlignment;
    int flags;
};

class studiohdr_t
{
public:
    __int32 id;                     
    __int32 version;                
    long    checksum;               
    char    szName[64];             
    __int32 length;                 
    Vector  vecEyePos;              
    Vector  vecIllumPos;            
    Vector  vecHullMin;             
    Vector  vecHullMax;             
    Vector  vecBBMin;               
    Vector  vecBBMax;               
    __int32 flags;                  
    __int32 numbones;               
    __int32 boneindex;              
    __int32 numbonecontrollers;     
    __int32 bonecontrollerindex;    
    __int32 numhitboxsets;          
    __int32 hitboxsetindex;         
    __int32 numlocalanim;           
    __int32 localanimindex;         
    __int32 numlocalseq;            
    __int32 localseqindex;          
    __int32 activitylistversion;    
    __int32 eventsindexed;          
    __int32 numtextures;            
    __int32 textureindex;           

    mstudiohitboxset_t* GetHitboxSet(int i)
    {
        if (i > numhitboxsets) return nullptr;
        return (mstudiohitboxset_t*)((uint8_t*)this + hitboxsetindex) + i;
    }
    mstudiobone_t* GetBone(int i)
    {
        if (i > numbones) return nullptr;
        return (mstudiobone_t*)((uint8_t*)this + boneindex) + i;
    }

};

struct model_t {
    void* fnHandle;               
    char    szName[260];            
    __int32 nLoadFlags;             
    __int32 nServerCount;           
    __int32 type;                   
    __int32 flags;                  
    Vector  vecMins;                
    Vector  vecMaxs;                
    float   radius;                 
    void* m_pKeyValues;
    union
    {
        void* brush;
        MDLHandle_t studio;
        void* sprite;
    };
};