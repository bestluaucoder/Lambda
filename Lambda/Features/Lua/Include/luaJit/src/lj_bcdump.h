

#ifndef _LJ_BCDUMP_H
#define _LJ_BCDUMP_H

#include "lj_obj.h"
#include "lj_lex.h"






#define BCDUMP_HEAD1		0x1b
#define BCDUMP_HEAD2		0x4c
#define BCDUMP_HEAD3		0x4a


#define BCDUMP_VERSION		1


#define BCDUMP_F_BE		0x01
#define BCDUMP_F_STRIP		0x02
#define BCDUMP_F_FFI		0x04

#define BCDUMP_F_KNOWN		(BCDUMP_F_FFI*2-1)


enum {
  BCDUMP_KGC_CHILD, BCDUMP_KGC_TAB, BCDUMP_KGC_I64, BCDUMP_KGC_U64,
  BCDUMP_KGC_COMPLEX, BCDUMP_KGC_STR
};


enum {
  BCDUMP_KTAB_NIL, BCDUMP_KTAB_FALSE, BCDUMP_KTAB_TRUE,
  BCDUMP_KTAB_INT, BCDUMP_KTAB_NUM, BCDUMP_KTAB_STR
};



LJ_FUNC int lj_bcwrite(lua_State *L, GCproto *pt, lua_Writer writer,
		       void *data, int strip);
LJ_FUNC GCproto *lj_bcread(LexState *ls);

#endif
