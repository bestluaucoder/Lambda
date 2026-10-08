






#pragma once

#include <malloc.h>
#include <limits.h>
#include <float.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <new.h>

#define COMPILER_MSVC














#ifdef _WIN32
#define IsPlatformLinux()	false
#define IsPlatformPosix()	false
#define IsPlatformOSX()		false
#define IsPlatformPS3()		false
#define IsPlatformWindows() true
#ifndef PLATFORM_WINDOWS
#define PLATFORM_WINDOWS	1
#endif

#ifndef _X360
#define IsPlatformX360() false
#define IsPlatformWindowsPC() true
#define PLATFORM_WINDOWS_PC 1

#ifdef _WIN64
#define IsPlatformWindowsPC64() true
#define IsPlatformWindowsPC32() false
#define PLATFORM_WINDOWS_PC64 1
#else
#define IsPlatformWindowsPC64() false
#define IsPlatformWindowsPC32() true
#define PLATFORM_WINDOWS_PC32 1
#endif

#else 

#define IsPlatformWindowsPC()	false
#define IsPlatformWindowsPC64() false
#define IsPlatformWindowsPC32() false
#define IsPlatformX360()		true
#define PLATFORM_X360 1

#endif 

#elif defined(POSIX)
#define IsPlatformX360()		false
#define IsPlatformPS3()			false
#define IsPlatformWindows()		false
#define IsPlatformWindowsPC()	false
#define IsPlatformWindowsPC64()	false
#define IsPlatformWindowsPC32()	false
#define IsPlatformPosix()		true
#ifndef PLATFORM_POSIX
#define PLATFORM_POSIX 1
#endif

#if defined( LINUX )
#define IsPlatformLinux() true
#define IsPlatformOSX() false
#ifndef PLATFORM_LINUX
#define PLATFORM_LINUX 1
#endif
#elif defined ( OSX )
#define IsPlatformLinux() false
#define IsPlatformOSX() true
#ifndef PLATFORM_OSX
#define PLATFORM_OSX 1
#endif
#else
#define IsPlatformLinux() false
#define IsPlatformOSX() false
#endif

#else
#error
#endif




#ifdef PLATFORM_X360
#ifndef _CONSOLE
#define _CONSOLE
#endif
#define IsPC()		false
#define IsConsole() true
#else
#define IsPC()		true
#define IsConsole() false
#endif






#ifdef _CERT
#define IsCert() true
#else
#define IsCert() false
#endif

#ifdef _DEBUG
#define IsRelease() false
#define IsDebug() true
#else
#define IsRelease() true
#define IsDebug() false
#endif

#ifdef _RETAIL
#define IsRetail() true
#else
#define IsRetail() false
#endif


#if !defined(PLATFORM_OSX) && !defined(__STDC_LIMIT_MACROS)

#ifndef INT8_MAX
#define  INT8_MAX			SCHAR_MAX
#endif
#ifndef INT16_MAX
#define  INT16_MAX			SHRT_MAX
#endif
#ifndef INT32_MAX
#define  INT32_MAX			LONG_MAX
#endif
#ifndef INT64_MAX
#define  INT64_MAX			(((int64_t)~0) >> 1)
#endif

#ifndef INT8_MIN
#define  INT8_MIN			SCHAR_MIN
#endif
#ifndef INT16_MIN
#define  INT16_MIN			SHRT_MIN
#endif
#ifndef INT32_MIN
#define  INT32_MIN			LONG_MIN
#endif
#ifndef INT64_MIN
#define  INT64_MIN			(((int64_t)1) << 63)
#endif

#ifndef UINT8_MAX
#define  UINT8_MAX			((uint8_t)~0)
#endif
#ifndef UINT16_MAX
#define  UINT16_MAX			((uint16)~0)
#endif
#ifndef UINT32_MAX
#define  UINT32_MAX			((uint32_t)~0)
#endif
#ifndef UINT16_MAX
#define  UINT64_MAX			((uint64_t)~0)
#endif

#ifndef UINT8_MIN
#define  UINT8_MIN			0
#endif
#ifndef UINT16_MIN
#define  UINT16_MIN			0
#endif
#ifndef UINT32_MIN
#define  UINT32_MIN			0
#endif
#ifndef UINT64_MIN
#define  UINT64_MIN			0
#endif

#endif 

#ifndef  UINT_MIN
#define  UINT_MIN			UINT32_MIN
#endif

#define  FLOAT32_MAX		FLT_MAX
#define  FLOAT64_MAX		DBL_MAX

#ifdef GNUC
#undef offsetof

#define offsetof(s,m)	(size_t)&(((s *)0)->m)
#else
#include <stddef.h>
#undef offsetof
#define offsetof(s,m)	(size_t)&(((s *)0)->m)
#endif


#define  FLOAT32_MIN		FLT_MIN
#define  FLOAT64_MIN		DBL_MIN








#ifdef DISALLOW_USE_OF_LONG
#define long			long_is_the_devil_stop_using_it_use_int32_or_int64
#endif





#ifdef COMPILER_MSVC

#ifdef FORCEINLINE
#undef FORCEINLINE
#endif
#define STDCALL					__stdcall
#ifndef FASTCALL
#define  FASTCALL			__fastcall
#endif
#define FORCEINLINE				__forceinline
#define FORCEINLINE_TEMPLATE	__forceinline
#define NULLTERMINATED			__nullterminated



#define SINGLE_INHERITANCE		__single_inheritance
#define MULTIPLE_INHERITANCE	__multiple_inheritance
#define EXPLICIT				explicit
#define NO_VTABLE				__declspec( novtable )


#define TEMPLATE_STATIC			static


#define DLL_EXPORT				extern "C" __declspec( dllexport )
#define DLL_IMPORT				extern "C" __declspec( dllimport )


#define DLL_CLASS_EXPORT		__declspec( dllexport )
#define DLL_CLASS_IMPORT		__declspec( dllimport )


#define DLL_GLOBAL_EXPORT		extern __declspec( dllexport )
#define DLL_GLOBAL_IMPORT		extern __declspec( dllimport )









#define HINT(THE_HINT)			__assume((THE_HINT))


#define DECL_ALIGN(x)			__declspec( align( x ) )




#define CONSTRUCT_EARLY

#define SELECTANY				__declspec(selectany)

#define RESTRICT				__restrict
#define RESTRICT_FUNC			__declspec(restrict)
#define FMTFUNCTION( a, b )
#define NOINLINE

#if !defined( NO_THREAD_LOCAL )
#define DECL_THREAD_LOCAL		__declspec(thread)
#endif 

#define DISABLE_VC_WARNING( x ) __pragma(warning(disable:4310) )
#define DEFAULT_VC_WARNING( x ) __pragma(warning(default:4310) )


#elif defined ( COMPILER_GCC )

#if (CROSS_PLATFORM_VERSION >= 1) && !defined( PLATFORM_64BITS )
#define  STDCALL			__attribute__ ((__stdcall__))
#else
#define  STDCALL
#define  __stdcall			__attribute__ ((__stdcall__))
#endif

#define  FASTCALL
#ifdef _LINUX_DEBUGGABLE
#define  FORCEINLINE
#else
#define  FORCEINLINE		inline
#endif



#define FORCEINLINE_TEMPLATE	inline
#define SINGLE_INHERITANCE
#define MULTIPLE_INHERITANCE
#define EXPLICIT
#define NO_VTABLE

#define NULLTERMINATED			

#define TEMPLATE_STATIC


#define DLL_EXPORT				extern "C" __attribute__ ((visibility("default"))
#define DLL_IMPORT				extern "C"


#define DLL_CLASS_EXPORT		__attribute__ ((visibility("default"))
#define DLL_CLASS_IMPORT


#define DLL_GLOBAL_EXPORT		__attribute__((visibility("default"))
#define DLL_GLOBAL_IMPORT		extern

#define HINT(THE_HINT)			0
#define DECL_ALIGN(x)			__attribute__( ( aligned( x ) ) )
#define CONSTRUCT_EARLY			__attribute__((init_priority(101)))
#define SELECTANY				__attribute__((weak))
#define RESTRICT
#define RESTRICT_FUNC
#define FMTFUNCTION( fmtargnumber, firstvarargnumber ) __attribute__ (( format( printf, fmtargnumber, firstvarargnumber )))
#define NOINLINE				__attribute__ ((noinline))

#if !defined( NO_THREAD_LOCAL )
#define DECL_THREAD_LOCAL		__thread
#endif

#define DISABLE_VC_WARNING( x )
#define DEFAULT_VC_WARNING( x )

#else

#define DECL_ALIGN(x)			
#define SELECTANY				static

#endif

#if defined( GNUC )

#define ALIGN4
#define ALIGN8 
#define ALIGN16
#define ALIGN32
#define ALIGN128

#define ALIGN4_POST DECL_ALIGN(4)
#define ALIGN8_POST DECL_ALIGN(8)
#define ALIGN16_POST DECL_ALIGN(16)
#define ALIGN32_POST DECL_ALIGN(32)
#define ALIGN128_POST DECL_ALIGN(128)
#else

#define ALIGN4 DECL_ALIGN(4)
#define ALIGN8 DECL_ALIGN(8)
#define ALIGN16 DECL_ALIGN(16)
#define ALIGN32 DECL_ALIGN(32)
#define ALIGN128 DECL_ALIGN(128)

#define ALIGN4_POST
#define ALIGN8_POST
#define ALIGN16_POST
#define ALIGN32_POST
#define ALIGN128_POST
#endif

















#ifndef COMPILER_MSVCX360
#define abstract_class class
#else
#define abstract_class class NO_VTABLE
#endif






#if defined( COMPILER_MSVC ) && !defined( WINDED )
#if defined(_M_IX86)
#define __i386__	1
#endif
#endif

#if defined __i386__ && !defined __linux__
#define id386	1
#else
#define id386	0
#endif  





#ifdef COMPILER_MSVC

#pragma warning(disable : 4514) 
#pragma warning(disable : 4100) 
#pragma warning(disable : 4127) 
#pragma warning(disable : 4512) 
#pragma warning(disable : 4611) 
#pragma warning(disable : 4710) 
#pragma warning(disable : 4702) 
#pragma warning(disable : 4505) 
#pragma warning(disable : 4239) 
#pragma warning(disable : 4097) 
#pragma warning(disable : 4324) 
#pragma warning(disable : 4244) 
#pragma warning(disable : 4305)	
#pragma warning(disable : 4786)	
#pragma warning(disable : 4250) 
#pragma warning(disable : 4201) 

#if _MSC_VER >= 1300
#pragma warning(disable : 4511)	
#pragma warning(disable : 4121)	
#pragma warning(disable : 4530)	
#endif

#if _MSC_VER >= 1400
#pragma warning(disable : 4996)	
#endif


#if !defined( COMPILER_MSVC64 )
#if ( CROSS_PLATFORM_VERSION < 1 )
#pragma warning( disable : 4267 )	
#pragma warning( disable : 4311 )	
#pragma warning( disable : 4312 )	
#endif
#endif

#endif





#if defined( COMPILER_GCC )

#define stackalloc( _size )		alloca( ALIGN_VALUE( _size, 16 ) )

#ifdef PLATFORM_OSX
#define mallocsize( _p )	( malloc_size( _p ) )
#else
#define mallocsize( _p )	( malloc_usable_size( _p ) )
#endif

#elif defined ( COMPILER_MSVC )

#define stackalloc( _size )		_alloca( ALIGN_VALUE( _size, 16 ) )
#define mallocsize( _p )		( _msize( _p ) )

#endif

#define  stackfree( _p )			0





#ifdef COMPILER_MSVC64
#define DebuggerBreak()		__debugbreak()
#elif COMPILER_MSVC32
#define DebuggerBreak()		__asm { int 3 }
#elif COMPILER_MSVCX360
#define DebuggerBreak()		DebugBreak()
#elif COMPILER_GCC 
#if defined( PLATFORM_CYGWIN ) || defined( PLATFORM_POSIX )
#define DebuggerBreak()		__asm__( "int $0x3;")
#else
#define DebuggerBreak()		asm( "int3" )
#endif
#endif





#ifndef STATIC_TIER0

#ifdef TIER0_DLL_EXPORT
#define PLATFORM_INTERFACE	DLL_EXPORT
#define PLATFORM_OVERLOAD	DLL_GLOBAL_EXPORT
#define PLATFORM_CLASS		DLL_CLASS_EXPORT
#else
#define PLATFORM_INTERFACE	DLL_IMPORT
#define PLATFORM_OVERLOAD	DLL_GLOBAL_IMPORT
#define PLATFORM_CLASS		DLL_CLASS_IMPORT
#endif

#else	

#define PLATFORM_INTERFACE	extern
#define PLATFORM_OVERLOAD
#define PLATFORM_CLASS

#endif	




#ifdef PLATFORM_POSIX


#define _strupr strupr
#define _getcwd getcwd
#define _open open
#define _lseek lseek
#define _read read
#define _close close
#define _vsnprintf vsnprintf
#define _stat stat
#define _O_RDONLY O_RDONLY
#define _stricmp strcasecmp
#define _finite finite
#define _unlink unlink
#define _putenv putenv
#define _chdir chdir
#define _access access
#define _strtoi64 strtoll

#if !defined( _snprintf )	
#define _snprintf snprintf
#endif

#include <alloca.h>
#include <unistd.h>											
#include <errno.h>

#endif 







#define ALIGN_VALUE( val, alignment ) ( ( val + alignment - 1 ) & ~( alignment - 1 ) ) 


#define DONT_INLINE(a) (((int)(a)+1)?(a):(a))



#define UNREACHABLE() { Assert(0); HINT(0); }



#define NO_DEFAULT default: UNREACHABLE();


#ifndef MAX_PATH
#define MAX_PATH  260
#endif








#if defined( COMPILER_MSVC64 )

inline void SetupFPUControlWord()
{
}

#elif defined ( COMPILER_MSVC32 )

inline void SetupFPUControlWordForceExceptions()
{
    
    uint16 tmpCtrlW;
    __asm
    {
        fnclex						
        fnstcw word ptr[tmpCtrlW]	
        and [tmpCtrlW], 0FCC0h		
        or [tmpCtrlW], 0230h		
        fldcw word ptr[tmpCtrlW]	
    }
}

#ifdef CHECK_FLOAT_EXCEPTIONS

inline void SetupFPUControlWord()
{
    SetupFPUControlWordForceExceptions();
}

#else

inline void SetupFPUControlWord()
{
    
    uint16 tmpCtrlW;
    __asm
    {
        fnstcw word ptr[tmpCtrlW]	
        and [tmpCtrlW], 0FCC0h		
        or [tmpCtrlW], 023Fh		
        fldcw word ptr[tmpCtrlW]	
    }
}

#endif

#elif defined ( COMPILER_GCC )

inline void SetupFPUControlWord()
{
    __volatile unsigned short int __cw;
    __asm __volatile("fnstcw %0" : "=m" (__cw));
    __cw = __cw & 0x0FCC0;	
    __cw = __cw | 0x023F;	
    __asm __volatile("fldcw %0" : : "m" (__cw));
}

#elif defined( COMPILER_MSVCX360 )

#ifdef CHECK_FPU_CONTROL_WORD_SET
FORCEINLINE bool IsFPUControlWordSet()
{
    float f = 0.996f;
    union
    {
        double flResult;
        int pResult[2];
    };
    flResult = __fctiw(f);
    return (pResult[1] == 1);
}
#else
#define IsFPUControlWordSet() true
#endif

inline void SetupFPUControlWord()
{
    
    
    __emit(0xFF80010C);	

                        
                        
    __vector4  a = { 0.0f, 0.0f, 0.0f, 0.0f };
    a;				
    __asm
    {
        mtvscr a;	
    }
}

#endif 










template <typename T>
inline T WordSwapC(T w)
{
    uint16_t temp;

    temp = ((*((uint16_t *)&w) & 0xff00) >> 8);
    temp |= ((*((uint16_t *)&w) & 0x00ff) << 8);

    return *((T*)&temp);
}

template <typename T>
inline T DWordSwapC(T dw)
{
    uint32_t temp;

    temp = *((uint32_t *)&dw) >> 24;
    temp |= ((*((uint32_t *)&dw) & 0x00FF0000) >> 8);
    temp |= ((*((uint32_t *)&dw) & 0x0000FF00) << 8);
    temp |= ((*((uint32_t *)&dw) & 0x000000FF) << 24);

    return *((T*)&temp);
}





#if defined( COMPILER_MSVCX360 )

#define WordSwap  WordSwap360Intr
#define DWordSwap DWordSwap360Intr

template <typename T>
inline T WordSwap360Intr(T w)
{
    T output;
    __storeshortbytereverse(w, 0, &output);
    return output;
}

template <typename T>
inline T DWordSwap360Intr(T dw)
{
    T output;
    __storewordbytereverse(dw, 0, &output);
    return output;
}

#elif defined( COMPILER_MSVC32 )

#define WordSwap  WordSwapAsm
#define DWordSwap DWordSwapAsm

#pragma warning(push)
#pragma warning (disable:4035) 

template <typename T>
inline T WordSwapAsm(T w)
{
    __asm
    {
        mov ax, w
        xchg al, ah
    }
}

template <typename T>
inline T DWordSwapAsm(T dw)
{
    __asm
    {
        mov eax, dw
        bswap eax
    }
}

#pragma warning(pop)

#else

#define WordSwap  WordSwapC
#define DWordSwap DWordSwapC

#endif





#if defined( _SGI_SOURCE ) || defined( PLATFORM_X360 )
#define	PLAT_BIG_ENDIAN 1
#else
#define PLAT_LITTLE_ENDIAN 1
#endif




#define SafeSwapFloat( pOut, pIn )	(*((unsigned int*)pOut) = DWordSwap( *((unsigned int*)pIn) ))

#if defined(PLAT_LITTLE_ENDIAN)
#define BigShort( val )				WordSwap( val )
#define BigWord( val )				WordSwap( val )
#define BigLong( val )				DWordSwap( val )
#define BigDWord( val )				DWordSwap( val )
#define LittleShort( val )			( val )
#define LittleWord( val )			( val )
#define LittleLong( val )			( val )
#define LittleDWord( val )			( val )
#define SwapShort( val )			BigShort( val )
#define SwapWord( val )				BigWord( val )
#define SwapLong( val )				BigLong( val )
#define SwapDWord( val )			BigDWord( val )


#define BigFloat( pOut, pIn )		SafeSwapFloat( pOut, pIn )
#define LittleFloat( pOut, pIn )	( *pOut = *pIn )
#define SwapFloat( pOut, pIn )		BigFloat( pOut, pIn )

#elif defined(PLAT_BIG_ENDIAN)

#define BigShort( val )				( val )
#define BigWord( val )				( val )
#define BigLong( val )				( val )
#define BigDWord( val )				( val )
#define LittleShort( val )			WordSwap( val )
#define LittleWord( val )			WordSwap( val )
#define LittleLong( val )			DWordSwap( val )
#define LittleDWord( val )			DWordSwap( val )
#define SwapShort( val )			LittleShort( val )
#define SwapWord( val )				LittleWord( val )
#define SwapLong( val )				LittleLong( val )
#define SwapDWord( val )			LittleDWord( val )


#define BigFloat( pOut, pIn )		( *pOut = *pIn )
#define LittleFloat( pOut, pIn )	SafeSwapFloat( pOut, pIn )
#define SwapFloat( pOut, pIn )		LittleFloat( pOut, pIn )

#else




inline short BigShort(short val) { int test = 1; return (*(char *)&test == 1) ? WordSwap(val) : val; }
inline uint16 BigWord(uint16 val) { int test = 1; return (*(char *)&test == 1) ? WordSwap(val) : val; }
inline long BigLong(long val) { int test = 1; return (*(char *)&test == 1) ? DWordSwap(val) : val; }
inline uint32_t BigDWord(uint32_t val) { int test = 1; return (*(char *)&test == 1) ? DWordSwap(val) : val; }
inline short LittleShort(short val) { int test = 1; return (*(char *)&test == 1) ? val : WordSwap(val); }
inline uint16 LittleWord(uint16 val) { int test = 1; return (*(char *)&test == 1) ? val : WordSwap(val); }
inline long LittleLong(long val) { int test = 1; return (*(char *)&test == 1) ? val : DWordSwap(val); }
inline uint32_t LittleDWord(uint32_t val) { int test = 1; return (*(char *)&test == 1) ? val : DWordSwap(val); }
inline short SwapShort(short val) { return WordSwap(val); }
inline uint16 SwapWord(uint16 val) { return WordSwap(val); }
inline long SwapLong(long val) { return DWordSwap(val); }
inline uint32_t SwapDWord(uint32_t val) { return DWordSwap(val); }


inline void BigFloat(float *pOut, const float *pIn) { int test = 1; (*(char *)&test == 1) ? SafeSwapFloat(pOut, pIn) : (*pOut = *pIn); }
inline void LittleFloat(float *pOut, const float *pIn) { int test = 1; (*(char *)&test == 1) ? (*pOut = *pIn) : SafeSwapFloat(pOut, pIn); }
inline void SwapFloat(float *pOut, const float *pIn) { SafeSwapFloat(pOut, pIn); }

#endif

inline uint32_t LoadLittleDWord(uint32_t *base, unsigned int dwordIndex) 
{
    return LittleDWord(base[dwordIndex]);
}

inline void StoreLittleDWord(uint32_t *base, unsigned int dwordIndex, uint32_t dword)
{
    base[dwordIndex] = LittleDWord(dword);
}


#define DISALLOW_OPERATOR_EQUAL( _classname )			\
	private:											\
		_classname &operator=( const _classname & );	\
	public:


#define IMPLEMENT_OPERATOR_EQUAL( _classname )			\
	public:												\
		_classname &operator=( const _classname &src )	\
		{												\
			memcpy( this, &src, sizeof(_classname) );	\
			return *this;								\
		}

#if _X360
#define Plat_FastMemset XMemSet
#define Plat_FastMemcpy XMemCpy
#else
#define Plat_FastMemset memset
#define Plat_FastMemcpy memcpy
#endif





#define XBOX_DVD_SECTORSIZE			2048
#define XBOX_DVD_ECC_SIZE			32768 
#define XBOX_HDD_SECTORSIZE			512


#define WM_XREMOTECOMMAND					(WM_USER + 100)
#define WM_XCONTROLLER_KEY					(WM_USER + 101)
#define WM_SYS_UI							(WM_USER + 102)
#define WM_SYS_SIGNINCHANGED				(WM_USER + 103)
#define WM_SYS_STORAGEDEVICESCHANGED		(WM_USER + 104)
#define WM_SYS_PROFILESETTINGCHANGED		(WM_USER + 105)
#define WM_SYS_MUTELISTCHANGED				(WM_USER + 106)
#define WM_SYS_INPUTDEVICESCHANGED			(WM_USER + 107)
#define WM_SYS_INPUTDEVICECONFIGCHANGED		(WM_USER + 108)
#define WM_LIVE_CONNECTIONCHANGED			(WM_USER + 109)
#define WM_LIVE_INVITE_ACCEPTED				(WM_USER + 110)
#define WM_LIVE_LINK_STATE_CHANGED			(WM_USER + 111)
#define WM_LIVE_CONTENT_INSTALLED			(WM_USER + 112)
#define WM_LIVE_MEMBERSHIP_PURCHASED		(WM_USER + 113)
#define WM_LIVE_VOICECHAT_AWAY				(WM_USER + 114)
#define WM_LIVE_PRESENCE_CHANGED			(WM_USER + 115)
#define WM_FRIENDS_PRESENCE_CHANGED			(WM_USER + 116)
#define WM_FRIENDS_FRIEND_ADDED				(WM_USER + 117)
#define WM_FRIENDS_FRIEND_REMOVED			(WM_USER + 118)
#define WM_CUSTOM_GAMEBANNERPRESSED			(WM_USER + 119)
#define WM_CUSTOM_ACTIONPRESSED				(WM_USER + 120)
#define WM_XMP_STATECHANGED					(WM_USER + 121)
#define WM_XMP_PLAYBACKBEHAVIORCHANGED		(WM_USER + 122)
#define WM_XMP_PLAYBACKCONTROLLERCHANGED	(WM_USER + 123)

inline const char *GetPlatformExt(void)
{
    return IsPlatformX360() ? ".360" : "";
}


#define XBOX_PROCESSOR_0			( 1<<0 )
#define XBOX_PROCESSOR_1			( 1<<1 )
#define XBOX_PROCESSOR_2			( 1<<2 )
#define XBOX_PROCESSOR_3			( 1<<3 )
#define XBOX_PROCESSOR_4			( 1<<4 )
#define XBOX_PROCESSOR_5			( 1<<5 )


#define XBOX_CORE_0_HWTHREAD_0		XBOX_PROCESSOR_0
#define XBOX_CORE_0_HWTHREAD_1		XBOX_PROCESSOR_1
#define XBOX_CORE_1_HWTHREAD_0		XBOX_PROCESSOR_2
#define XBOX_CORE_1_HWTHREAD_1		XBOX_PROCESSOR_3
#define XBOX_CORE_2_HWTHREAD_0		XBOX_PROCESSOR_4
#define XBOX_CORE_2_HWTHREAD_1		XBOX_PROCESSOR_5




#if defined( PLATFORM_X360 )
#include "xbox/xbox_core.hpp"
#endif





template <class T>
inline T* Construct(T* pMemory)
{
    return ::new(pMemory) T;
}

template <class T, typename ARG1>
inline T* Construct(T* pMemory, ARG1 a1)
{
    return ::new(pMemory) T(a1);
}

template <class T, typename ARG1, typename ARG2>
inline T* Construct(T* pMemory, ARG1 a1, ARG2 a2)
{
    return ::new(pMemory) T(a1, a2);
}

template <class T, typename ARG1, typename ARG2, typename ARG3>
inline T* Construct(T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3)
{
    return ::new(pMemory) T(a1, a2, a3);
}

template <class T, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
inline T* Construct(T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4)
{
    return ::new(pMemory) T(a1, a2, a3, a4);
}

template <class T, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
inline T* Construct(T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4, ARG5 a5)
{
    return ::new(pMemory) T(a1, a2, a3, a4, a5);
}

template <class T>
inline T* CopyConstruct(T* pMemory, T const& src)
{
    return ::new(pMemory) T(src);
}

template <class T>
inline void Destruct(T* pMemory)
{
    pMemory->~T();

#ifdef _DEBUG
    memset(pMemory, 0xDD, sizeof(T));
#endif
}
























#define GET_OUTER( OuterType, OuterMember ) \
   ( ( OuterType * ) ( (uint8_t *)this - offsetof( OuterType, OuterMember ) ) )




PLATFORM_INTERFACE bool vtune(bool resume);


#define TEMPLATE_FUNCTION_TABLE(RETURN_TYPE, NAME, ARGS, COUNT)			  \
                                                                      \
typedef RETURN_TYPE (FASTCALL *__Type_##NAME) ARGS;						        \
																		                                  \
template<const int nArgument>											                    \
struct __Function_##NAME												                      \
{																		                                  \
	static RETURN_TYPE FASTCALL Run ARGS;								                \
};																		                                \
																		                                  \
template <const int i>														                    \
struct __MetaLooper_##NAME : __MetaLooper_##NAME<i-1>					        \
{																		                                  \
	__Type_##NAME func;													                        \
	inline __MetaLooper_##NAME() { func = __Function_##NAME<i>::Run; }	\
};																		                                \
																		                                  \
template<>																                            \
struct __MetaLooper_##NAME<0>											                    \
{																		                                  \
	__Type_##NAME func;													                        \
	inline __MetaLooper_##NAME() { func = __Function_##NAME<0>::Run; }	\
};																		                                \
																		                                  \
class NAME																                            \
{																		                                  \
private:																                              \
    static const __MetaLooper_##NAME<COUNT> m;							          \
public:																	                              \
	enum { count = COUNT };												                      \
	static const __Type_##NAME* functions;								              \
};																		                                \
const __MetaLooper_##NAME<COUNT> NAME::m;								              \
const __Type_##NAME* NAME::functions = (__Type_##NAME*)&m;				    \
template<const int nArgument>													                \
RETURN_TYPE FASTCALL __Function_##NAME<nArgument>::Run ARGS


#define LOOP_INTERCHANGE(BOOLEAN, CODE)\
	if( (BOOLEAN) )\
	{\
		CODE;\
	} else\
	{\
		CODE;\
	}

#ifdef COMPILER_MSVC
FORCEINLINE uint32_t RotateBitsLeft32(uint32_t nValue, int nRotateBits)
{
    return _rotl(nValue, nRotateBits);
}
FORCEINLINE uint64_t RotateBitsLeft64(uint64_t nValue, int nRotateBits)
{
    return _rotl64(nValue, nRotateBits);
}
FORCEINLINE uint32_t RotateBitsRight32(uint32_t nValue, int nRotateBits)
{
    return _rotr(nValue, nRotateBits);
}
FORCEINLINE uint64_t RotateBitsRight64(uint64_t nValue, int nRotateBits)
{
    return _rotr64(nValue, nRotateBits);
}
#endif