#pragma once
#include "../Misc/Color.h"
#include "../Misc/UtlVector.h"




class ConVar;
class CCommand;

#define FCVAR_NONE                0 

#define FCVAR_UNREGISTERED              (1<<0)  
#define FCVAR_DEVELOPMENTONLY           (1<<1)  
#define FCVAR_GAMEDLL                   (1<<2)  
#define FCVAR_CLIENTDLL                 (1<<3)  
#define FCVAR_HIDDEN                    (1<<4)  
                              
#define FCVAR_PROTECTED                 (1<<5)  
#define FCVAR_SPONLY                    (1<<6)  
#define FCVAR_ARCHIVE                   (1<<7)  
#define FCVAR_NOTIFY                    (1<<8)  
#define FCVAR_USERINFO                  (1<<9)  

#define FCVAR_PRINTABLEONLY             (1<<10) 
#define FCVAR_UNLOGGED                  (1<<11) 
#define FCVAR_NEVER_AS_STRING           (1<<12) 
#define FCVAR_REPLICATED                (1<<13) 
#define FCVAR_CHEAT                     (1<<14) 
#define FCVAR_SS                        (1<<15) 
#define FCVAR_DEMO                      (1<<16) 
#define FCVAR_DONTRECORD                (1<<17) 
#define FCVAR_SS_ADDED                  (1<<18) 
#define FCVAR_RELEASE                   (1<<19) 
#define FCVAR_RELOAD_MATERIALS          (1<<20) 
#define FCVAR_RELOAD_TEXTURES           (1<<21) 
#define FCVAR_NOT_CONNECTED             (1<<22) 
#define FCVAR_MATERIAL_SYSTEM_THREAD    (1<<23) 
#define FCVAR_ARCHIVE_XBOX              (1<<24) 
#define FCVAR_ACCESSIBLE_FROM_THREADS   (1<<25) 


#define FCVAR_SERVER_CAN_EXECUTE        (1<<28) 
#define FCVAR_SERVER_CANNOT_QUERY       (1<<29) 
#define FCVAR_CLIENTCMD_CAN_EXECUTE     (1<<30) 
#define FCVAR_MEME_DLL                  (1<<31)

#define FCVAR_MATERIAL_THREAD_MASK ( FCVAR_RELOAD_MATERIALS | FCVAR_RELOAD_TEXTURES | FCVAR_MATERIAL_SYSTEM_THREAD )    





typedef void(*FnChangeCallback_t)(ConVar* var, const char* pOldValue, float flOldValue);


class ConVar {
public:
	struct CVValue_t
	{
		char*	m_pszString;
		int		m_StringLength;
		float	m_fValue;
		int		m_nValue;
	};

	ConVar*			m_pNext;
	int				m_nOriginalFlags;
	const char*		m_pszName;
	const char*		m_pszHelpString;
	int				m_nFlags;
private:
	void* __vfptr_ConVar;
public:
	ConVar*			m_pParent;
	const char*		m_pszDefaultValue;
	CVValue_t		m_Value;
	CVValue_t		m_OriginalValue;
	bool			m_bHasMin;
	float			m_fMinVal;
	bool			m_bHasMax;
	float			m_fMaxVal;
	CUtlVector<FnChangeCallback_t> m_pCallbacks;

	virtual void null0() = 0;
	virtual void null1() = 0;
	virtual void null2() = 0;
	virtual void null3() = 0;
	virtual void null4() = 0;
	virtual const char* GetName() = 0;
private:
	virtual void null6() = 0;
	virtual void null7() = 0;
	virtual void null8() = 0;
	virtual void null9() = 0;
	virtual void null10() = 0;
	virtual void null11() = 0;
public:
	virtual float GetFloat() = 0;
	virtual int GetInt() = 0;
	virtual void SetString(const char* pValue) = 0;
	virtual void SetFloat(float flValue) = 0;
	virtual void SetInt(int nValue) = 0;

	void SetFlag(int flag) {
		m_nFlags |= flag;
	}

	void RemoveFlag(int flag) {
		m_nFlags &= ~flag;
	}

	int GetFlags() const {
		return m_nFlags;
	}

	bool IsFlagSet(int flag) const {
		return m_nFlags & flag;
	}

	void RemoveCallbacks() {
		m_pCallbacks.RemoveAll();
	}

	std::string GetString() const {
		return m_Value.m_pszString;
	}
};