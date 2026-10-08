

#ifndef _LJ_JIT_H
#define _LJ_JIT_H

#include "lj_obj.h"
#include "lj_ir.h"


#define JIT_F_ON		0x00000001


#if LJ_TARGET_X86ORX64
#define JIT_F_CMOV		0x00000010
#define JIT_F_SSE2		0x00000020
#define JIT_F_SSE3		0x00000040
#define JIT_F_SSE4_1		0x00000080
#define JIT_F_P4		0x00000100
#define JIT_F_PREFER_IMUL	0x00000200
#define JIT_F_SPLIT_XMM		0x00000400
#define JIT_F_LEA_AGU		0x00000800


#define JIT_F_CPU_FIRST		JIT_F_CMOV
#define JIT_F_CPUSTRING		"\4CMOV\4SSE2\4SSE3\6SSE4.1\2P4\3AMD\2K8\4ATOM"
#elif LJ_TARGET_ARM
#define JIT_F_ARMV6_		0x00000010
#define JIT_F_ARMV6T2_		0x00000020
#define JIT_F_ARMV7		0x00000040
#define JIT_F_VFPV2		0x00000080
#define JIT_F_VFPV3		0x00000100

#define JIT_F_ARMV6		(JIT_F_ARMV6_|JIT_F_ARMV6T2_|JIT_F_ARMV7)
#define JIT_F_ARMV6T2		(JIT_F_ARMV6T2_|JIT_F_ARMV7)
#define JIT_F_VFP		(JIT_F_VFPV2|JIT_F_VFPV3)


#define JIT_F_CPU_FIRST		JIT_F_ARMV6_
#define JIT_F_CPUSTRING		"\5ARMv6\7ARMv6T2\5ARMv7\5VFPv2\5VFPv3"
#elif LJ_TARGET_PPC
#define JIT_F_SQRT		0x00000010
#define JIT_F_ROUND		0x00000020


#define JIT_F_CPU_FIRST		JIT_F_SQRT
#define JIT_F_CPUSTRING		"\4SQRT\5ROUND"
#elif LJ_TARGET_MIPS
#define JIT_F_MIPS32R2		0x00000010


#define JIT_F_CPU_FIRST		JIT_F_MIPS32R2
#define JIT_F_CPUSTRING		"\010MIPS32R2"
#else
#define JIT_F_CPU_FIRST		0
#define JIT_F_CPUSTRING		""
#endif


#define JIT_F_OPT_MASK		0x0fff0000

#define JIT_F_OPT_FOLD		0x00010000
#define JIT_F_OPT_CSE		0x00020000
#define JIT_F_OPT_DCE		0x00040000
#define JIT_F_OPT_FWD		0x00080000
#define JIT_F_OPT_DSE		0x00100000
#define JIT_F_OPT_NARROW	0x00200000
#define JIT_F_OPT_LOOP		0x00400000
#define JIT_F_OPT_ABC		0x00800000
#define JIT_F_OPT_SINK		0x01000000
#define JIT_F_OPT_FUSE		0x02000000


#define JIT_F_OPT_FIRST		JIT_F_OPT_FOLD
#define JIT_F_OPTSTRING	\
  "\4fold\3cse\3dce\3fwd\3dse\6narrow\4loop\3abc\4sink\4fuse"


#define JIT_F_OPT_0	0
#define JIT_F_OPT_1	(JIT_F_OPT_FOLD|JIT_F_OPT_CSE|JIT_F_OPT_DCE)
#define JIT_F_OPT_2	(JIT_F_OPT_1|JIT_F_OPT_NARROW|JIT_F_OPT_LOOP)
#define JIT_F_OPT_3	(JIT_F_OPT_2|\
  JIT_F_OPT_FWD|JIT_F_OPT_DSE|JIT_F_OPT_ABC|JIT_F_OPT_SINK|JIT_F_OPT_FUSE)
#define JIT_F_OPT_DEFAULT	JIT_F_OPT_3

#if LJ_TARGET_WINDOWS || LJ_64

#define JIT_P_sizemcode_DEFAULT		32
#endif


#define JIT_PARAMDEF(_) \
  _(\010, maxtrace,	1000)	 \
  _(\011, maxrecord,	4000)	 \
  _(\012, maxirconst,	500)	 \
  _(\007, maxside,	100)	 \
  _(\007, maxsnap,	500)	 \
  \
  _(\007, hotloop,	56)	 \
  _(\007, hotexit,	10)	 \
  _(\007, tryside,	4)	 \
  \
  _(\012, instunroll,	4)	 \
  _(\012, loopunroll,	15)	 \
  _(\012, callunroll,	3)	 \
  _(\011, recunroll,	2)	 \
  \
   \
  _(\011, sizemcode,	JIT_P_sizemcode_DEFAULT) \
   \
  _(\010, maxmcode,	512) \
  

enum {
#define JIT_PARAMENUM(len, name, value)	JIT_P_##name,
JIT_PARAMDEF(JIT_PARAMENUM)
#undef JIT_PARAMENUM
  JIT_P__MAX
};

#define JIT_PARAMSTR(len, name, value)	#len #name
#define JIT_P_STRING	JIT_PARAMDEF(JIT_PARAMSTR)


typedef enum {
  LJ_TRACE_IDLE,	
  LJ_TRACE_ACTIVE = 0x10,
  LJ_TRACE_RECORD,	
  LJ_TRACE_START,	
  LJ_TRACE_END,		
  LJ_TRACE_ASM,		
  LJ_TRACE_ERR		
} TraceState;


typedef enum {
  LJ_POST_NONE,		
  LJ_POST_FIXCOMP,	
  LJ_POST_FIXGUARD,	
  LJ_POST_FIXGUARDSNAP,	
  LJ_POST_FIXBOOL,	
  LJ_POST_FIXCONST,	
  LJ_POST_FFRETRY	
} PostProc;


#if LJ_TARGET_X86ORX64
typedef uint8_t MCode;
#else
typedef uint32_t MCode;
#endif


typedef struct SnapShot {
  uint16_t mapofs;	
  IRRef1 ref;		
  uint8_t nslots;	
  uint8_t topslot;	
  uint8_t nent;		
  uint8_t count;	
} SnapShot;

#define SNAPCOUNT_DONE	255	


typedef uint32_t SnapEntry;

#define SNAP_FRAME		0x010000	
#define SNAP_CONT		0x020000	
#define SNAP_NORESTORE		0x040000	
#define SNAP_SOFTFPNUM		0x080000	
LJ_STATIC_ASSERT(SNAP_FRAME == TREF_FRAME);
LJ_STATIC_ASSERT(SNAP_CONT == TREF_CONT);

#define SNAP(slot, flags, ref)	(((SnapEntry)(slot) << 24) + (flags) + (ref))
#define SNAP_TR(slot, tr) \
  (((SnapEntry)(slot) << 24) + ((tr) & (TREF_CONT|TREF_FRAME|TREF_REFMASK)))
#define SNAP_MKPC(pc)		((SnapEntry)u32ptr(pc))
#define SNAP_MKFTSZ(ftsz)	((SnapEntry)(ftsz))
#define snap_ref(sn)		((sn) & 0xffff)
#define snap_slot(sn)		((BCReg)((sn) >> 24))
#define snap_isframe(sn)	((sn) & SNAP_FRAME)
#define snap_pc(sn)		((const BCIns *)(uintptr_t)(sn))
#define snap_setref(sn, ref)	(((sn) & (0xffff0000&~SNAP_NORESTORE)) | (ref))


typedef uint32_t SnapNo;
typedef uint32_t ExitNo;


typedef uint32_t TraceNo;	
typedef uint16_t TraceNo1;	


typedef enum {
  LJ_TRLINK_NONE,		
  LJ_TRLINK_ROOT,		
  LJ_TRLINK_LOOP,		
  LJ_TRLINK_TAILREC,		
  LJ_TRLINK_UPREC,		
  LJ_TRLINK_DOWNREC,		
  LJ_TRLINK_INTERP,		
  LJ_TRLINK_RETURN		
} TraceLink;


typedef struct GCtrace {
  GCHeader;
  uint8_t topslot;	
  uint8_t linktype;	
  IRRef nins;		
  GCRef gclist;
  IRIns *ir;		
  IRRef nk;		
  uint16_t nsnap;	
  uint16_t nsnapmap;	
  SnapShot *snap;	
  SnapEntry *snapmap;	
  GCRef startpt;	
  MRef startpc;		
  BCIns startins;	
  MSize szmcode;	
  MCode *mcode;		
  MSize mcloop;		
  uint16_t nchild;	
  uint16_t spadjust;	
  TraceNo1 traceno;	
  TraceNo1 link;	
  TraceNo1 root;	
  TraceNo1 nextroot;	
  TraceNo1 nextside;	
  uint8_t sinktags;	
  uint8_t unused1;
#ifdef LUAJIT_USE_GDBJIT
  void *gdbjit_entry;	
#endif
} GCtrace;

#define gco2trace(o)	check_exp((o)->gch.gct == ~LJ_TTRACE, (GCtrace *)(o))
#define traceref(J, n) \
  check_exp((n)>0 && (MSize)(n)<J->sizetrace, (GCtrace *)gcref(J->trace[(n)]))

LJ_STATIC_ASSERT(offsetof(GChead, gclist) == offsetof(GCtrace, gclist));

static LJ_AINLINE MSize snap_nextofs(GCtrace *T, SnapShot *snap)
{
  if (snap+1 == &T->snap[T->nsnap])
    return T->nsnapmap;
  else
    return (snap+1)->mapofs;
}


typedef struct HotPenalty {
  MRef pc;		
  uint16_t val;		
  uint16_t reason;	
} HotPenalty;

#define PENALTY_SLOTS	64	
#define PENALTY_MIN	(36*2)	
#define PENALTY_MAX	60000	
#define PENALTY_RNDBITS	4	


typedef struct BPropEntry {
  IRRef1 key;		
  IRRef1 val;		
  IRRef mode;		
} BPropEntry;


#define BPROP_SLOTS	16


typedef struct ScEvEntry {
  MRef pc;		
  IRRef1 idx;		
  IRRef1 start;		
  IRRef1 stop;		
  IRRef1 step;		
  IRType1 t;		
  uint8_t dir;		
} ScEvEntry;


enum {
  LJ_KSIMD_ABS,
  LJ_KSIMD_NEG,
  LJ_KSIMD__MAX
};


#define LJ_KSIMD(J, n) \
  ((TValue *)(((intptr_t)&J->ksimd[2*(n)] + 15) & ~(intptr_t)15))


#if LJ_SOFTFP || (LJ_32 && LJ_HASFFI)
#define lj_needsplit(J)		(J->needsplit = 1)
#define lj_resetsplit(J)	(J->needsplit = 0)
#else
#define lj_needsplit(J)		UNUSED(J)
#define lj_resetsplit(J)	UNUSED(J)
#endif


typedef struct FoldState {
  IRIns ins;		
  IRIns left;		
  IRIns right;		
} FoldState;


typedef struct jit_State {
  GCtrace cur;		

  lua_State *L;		
  const BCIns *pc;	
  GCfunc *fn;		
  GCproto *pt;		
  TRef *base;		

  uint32_t flags;	
  BCReg maxslot;	
  BCReg baseslot;	

  uint8_t mergesnap;	
  uint8_t needsnap;	
  IRType1 guardemit;	
  uint8_t bcskip;	

  FoldState fold;	

  const BCIns *bc_min;	
  MSize bc_extent;	

  TraceState state;	

  int32_t instunroll;	
  int32_t loopunroll;	
  int32_t tailcalled;	
  int32_t framedepth;	
  int32_t retdepth;	

  MRef k64;		
  TValue ksimd[LJ_KSIMD__MAX*2+1];  

  IRIns *irbuf;		
  IRRef irtoplim;	
  IRRef irbotlim;	
  IRRef loopref;	

  MSize sizesnap;	
  SnapShot *snapbuf;	
  SnapEntry *snapmapbuf;  
  MSize sizesnapmap;	

  PostProc postproc;	
#if LJ_SOFTFP || (LJ_32 && LJ_HASFFI)
  int needsplit;	
#endif

  GCRef *trace;		
  TraceNo freetrace;	
  MSize sizetrace;	

  IRRef1 chain[IR__MAX];  
  TRef slot[LJ_MAX_JSLOTS+LJ_STACK_EXTRA];  

  int32_t param[JIT_P__MAX];  

  MCode *exitstubgroup[LJ_MAX_EXITSTUBGR];  

  HotPenalty penalty[PENALTY_SLOTS];  
  uint32_t penaltyslot;	
  uint32_t prngstate;	

  BPropEntry bpropcache[BPROP_SLOTS];  
  uint32_t bpropslot;	

  ScEvEntry scev;	

  const BCIns *startpc;	
  TraceNo parent;	
  ExitNo exitno;	

  BCIns *patchpc;	
  BCIns patchins;	

  int mcprot;		
  MCode *mcarea;	
  MCode *mctop;		
  MCode *mcbot;		
  size_t szmcarea;	
  size_t szallmcarea;	

  TValue errinfo;	
}
#if LJ_TARGET_ARM
LJ_ALIGN(16)		
#endif
jit_State;


static LJ_AINLINE uint32_t LJ_PRNG_BITS(jit_State *J, int bits)
{
  
  J->prngstate = J->prngstate * 1103515245 + 12345;
  return J->prngstate >> (32-bits);
}

#endif
