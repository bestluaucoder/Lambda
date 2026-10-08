#pragma once


#include <Windows.h>

#pragma pack(push, 8)
struct _UNICODE_STRING
{
	unsigned short Length; 
	unsigned short MaximumLength; 
	wchar_t* Buffer; 
};
static_assert(sizeof(_UNICODE_STRING) == 0x8);

struct _RTL_BALANCED_NODE
{
	_RTL_BALANCED_NODE* Children[2]; 
	unsigned long ParentValue; 
};
static_assert(sizeof(_RTL_BALANCED_NODE) == 0xC);

struct _LDR_DATA_TABLE_ENTRY
{
	_LIST_ENTRY InLoadOrderLinks; 
	_LIST_ENTRY InMemoryOrderLinks; 
	_LIST_ENTRY InInitializationOrderLinks; 
	void* DllBase; 
	void* EntryPoint; 
	unsigned long SizeOfImage; 
	_UNICODE_STRING FullDllName; 
	_UNICODE_STRING BaseDllName; 
	unsigned long Flags; 
	unsigned short ObsoleteLoadCount; 
	unsigned short TlsIndex; 
	_LIST_ENTRY HashLinks; 
	unsigned long TimeDateStamp; 
	void* EntryPointActivationContext; 
	void* Lock; 
	struct _LDR_DDAG_NODE* DdagNode; 
	_LIST_ENTRY NodeModuleLink; 
	struct _LDRP_LOAD_CONTEXT* LoadContext; 
	void* ParentDllBase; 
	void* SwitchBackContext; 
	_RTL_BALANCED_NODE BaseAddressIndexNode; 
	_RTL_BALANCED_NODE MappingInfoIndexNode; 
	unsigned long OriginalBase; 
	long long LoadTime; 
	unsigned long BaseNameHashValue; 
	unsigned long LoadReason; 
	unsigned long ImplicitPathOptions; 
	unsigned long ReferenceCount; 
};

static_assert(sizeof(_LDR_DATA_TABLE_ENTRY) == 0xA0);

struct _PEB_LDR_DATA
{
	unsigned long Length; 
	unsigned char Initialized; 
	void* SsHandle; 
	_LIST_ENTRY InLoadOrderModuleList; 
	_LIST_ENTRY InMemoryOrderModuleList; 
	_LIST_ENTRY InInitializationOrderModuleList; 
	void* EntryInProgress; 
	unsigned char ShutdownInProgress; 
	void* ShutdownThreadId; 
};

static_assert(sizeof(_PEB_LDR_DATA) == 0x30);

struct _CURDIR
{
	_UNICODE_STRING DosPath; 
	void* Handle; 
};
static_assert(sizeof(_CURDIR) == 0xC);

struct _STRING32
{
	unsigned short Length; 
	unsigned short MaximumLength; 
	char* Buffer; 
};
static_assert(sizeof(_STRING32) == 0x8);

struct _RTL_DRIVE_LETTER_CURDIR32
{
	unsigned short Flags; 
	unsigned short Length; 
	unsigned long Timestamp; 
	_STRING32 DosPath; 
};
static_assert(sizeof(_RTL_DRIVE_LETTER_CURDIR32) == 0x10);

struct _RTL_USER_PROCESS_PARAMETERS
{
	unsigned long MaximumLength; 
	unsigned long Length; 
	unsigned long Flags; 
	unsigned long DebugFlags; 
	void* ConsoleHandle; 
	unsigned long ConsoleFlags; 
	void* StandardInput; 
	void* StandardOutput; 
	void* StandardError; 
	_CURDIR CurrentDirectory; 
	_UNICODE_STRING DllPath; 
	_UNICODE_STRING ImagePathName; 
	_UNICODE_STRING CommandLine; 
	void* Environment; 
	unsigned long StartingX; 
	unsigned long StartingY; 
	unsigned long CountX; 
	unsigned long CountY; 
	unsigned long CountCharsX; 
	unsigned long CountCharsY; 
	unsigned long FillAttribute; 
	unsigned long WindowFlags; 
	unsigned long ShowWindowFlags; 
	_UNICODE_STRING WindowTitle; 
	_UNICODE_STRING DesktopInfo; 
	_UNICODE_STRING ShellInfo; 
	_UNICODE_STRING RuntimeData; 
	_RTL_DRIVE_LETTER_CURDIR32 CurrentDirectores[32]; 
	unsigned long EnvironmentSize; 
	unsigned long EnvironmentVersion; 
	void* PackageDependencyData; 
	unsigned long ProcessGroupId; 
	unsigned long LoaderThreads; 
};
static_assert(sizeof(_RTL_USER_PROCESS_PARAMETERS) == 0x2A4);

struct _PEB32
{
	unsigned char InheritedAddressSpace; 
	unsigned char ReadImageFileExecOptions; 
	unsigned char BeingDebugged; 
	unsigned char BitField; 
	void* Mutant; 
	void* ImageBaseAddress; 
	_PEB_LDR_DATA* Ldr; 
	_RTL_USER_PROCESS_PARAMETERS* ProcessParameters; 
	void* SubSystemData; 
	void* ProcessHeap; 
	_RTL_CRITICAL_SECTION* FastPebLock; 
	void* AtlThunkSListPtr; 
	void* IFEOKey; 
	unsigned long CrossProcessFlags; 
	void* KernelCallbackTable; 
	unsigned long SystemReserved[1]; 
	unsigned long AtlThunkSListPtr32; 
	void* ApiSetMap; 
	unsigned long TlsExpansionCounter; 
	void* TlsBitmap; 
	unsigned long TlsBitmapBits[2]; 
	void* ReadOnlySharedMemoryBase; 
	void* SparePvoid0; 
	void** ReadOnlyStaticServerData; 
	void* AnsiCodePageData; 
	void* OemCodePageData; 
	void* UnicodeCaseTableData; 
	unsigned long NumberOfProcessors; 
	unsigned long NtGlobalFlag; 
	long long CriticalSectionTimeout; 
	unsigned long HeapSegmentReserve; 
	unsigned long HeapSegmentCommit; 
	unsigned long HeapDeCommitTotalFreeThreshold; 
	unsigned long HeapDeCommitFreeBlockThreshold; 
	unsigned long NumberOfHeaps; 
	unsigned long MaximumNumberOfHeaps; 
	void** ProcessHeaps; 
	void* GdiSharedHandleTable; 
	void* ProcessStarterHelper; 
	unsigned long GdiDCAttributeList; 
	_RTL_CRITICAL_SECTION* LoaderLock; 
	unsigned long OSMajorVersion; 
	unsigned long OSMinorVersion; 
	unsigned short OSBuildNumber; 
	unsigned short OSCSDVersion; 
	unsigned long OSPlatformId; 
	unsigned long ImageSubsystem; 
	unsigned long ImageSubsystemMajorVersion; 
	unsigned long ImageSubsystemMinorVersion; 
	unsigned long ActiveProcessAffinityMask; 
	unsigned long GdiHandleBuffer[34]; 
	void(*PostProcessInitRoutine)(); 
	void* TlsExpansionBitmap; 
	unsigned long TlsExpansionBitmapBits[32]; 
	unsigned long SessionId; 
	unsigned long long AppCompatFlags; 
	unsigned long long AppCompatFlagsUser; 
	void* pShimData; 
	void* AppCompatInfo; 
	_UNICODE_STRING CSDVersion; 
	struct _ACTIVATION_CONTEXT_DATA* ActivationContextData; 
	struct _ASSEMBLY_STORAGE_MAP* ProcessAssemblyStorageMap; 
	_ACTIVATION_CONTEXT_DATA* SystemDefaultActivationContextData; 
	_ASSEMBLY_STORAGE_MAP* SystemAssemblyStorageMap; 
	unsigned long MinimumStackCommit; 
	struct _FLS_CALLBACK_INFO* FlsCallback; 
	_LIST_ENTRY FlsListHead; 
	void* FlsBitmap; 
	unsigned long FlsBitmapBits[4]; 
	unsigned long FlsHighIndex; 
	void* WerRegistrationData; 
	void* WerShipAssertPtr; 
	void* pUnused; 
	void* pImageHeaderHash; 
	unsigned long TracingFlags; 
	unsigned long long CsrServerReadOnlySharedMemoryBase; 
};
static_assert(sizeof(_PEB32) == 0x250);

struct _CLIENT_ID32
{
	unsigned long UniqueProcess; 
	unsigned long UniqueThread; 
};
static_assert(sizeof(_CLIENT_ID32) == 0x8);

struct _GDI_TEB_BATCH32
{
	unsigned long Offset : 31; 
	unsigned long HasRenderingCommand : 1; 
	unsigned long HDC; 
	unsigned long Buffer[310]; 
};
static_assert(sizeof(_GDI_TEB_BATCH32) == 0x4E0);

struct _TEB32
{
	_NT_TIB NtTib; 
	void* EnvironmentPointer; 
	_CLIENT_ID32 ClientId; 
	void* ActiveRpcHandle; 
	void* ThreadLocalStoragePointer; 
	_PEB32* ProcessEnvironmentBlock; 
	unsigned long LastErrorValue; 
	unsigned long CountOfOwnedCriticalSections; 
	void* CsrClientThread; 
	void* Win32ThreadInfo; 
	unsigned long User32Reserved[26]; 
	unsigned long UserReserved[5]; 
	void* WOW32Reserved; 
	unsigned long CurrentLocale; 
	unsigned long FpSoftwareStatusRegister; 
	void* ReservedForDebuggerInstrumentation[16]; 
	void* SystemReserved1[38]; 
	long ExceptionCode; 
	struct _ACTIVATION_CONTEXT_STACK* ActivationContextStackPointer; 
	unsigned long InstrumentationCallbackSp; 
	unsigned long InstrumentationCallbackPreviousPc; 
	unsigned long InstrumentationCallbackPreviousSp; 
	unsigned char InstrumentationCallbackDisabled; 
	unsigned char SpareBytes[23]; 
	unsigned long TxFsContext; 
	_GDI_TEB_BATCH32 GdiTebBatch; 
	_CLIENT_ID32 RealClientId; 
	void* GdiCachedProcessHandle; 
	unsigned long GdiClientPID; 
	unsigned long GdiClientTID; 
	void* GdiThreadLocalInfo; 
	unsigned long Win32ClientInfo[62]; 
	void* glDispatchTable[233]; 
	unsigned long glReserved1[29]; 
	void* glReserved2; 
	void* glSectionInfo; 
	void* glSection; 
	void* glTable; 
	void* glCurrentRC; 
	void* glContext; 
	unsigned long LastStatusValue; 
	_UNICODE_STRING StaticUnicodeString; 
	wchar_t StaticUnicodeBuffer[261]; 
	void* DeallocationStack; 
	void* TlsSlots[64]; 
	_LIST_ENTRY TlsLinks; 
	void* Vdm; 
	void* ReservedForNtRpc; 
	void* DbgSsReserved[2]; 
	unsigned long HardErrorMode; 
	void* Instrumentation[9]; 
	_GUID ActivityId; 
	void* SubProcessTag; 
	void* PerflibData; 
	void* EtwTraceData; 
	void* WinSockData; 
	unsigned long GdiBatchCount; 
	_PROCESSOR_NUMBER CurrentIdealProcessor; 
	unsigned long GuaranteedStackBytes; 
	void* ReservedForPerf; 
	void* ReservedForOle; 
	unsigned long WaitingOnLoaderLock; 
	void* SavedPriorityState; 
	unsigned long ReservedForCodeCoverage; 
	void* ThreadPoolData; 
	void** TlsExpansionSlots; 
	unsigned long MuiGeneration; 
	unsigned long IsImpersonating; 
	void* NlsCache; 
	void* pShimData; 
	unsigned short HeapVirtualAffinity; 
	unsigned short LowFragHeapDataSlot; 
	void* CurrentTransactionHandle; 
	struct _TEB_ACTIVE_FRAME* ActiveFrame; 
	void* FlsData; 
	void* PreferredLanguages; 
	void* UserPrefLanguages; 
	void* MergedPrefLanguages; 
	unsigned long MuiImpersonation; 
	volatile unsigned short CrossTebFlags; 
	unsigned short SameTebFlags; 
	void* TxnScopeEnterCallback; 
	void* TxnScopeExitCallback; 
	void* TxnScopeContext; 
	unsigned long LockCount; 
	long WowTebOffset; 
	void* ResourceRetValue; 
	void* ReservedForWdf; 
	unsigned long long ReservedForCrt; 
	_GUID EffectiveContainerId; 
};
static_assert(sizeof(_TEB32) == 0x1000);
#pragma pack(pop)