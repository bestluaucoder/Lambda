

#ifndef _LUAJIT_H
#define _LUAJIT_H

#include "lua.h"

#define LUAJIT_VERSION		"LuaJIT 2.0.5"
#define LUAJIT_VERSION_NUM	20005  
#define LUAJIT_VERSION_SYM	luaJIT_version_2_0_5
#define LUAJIT_COPYRIGHT	"Copyright (C) 2005-2017 Mike Pall"
#define LUAJIT_URL		"http:


#define LUAJIT_MODE_MASK	0x00ff

enum {
  LUAJIT_MODE_ENGINE,		
  LUAJIT_MODE_DEBUG,		

  LUAJIT_MODE_FUNC,		
  LUAJIT_MODE_ALLFUNC,		
  LUAJIT_MODE_ALLSUBFUNC,	

  LUAJIT_MODE_TRACE,		

  LUAJIT_MODE_WRAPCFUNC = 0x10,	

  LUAJIT_MODE_MAX
};


#define LUAJIT_MODE_OFF		0x0000	
#define LUAJIT_MODE_ON		0x0100	
#define LUAJIT_MODE_FLUSH	0x0200	




LUA_API int luaJIT_setmode(lua_State *L, int idx, int mode);


LUA_API void LUAJIT_VERSION_SYM(void);

#endif
