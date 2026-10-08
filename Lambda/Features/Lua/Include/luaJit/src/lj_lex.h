

#ifndef _LJ_LEX_H
#define _LJ_LEX_H

#include <stdarg.h>

#include "lj_obj.h"
#include "lj_err.h"


#define TKDEF(_, __) \
  _(and) _(break) _(do) _(else) _(elseif) _(end) _(false) \
  _(for) _(function) _(goto) _(if) _(in) _(local) _(nil) _(not) _(or) \
  _(repeat) _(return) _(then) _(true) _(until) _(while) \
  __(concat, ..) __(dots, ...) __(eq, ==) __(ge, >=) __(le, <=) __(ne, ~=) \
  __(label, ::) __(number, <number>) __(name, <name>) __(string, <string>) \
  __(eof, <eof>)

enum {
  TK_OFS = 256,
#define TKENUM1(name)		TK_##name,
#define TKENUM2(name, sym)	TK_##name,
TKDEF(TKENUM1, TKENUM2)
#undef TKENUM1
#undef TKENUM2
  TK_RESERVED = TK_while - TK_OFS
};

typedef int LexToken;


typedef struct BCInsLine {
  BCIns ins;		
  BCLine line;		
} BCInsLine;


typedef struct VarInfo {
  GCRef name;		
  BCPos startpc;	
  BCPos endpc;		
  uint8_t slot;		
  uint8_t info;		
} VarInfo;


typedef struct LexState {
  struct FuncState *fs;	
  struct lua_State *L;	
  TValue tokenval;	
  TValue lookaheadval;	
  int current;		
  LexToken token;	
  LexToken lookahead;	
  MSize n;		
  const char *p;	
  SBuf sb;		
  lua_Reader rfunc;	
  void *rdata;		
  BCLine linenumber;	
  BCLine lastline;	
  GCstr *chunkname;	
  const char *chunkarg;	
  const char *mode;	
  VarInfo *vstack;	
  MSize sizevstack;	
  MSize vtop;		
  BCInsLine *bcstack;	
  MSize sizebcstack;	
  uint32_t level;	
} LexState;

LJ_FUNC int lj_lex_setup(lua_State *L, LexState *ls);
LJ_FUNC void lj_lex_cleanup(lua_State *L, LexState *ls);
LJ_FUNC void lj_lex_next(LexState *ls);
LJ_FUNC LexToken lj_lex_lookahead(LexState *ls);
LJ_FUNC const char *lj_lex_token2str(LexState *ls, LexToken token);
LJ_FUNC_NORET void lj_lex_error(LexState *ls, LexToken token, ErrMsg em, ...);
LJ_FUNC void lj_lex_init(lua_State *L);

#endif
