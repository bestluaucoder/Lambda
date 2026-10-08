#pragma once
#include "../Misc/UtlVector.h"
#include "../Misc/Vector.h"

class IClientUnknown;

class IClientAlphaProperty
{
public:
	
	virtual IClientUnknown* GetClientUnknown() = 0;

	
	virtual void SetAlphaModulation(unsigned char a) = 0;

	IClientUnknown* m_pOuter;

	unsigned short m_hShadowHandle;
	uint16_t m_nRenderFX : 5;
	uint16_t m_nRenderMode : 4;
	uint16_t m_bAlphaOverride : 1;
	uint16_t m_bShadowAlphaOverride : 1;
	uint16_t m_nDistanceFadeMode : 1;
	uint16_t m_nReserved : 4;

	uint16_t m_nDesyncOffset;
	uint8_t m_nAlpha;
	uint8_t m_nReserved2;

	uint16_t m_nDistFadeStart;
	uint16_t m_nDistFadeEnd;

	float m_flFadeScale;
	float m_flRenderFxStartTime;
	float m_flRenderFxDuration;

	inline int GetAlphaModulation() const { return m_nAlpha; };
};

class CStaticProp
{
public:
	char pad_0000[16]; 
	Vector m_Origin; 
	char pad_001C[24]; 
	uint32_t m_Alpha; 
	char pad_0038[20]; 
	IClientAlphaProperty* m_pClientAlphaProperty; 
	char pad_0050[160]; 
	float m_DiffuseModulation[4]; 
};

class CStaticPropMgr {
	void* __vfptr1; 
	void* __vfptr2; 
	char pad_0008[20]; 
public:
	CUtlVector<CStaticProp> m_StaticProps;
};