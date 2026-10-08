




static MCode *asm_exitstub_gen(ASMState *as, ExitNo group)
{
  ExitNo i, groupofs = (group*EXITSTUBS_PER_GROUP) & 0xff;
  MCode *mxp = as->mcbot;
  MCode *mxpstart = mxp;
  if (mxp + (2+2)*EXITSTUBS_PER_GROUP+8+5 >= as->mctop)
    asm_mclimit(as);
  
  *mxp++ = XI_PUSHi8; *mxp++ = (MCode)groupofs;
  for (i = 1; i < EXITSTUBS_PER_GROUP; i++) {
    *mxp++ = XI_JMPs; *mxp++ = (MCode)((2+2)*(EXITSTUBS_PER_GROUP - i) - 2);
    *mxp++ = XI_PUSHi8; *mxp++ = (MCode)(groupofs + i);
  }
  
  *mxp++ = XI_PUSHi8; *mxp++ = (MCode)((group*EXITSTUBS_PER_GROUP)>>8);
  
  *mxp++ = XI_MOVmi;
  *mxp++ = MODRM(XM_OFS8, 0, RID_ESP);
  *mxp++ = MODRM(XM_SCALE1, RID_ESP, RID_ESP);
  *mxp++ = 2*sizeof(void *);
  *(int32_t *)mxp = ptr2addr(J2GG(as->J)->dispatch); mxp += 4;
  
  *mxp++ = XI_JMP; mxp += 4;
  *((int32_t *)(mxp-4)) = jmprel(mxp, (MCode *)(void *)lj_vm_exit_handler);
  
  lj_mcode_commitbot(as->J, mxp);
  as->mcbot = mxp;
  as->mclim = as->mcbot + MCLIM_REDZONE;
  return mxpstart;
}


static void asm_exitstub_setup(ASMState *as, ExitNo nexits)
{
  ExitNo i;
  if (nexits >= EXITSTUBS_PER_GROUP*LJ_MAX_EXITSTUBGR)
    lj_trace_err(as->J, LJ_TRERR_SNAPOV);
  for (i = 0; i < (nexits+EXITSTUBS_PER_GROUP-1)/EXITSTUBS_PER_GROUP; i++)
    if (as->J->exitstubgroup[i] == NULL)
      as->J->exitstubgroup[i] = asm_exitstub_gen(as, i);
}


static void asm_guardcc(ASMState *as, int cc)
{
  MCode *target = exitstub_addr(as->J, as->snapno);
  MCode *p = as->mcp;
  if (LJ_UNLIKELY(p == as->invmcp)) {
    as->loopinv = 1;
    *(int32_t *)(p+1) = jmprel(p+5, target);
    target = p;
    cc ^= 1;
    if (as->realign) {
      emit_sjcc(as, cc, target);
      return;
    }
  }
  emit_jcc(as, cc, target);
}




#define CONFLICT_SEARCH_LIM	31


static int asm_isk32(ASMState *as, IRRef ref, int32_t *k)
{
  if (irref_isk(ref)) {
    IRIns *ir = IR(ref);
    if (ir->o != IR_KINT64) {
      *k = ir->i;
      return 1;
    } else if (checki32((int64_t)ir_kint64(ir)->u64)) {
      *k = (int32_t)ir_kint64(ir)->u64;
      return 1;
    }
  }
  return 0;
}


static int noconflict(ASMState *as, IRRef ref, IROp conflict, int noload)
{
  IRIns *ir = as->ir;
  IRRef i = as->curins;
  if (i > ref + CONFLICT_SEARCH_LIM)
    return 0;  
  while (--i > ref) {
    if (ir[i].o == conflict)
      return 0;  
    else if (!noload && (ir[i].op1 == ref || ir[i].op2 == ref))
      return 0;
  }
  return 1;  
}


static IRRef asm_fuseabase(ASMState *as, IRRef ref)
{
  IRIns *irb = IR(ref);
  as->mrm.ofs = 0;
  if (irb->o == IR_FLOAD) {
    IRIns *ira = IR(irb->op1);
    lua_assert(irb->op2 == IRFL_TAB_ARRAY);
    
    if (ira->o == IR_TNEW && ira->op1 <= LJ_MAX_COLOSIZE &&
	!neverfuse(as) && noconflict(as, irb->op1, IR_NEWREF, 1)) {
      as->mrm.ofs = (int32_t)sizeof(GCtab);  
      return irb->op1;  
    }
  } else if (irb->o == IR_ADD && irref_isk(irb->op2)) {
    
    as->mrm.ofs = IR(irb->op2)->i;
    return irb->op1;
  }
  return ref;  
}


static void asm_fusearef(ASMState *as, IRIns *ir, RegSet allow)
{
  IRIns *irx;
  lua_assert(ir->o == IR_AREF);
  as->mrm.base = (uint8_t)ra_alloc1(as, asm_fuseabase(as, ir->op1), allow);
  irx = IR(ir->op2);
  if (irref_isk(ir->op2)) {
    as->mrm.ofs += 8*irx->i;
    as->mrm.idx = RID_NONE;
  } else {
    rset_clear(allow, as->mrm.base);
    as->mrm.scale = XM_SCALE8;
    
    if (!LJ_64 &&  
	mayfuse(as, ir->op2) && ra_noreg(irx->r) &&
	irx->o == IR_ADD && irref_isk(irx->op2)) {
      as->mrm.ofs += 8*IR(irx->op2)->i;
      as->mrm.idx = (uint8_t)ra_alloc1(as, irx->op1, allow);
    } else {
      as->mrm.idx = (uint8_t)ra_alloc1(as, ir->op2, allow);
    }
  }
}


static void asm_fuseahuref(ASMState *as, IRRef ref, RegSet allow)
{
  IRIns *ir = IR(ref);
  if (ra_noreg(ir->r)) {
    switch ((IROp)ir->o) {
    case IR_AREF:
      if (mayfuse(as, ref)) {
	asm_fusearef(as, ir, allow);
	return;
      }
      break;
    case IR_HREFK:
      if (mayfuse(as, ref)) {
	as->mrm.base = (uint8_t)ra_alloc1(as, ir->op1, allow);
	as->mrm.ofs = (int32_t)(IR(ir->op2)->op2 * sizeof(Node));
	as->mrm.idx = RID_NONE;
	return;
      }
      break;
    case IR_UREFC:
      if (irref_isk(ir->op1)) {
	GCfunc *fn = ir_kfunc(IR(ir->op1));
	GCupval *uv = &gcref(fn->l.uvptr[(ir->op2 >> 8)])->uv;
	as->mrm.ofs = ptr2addr(&uv->tv);
	as->mrm.base = as->mrm.idx = RID_NONE;
	return;
      }
      break;
    default:
      lua_assert(ir->o == IR_HREF || ir->o == IR_NEWREF || ir->o == IR_UREFO ||
		 ir->o == IR_KKPTR);
      break;
    }
  }
  as->mrm.base = (uint8_t)ra_alloc1(as, ref, allow);
  as->mrm.ofs = 0;
  as->mrm.idx = RID_NONE;
}


static void asm_fusefref(ASMState *as, IRIns *ir, RegSet allow)
{
  lua_assert(ir->o == IR_FLOAD || ir->o == IR_FREF);
  as->mrm.ofs = field_ofs[ir->op2];
  as->mrm.idx = RID_NONE;
  if (irref_isk(ir->op1)) {
    as->mrm.ofs += IR(ir->op1)->i;
    as->mrm.base = RID_NONE;
  } else {
    as->mrm.base = (uint8_t)ra_alloc1(as, ir->op1, allow);
  }
}


static void asm_fusestrref(ASMState *as, IRIns *ir, RegSet allow)
{
  IRIns *irr;
  lua_assert(ir->o == IR_STRREF);
  as->mrm.base = as->mrm.idx = RID_NONE;
  as->mrm.scale = XM_SCALE1;
  as->mrm.ofs = sizeof(GCstr);
  if (irref_isk(ir->op1)) {
    as->mrm.ofs += IR(ir->op1)->i;
  } else {
    Reg r = ra_alloc1(as, ir->op1, allow);
    rset_clear(allow, r);
    as->mrm.base = (uint8_t)r;
  }
  irr = IR(ir->op2);
  if (irref_isk(ir->op2)) {
    as->mrm.ofs += irr->i;
  } else {
    Reg r;
    
    if (!LJ_64 &&  
	mayfuse(as, ir->op2) && irr->o == IR_ADD && irref_isk(irr->op2)) {
      as->mrm.ofs += IR(irr->op2)->i;
      r = ra_alloc1(as, irr->op1, allow);
    } else {
      r = ra_alloc1(as, ir->op2, allow);
    }
    if (as->mrm.base == RID_NONE)
      as->mrm.base = (uint8_t)r;
    else
      as->mrm.idx = (uint8_t)r;
  }
}

static void asm_fusexref(ASMState *as, IRRef ref, RegSet allow)
{
  IRIns *ir = IR(ref);
  as->mrm.idx = RID_NONE;
  if (ir->o == IR_KPTR || ir->o == IR_KKPTR) {
    as->mrm.ofs = ir->i;
    as->mrm.base = RID_NONE;
  } else if (ir->o == IR_STRREF) {
    asm_fusestrref(as, ir, allow);
  } else {
    as->mrm.ofs = 0;
    if (canfuse(as, ir) && ir->o == IR_ADD && ra_noreg(ir->r)) {
      
      IRIns *irx;
      IRRef idx;
      Reg r;
      if (asm_isk32(as, ir->op2, &as->mrm.ofs)) {  
	ref = ir->op1;
	ir = IR(ref);
	if (!(ir->o == IR_ADD && canfuse(as, ir) && ra_noreg(ir->r)))
	  goto noadd;
      }
      as->mrm.scale = XM_SCALE1;
      idx = ir->op1;
      ref = ir->op2;
      irx = IR(idx);
      if (!(irx->o == IR_BSHL || irx->o == IR_ADD)) {  
	idx = ir->op2;
	ref = ir->op1;
	irx = IR(idx);
      }
      if (canfuse(as, irx) && ra_noreg(irx->r)) {
	if (irx->o == IR_BSHL && irref_isk(irx->op2) && IR(irx->op2)->i <= 3) {
	  
	  idx = irx->op1;
	  as->mrm.scale = (uint8_t)(IR(irx->op2)->i << 6);
	} else if (irx->o == IR_ADD && irx->op1 == irx->op2) {
	  
	  idx = irx->op1;
	  as->mrm.scale = XM_SCALE2;
	}
      }
      r = ra_alloc1(as, idx, allow);
      rset_clear(allow, r);
      as->mrm.idx = (uint8_t)r;
    }
  noadd:
    as->mrm.base = (uint8_t)ra_alloc1(as, ref, allow);
  }
}


static Reg asm_fuseload(ASMState *as, IRRef ref, RegSet allow)
{
  IRIns *ir = IR(ref);
  if (ra_hasreg(ir->r)) {
    if (allow != RSET_EMPTY) {  
      ra_noweak(as, ir->r);
      return ir->r;
    }
  fusespill:
    
    as->mrm.base = RID_ESP;
    as->mrm.ofs = ra_spill(as, ir);
    as->mrm.idx = RID_NONE;
    return RID_MRM;
  }
  if (ir->o == IR_KNUM) {
    RegSet avail = as->freeset & ~as->modset & RSET_FPR;
    lua_assert(allow != RSET_EMPTY);
    if (!(avail & (avail-1))) {  
      as->mrm.ofs = ptr2addr(ir_knum(ir));
      as->mrm.base = as->mrm.idx = RID_NONE;
      return RID_MRM;
    }
  } else if (ref == REF_BASE || ir->o == IR_KINT64) {
    RegSet avail = as->freeset & ~as->modset & RSET_GPR;
    lua_assert(allow != RSET_EMPTY);
    if (!(avail & (avail-1))) {  
      as->mrm.ofs = ptr2addr(ref == REF_BASE ? (void *)&J2G(as->J)->jit_base : (void *)ir_kint64(ir));
      as->mrm.base = as->mrm.idx = RID_NONE;
      return RID_MRM;
    }
  } else if (mayfuse(as, ref)) {
    RegSet xallow = (allow & RSET_GPR) ? allow : RSET_GPR;
    if (ir->o == IR_SLOAD) {
      if (!(ir->op2 & (IRSLOAD_PARENT|IRSLOAD_CONVERT)) &&
	  noconflict(as, ref, IR_RETF, 0)) {
	as->mrm.base = (uint8_t)ra_alloc1(as, REF_BASE, xallow);
	as->mrm.ofs = 8*((int32_t)ir->op1-1) + ((ir->op2&IRSLOAD_FRAME)?4:0);
	as->mrm.idx = RID_NONE;
	return RID_MRM;
      }
    } else if (ir->o == IR_FLOAD) {
      
      if ((irt_isint(ir->t) || irt_isu32(ir->t) || irt_isaddr(ir->t)) &&
	  noconflict(as, ref, IR_FSTORE, 0)) {
	asm_fusefref(as, ir, xallow);
	return RID_MRM;
      }
    } else if (ir->o == IR_ALOAD || ir->o == IR_HLOAD || ir->o == IR_ULOAD) {
      if (noconflict(as, ref, ir->o + IRDELTA_L2S, 0)) {
	asm_fuseahuref(as, ir->op1, xallow);
	return RID_MRM;
      }
    } else if (ir->o == IR_XLOAD) {
      
      if ((!irt_typerange(ir->t, IRT_I8, IRT_U16)) &&
	  noconflict(as, ref, IR_XSTORE, 0)) {
	asm_fusexref(as, ir->op1, xallow);
	return RID_MRM;
      }
    } else if (ir->o == IR_VLOAD) {
      asm_fuseahuref(as, ir->op1, xallow);
      return RID_MRM;
    }
  }
  if (!(as->freeset & allow) && !emit_canremat(ref) &&
      (allow == RSET_EMPTY || ra_hasspill(ir->s) || iscrossref(as, ref)))
    goto fusespill;
  return ra_allocref(as, ref, allow);
}

#if LJ_64

static Reg asm_fuseloadm(ASMState *as, IRRef ref, RegSet allow, int is64)
{
  if (is64 && !irt_is64(IR(ref)->t))
    return ra_alloc1(as, ref, allow);
  return asm_fuseload(as, ref, allow);
}
#else
#define asm_fuseloadm(as, ref, allow, is64)  asm_fuseload(as, (ref), (allow))
#endif




static int asm_count_call_slots(ASMState *as, const CCallInfo *ci, IRRef *args)
{
  uint32_t i, nargs = CCI_NARGS(ci);
  int nslots = 0;
#if LJ_64
  if (LJ_ABI_WIN) {
    nslots = (int)(nargs*2);  
  } else {
    int ngpr = REGARG_NUMGPR, nfpr = REGARG_NUMFPR;
    for (i = 0; i < nargs; i++)
      if (args[i] && irt_isfp(IR(args[i])->t)) {
	if (nfpr > 0) nfpr--; else nslots += 2;
      } else {
	if (ngpr > 0) ngpr--; else nslots += 2;
      }
  }
#else
  int ngpr = 0;
  if ((ci->flags & CCI_CC_MASK) == CCI_CC_FASTCALL)
    ngpr = 2;
  else if ((ci->flags & CCI_CC_MASK) == CCI_CC_THISCALL)
    ngpr = 1;
  for (i = 0; i < nargs; i++)
    if (args[i] && irt_isfp(IR(args[i])->t)) {
      nslots += irt_isnum(IR(args[i])->t) ? 2 : 1;
    } else {
      if (ngpr > 0) ngpr--; else nslots++;
    }
#endif
  return nslots;
}


static void asm_gencall(ASMState *as, const CCallInfo *ci, IRRef *args)
{
  uint32_t n, nargs = CCI_NARGS(ci);
  int32_t ofs = STACKARG_OFS;
#if LJ_64
  uint32_t gprs = REGARG_GPRS;
  Reg fpr = REGARG_FIRSTFPR;
#if !LJ_ABI_WIN
  MCode *patchnfpr = NULL;
#endif
#else
  uint32_t gprs = 0;
  if ((ci->flags & CCI_CC_MASK) != CCI_CC_CDECL) {
    if ((ci->flags & CCI_CC_MASK) == CCI_CC_THISCALL)
      gprs = (REGARG_GPRS & 31);
    else if ((ci->flags & CCI_CC_MASK) == CCI_CC_FASTCALL)
      gprs = REGARG_GPRS;
  }
#endif
  if ((void *)ci->func)
    emit_call(as, ci->func);
#if LJ_64
  if ((ci->flags & CCI_VARARG)) {  
#if LJ_ABI_WIN
    for (n = 0; n < 4 && n < nargs; n++) {
      IRIns *ir = IR(args[n]);
      if (irt_isfp(ir->t))  
	emit_rr(as, XO_MOVDto, (irt_isnum(ir->t) ? REX_64 : 0) | (fpr+n),
		((gprs >> (n*5)) & 31));  
    }
#else
    patchnfpr = --as->mcp;  
    *--as->mcp = XI_MOVrib | RID_EAX;
#endif
  }
#endif
  for (n = 0; n < nargs; n++) {  
    IRRef ref = args[n];
    IRIns *ir = IR(ref);
    Reg r;
#if LJ_64 && LJ_ABI_WIN
    
    r = irt_isfp(ir->t) ? (fpr <= REGARG_LASTFPR ? fpr : 0) : (gprs & 31);
    fpr++; gprs >>= 5;
#elif LJ_64
    
    if (irt_isfp(ir->t)) {
      r = fpr <= REGARG_LASTFPR ? fpr++ : 0;
    } else {
      r = gprs & 31; gprs >>= 5;
    }
#else
    if (ref && irt_isfp(ir->t)) {
      r = 0;
    } else {
      r = gprs & 31; gprs >>= 5;
      if (!ref) continue;
    }
#endif
    if (r) {  
      if (r < RID_MAX_GPR && ref < ASMREF_TMP1) {
#if LJ_64
	if (ir->o == IR_KINT64)
	  emit_loadu64(as, r, ir_kint64(ir)->u64);
	else
#endif
	  emit_loadi(as, r, ir->i);
      } else {
	lua_assert(rset_test(as->freeset, r));  
	if (ra_hasreg(ir->r)) {
	  ra_noweak(as, ir->r);
	  emit_movrr(as, ir, r, ir->r);
	} else {
	  ra_allocref(as, ref, RID2RSET(r));
	}
      }
    } else if (irt_isfp(ir->t)) {  
      lua_assert(!(irt_isfloat(ir->t) && irref_isk(ref)));  
      if (LJ_32 && (ofs & 4) && irref_isk(ref)) {
	
	emit_movmroi(as, RID_ESP, ofs, (int32_t)ir_knum(ir)->u32.lo);
	emit_movmroi(as, RID_ESP, ofs+4, (int32_t)ir_knum(ir)->u32.hi);
      } else {
	r = ra_alloc1(as, ref, RSET_FPR);
	emit_rmro(as, irt_isnum(ir->t) ? XO_MOVSDto : XO_MOVSSto,
		  r, RID_ESP, ofs);
      }
      ofs += (LJ_32 && irt_isfloat(ir->t)) ? 4 : 8;
    } else {  
      if (LJ_32 && ref < ASMREF_TMP1) {
	emit_movmroi(as, RID_ESP, ofs, ir->i);
      } else {
	r = ra_alloc1(as, ref, RSET_GPR);
	emit_movtomro(as, REX_64 + r, RID_ESP, ofs);
      }
      ofs += sizeof(intptr_t);
    }
    checkmclim(as);
  }
#if LJ_64 && !LJ_ABI_WIN
  if (patchnfpr) *patchnfpr = fpr - REGARG_FIRSTFPR;
#endif
}


static void asm_setupresult(ASMState *as, IRIns *ir, const CCallInfo *ci)
{
  RegSet drop = RSET_SCRATCH;
  int hiop = (LJ_32 && (ir+1)->o == IR_HIOP && !irt_isnil((ir+1)->t));
  if ((ci->flags & CCI_NOFPRCLOBBER))
    drop &= ~RSET_FPR;
  if (ra_hasreg(ir->r))
    rset_clear(drop, ir->r);  
  if (hiop && ra_hasreg((ir+1)->r))
    rset_clear(drop, (ir+1)->r);  
  ra_evictset(as, drop);  
  if (ra_used(ir)) {
    if (irt_isfp(ir->t)) {
      int32_t ofs = sps_scale(ir->s);  
#if LJ_64
      if ((ci->flags & CCI_CASTU64)) {
	Reg dest = ir->r;
	if (ra_hasreg(dest)) {
	  ra_free(as, dest);
	  ra_modified(as, dest);
	  emit_rr(as, XO_MOVD, dest|REX_64, RID_RET);  
	}
	if (ofs) emit_movtomro(as, RID_RET|REX_64, RID_ESP, ofs);
      } else {
	ra_destreg(as, ir, RID_FPRET);
      }
#else
      
      Reg dest = ir->r;
      if (ra_hasreg(dest)) {
	ra_free(as, dest);
	ra_modified(as, dest);
	emit_rmro(as, irt_isnum(ir->t) ? XMM_MOVRM(as) : XO_MOVSS,
		  dest, RID_ESP, ofs);
      }
      if ((ci->flags & CCI_CASTU64)) {
	emit_movtomro(as, RID_RETLO, RID_ESP, ofs);
	emit_movtomro(as, RID_RETHI, RID_ESP, ofs+4);
      } else {
	emit_rmro(as, irt_isnum(ir->t) ? XO_FSTPq : XO_FSTPd,
		  irt_isnum(ir->t) ? XOg_FSTPq : XOg_FSTPd, RID_ESP, ofs);
      }
#endif
#if LJ_32
    } else if (hiop) {
      ra_destpair(as, ir);
#endif
    } else {
      lua_assert(!irt_ispri(ir->t));
      ra_destreg(as, ir, RID_RET);
    }
  } else if (LJ_32 && irt_isfp(ir->t) && !(ci->flags & CCI_CASTU64)) {
    emit_x87op(as, XI_FPOP);  
  }
}

static void asm_call(ASMState *as, IRIns *ir)
{
  IRRef args[CCI_NARGS_MAX];
  const CCallInfo *ci = &lj_ir_callinfo[ir->op2];
  asm_collectargs(as, ir, ci, args);
  asm_setupresult(as, ir, ci);
  asm_gencall(as, ci, args);
}


static void *asm_callx_func(ASMState *as, IRIns *irf, IRRef func)
{
#if LJ_32
  UNUSED(as);
  if (irref_isk(func))
    return (void *)irf->i;
#else
  if (irref_isk(func)) {
    MCode *p;
    if (irf->o == IR_KINT64)
      p = (MCode *)(void *)ir_k64(irf)->u64;
    else
      p = (MCode *)(void *)(uintptr_t)(uint32_t)irf->i;
    if (p - as->mcp == (int32_t)(p - as->mcp))
      return p;  
    
  }
#endif
  return NULL;
}

static void asm_callx(ASMState *as, IRIns *ir)
{
  IRRef args[CCI_NARGS_MAX*2];
  CCallInfo ci;
  IRRef func;
  IRIns *irf;
  int32_t spadj = 0;
  ci.flags = asm_callx_flags(as, ir);
  asm_collectargs(as, ir, &ci, args);
  asm_setupresult(as, ir, &ci);
#if LJ_32
  
  if ((ci.flags & CCI_CC_MASK) != CCI_CC_CDECL)
    spadj = 4 * asm_count_call_slots(as, &ci, args);
#endif
  func = ir->op2; irf = IR(func);
  if (irf->o == IR_CARG) { func = irf->op1; irf = IR(func); }
  ci.func = (ASMFunction)asm_callx_func(as, irf, func);
  if (!(void *)ci.func) {
    
    RegSet allow = (RSET_GPR & ~RSET_SCRATCH);
    Reg r = ra_alloc1(as, func, allow);
    if (LJ_32) emit_spsub(as, spadj);  
    emit_rr(as, XO_GROUP5, XOg_CALL, r);
  } else if (LJ_32) {
    emit_spsub(as, spadj);
  }
  asm_gencall(as, &ci, args);
}




static void asm_retf(ASMState *as, IRIns *ir)
{
  Reg base = ra_alloc1(as, REF_BASE, RSET_GPR);
  void *pc = ir_kptr(IR(ir->op2));
  int32_t delta = 1+bc_a(*((const BCIns *)pc - 1));
  as->topslot -= (BCReg)delta;
  if ((int32_t)as->topslot < 0) as->topslot = 0;
  irt_setmark(IR(REF_BASE)->t);  
  emit_setgl(as, base, jit_base);
  emit_addptr(as, base, -8*delta);
  asm_guardcc(as, CC_NE);
  emit_gmroi(as, XG_ARITHi(XOg_CMP), base, -4, ptr2addr(pc));
}



static void asm_tointg(ASMState *as, IRIns *ir, Reg left)
{
  Reg tmp = ra_scratch(as, rset_exclude(RSET_FPR, left));
  Reg dest = ra_dest(as, ir, RSET_GPR);
  asm_guardcc(as, CC_P);
  asm_guardcc(as, CC_NE);
  emit_rr(as, XO_UCOMISD, left, tmp);
  emit_rr(as, XO_CVTSI2SD, tmp, dest);
  if (!(as->flags & JIT_F_SPLIT_XMM))
    emit_rr(as, XO_XORPS, tmp, tmp);  
  emit_rr(as, XO_CVTTSD2SI, dest, left);
  
}

static void asm_tobit(ASMState *as, IRIns *ir)
{
  Reg dest = ra_dest(as, ir, RSET_GPR);
  Reg tmp = ra_noreg(IR(ir->op1)->r) ?
	      ra_alloc1(as, ir->op1, RSET_FPR) :
	      ra_scratch(as, RSET_FPR);
  Reg right = asm_fuseload(as, ir->op2, rset_exclude(RSET_FPR, tmp));
  emit_rr(as, XO_MOVDto, tmp, dest);
  emit_mrm(as, XO_ADDSD, tmp, right);
  ra_left(as, tmp, ir->op1);
}

static void asm_conv(ASMState *as, IRIns *ir)
{
  IRType st = (IRType)(ir->op2 & IRCONV_SRCMASK);
  int st64 = (st == IRT_I64 || st == IRT_U64 || (LJ_64 && st == IRT_P64));
  int stfp = (st == IRT_NUM || st == IRT_FLOAT);
  IRRef lref = ir->op1;
  lua_assert(irt_type(ir->t) != st);
  lua_assert(!(LJ_32 && (irt_isint64(ir->t) || st64)));  
  if (irt_isfp(ir->t)) {
    Reg dest = ra_dest(as, ir, RSET_FPR);
    if (stfp) {  
      Reg left = asm_fuseload(as, lref, RSET_FPR);
      emit_mrm(as, st == IRT_NUM ? XO_CVTSD2SS : XO_CVTSS2SD, dest, left);
      if (left == dest) return;  
    } else if (LJ_32 && st == IRT_U32) {  
      
      cTValue *k = lj_ir_k64_find(as->J, U64x(43380000,00000000));
      Reg bias = ra_scratch(as, rset_exclude(RSET_FPR, dest));
      if (irt_isfloat(ir->t))
	emit_rr(as, XO_CVTSD2SS, dest, dest);
      emit_rr(as, XO_SUBSD, dest, bias);  
      emit_rr(as, XO_XORPS, dest, bias);  
      emit_loadn(as, bias, k);
      emit_mrm(as, XO_MOVD, dest, asm_fuseload(as, lref, RSET_GPR));
      return;
    } else {  
      Reg left = (LJ_64 && (st == IRT_U32 || st == IRT_U64)) ?
		 ra_alloc1(as, lref, RSET_GPR) :
		 asm_fuseloadm(as, lref, RSET_GPR, st64);
      if (LJ_64 && st == IRT_U64) {
	MCLabel l_end = emit_label(as);
	const void *k = lj_ir_k64_find(as->J, U64x(43f00000,00000000));
	emit_rma(as, XO_ADDSD, dest, k);  
	emit_sjcc(as, CC_NS, l_end);
	emit_rr(as, XO_TEST, left|REX_64, left);  
      }
      emit_mrm(as, irt_isnum(ir->t) ? XO_CVTSI2SD : XO_CVTSI2SS,
	       dest|((LJ_64 && (st64 || st == IRT_U32)) ? REX_64 : 0), left);
    }
    if (!(as->flags & JIT_F_SPLIT_XMM))
      emit_rr(as, XO_XORPS, dest, dest);  
  } else if (stfp) {  
    if (irt_isguard(ir->t)) {
      
      lua_assert(irt_isint(ir->t) && st == IRT_NUM);
      asm_tointg(as, ir, ra_alloc1(as, lref, RSET_FPR));
    } else {
      Reg dest = ra_dest(as, ir, RSET_GPR);
      x86Op op = st == IRT_NUM ?
		 ((ir->op2 & IRCONV_TRUNC) ? XO_CVTTSD2SI : XO_CVTSD2SI) :
		 ((ir->op2 & IRCONV_TRUNC) ? XO_CVTTSS2SI : XO_CVTSS2SI);
      if (LJ_64 ? irt_isu64(ir->t) : irt_isu32(ir->t)) {
	
	
	Reg tmp = ra_noreg(IR(lref)->r) ? ra_alloc1(as, lref, RSET_FPR) :
					  ra_scratch(as, RSET_FPR);
	MCLabel l_end = emit_label(as);
	if (LJ_32)
	  emit_gri(as, XG_ARITHi(XOg_ADD), dest, (int32_t)0x80000000);
	emit_rr(as, op, dest|REX_64, tmp);
	if (st == IRT_NUM)
	  emit_rma(as, XO_ADDSD, tmp, lj_ir_k64_find(as->J,
		   LJ_64 ? U64x(c3f00000,00000000) : U64x(c1e00000,00000000)));
	else
	  emit_rma(as, XO_ADDSS, tmp, lj_ir_k64_find(as->J,
		   LJ_64 ? U64x(00000000,df800000) : U64x(00000000,cf000000)));
	emit_sjcc(as, CC_NS, l_end);
	emit_rr(as, XO_TEST, dest|REX_64, dest);  
	emit_rr(as, op, dest|REX_64, tmp);
	ra_left(as, tmp, lref);
      } else {
	Reg left = asm_fuseload(as, lref, RSET_FPR);
	if (LJ_64 && irt_isu32(ir->t))
	  emit_rr(as, XO_MOV, dest, dest);  
	emit_mrm(as, op,
		 dest|((LJ_64 &&
			(irt_is64(ir->t) || irt_isu32(ir->t))) ? REX_64 : 0),
		 left);
      }
    }
  } else if (st >= IRT_I8 && st <= IRT_U16) {  
    Reg left, dest = ra_dest(as, ir, RSET_GPR);
    RegSet allow = RSET_GPR;
    x86Op op;
    lua_assert(irt_isint(ir->t) || irt_isu32(ir->t));
    if (st == IRT_I8) {
      op = XO_MOVSXb; allow = RSET_GPR8; dest |= FORCE_REX;
    } else if (st == IRT_U8) {
      op = XO_MOVZXb; allow = RSET_GPR8; dest |= FORCE_REX;
    } else if (st == IRT_I16) {
      op = XO_MOVSXw;
    } else {
      op = XO_MOVZXw;
    }
    left = asm_fuseload(as, lref, allow);
    
    if (!LJ_64 && left != RID_MRM && !rset_test(allow, left)) {
      Reg tmp = ra_scratch(as, allow);
      emit_rr(as, op, dest, tmp);
      emit_rr(as, XO_MOV, tmp, left);
    } else {
      emit_mrm(as, op, dest, left);
    }
  } else {  
    if (LJ_32) {  
      Reg dest = ra_dest(as, ir, RSET_GPR);
      ra_left(as, dest, lref);  
    } else if (irt_is64(ir->t)) {
      Reg dest = ra_dest(as, ir, RSET_GPR);
      if (st64 || !(ir->op2 & IRCONV_SEXT)) {
	
	ra_left(as, dest, lref);  
      } else {  
	Reg left = asm_fuseload(as, lref, RSET_GPR);
	emit_mrm(as, XO_MOVSXd, dest|REX_64, left);
      }
    } else {
      Reg dest = ra_dest(as, ir, RSET_GPR);
      if (st64) {
	Reg left = asm_fuseload(as, lref, RSET_GPR);
	
	emit_mrm(as, XO_MOV, dest, left);
      } else {  
	ra_left(as, dest, lref);  
      }
    }
  }
}

#if LJ_32 && LJ_HASFFI



static void asm_conv_fp_int64(ASMState *as, IRIns *ir)
{
  Reg hi = ra_alloc1(as, ir->op1, RSET_GPR);
  Reg lo = ra_alloc1(as, (ir-1)->op1, rset_exclude(RSET_GPR, hi));
  int32_t ofs = sps_scale(ir->s);  
  Reg dest = ir->r;
  if (ra_hasreg(dest)) {
    ra_free(as, dest);
    ra_modified(as, dest);
    emit_rmro(as, irt_isnum(ir->t) ? XMM_MOVRM(as) : XO_MOVSS,
	      dest, RID_ESP, ofs);
  }
  emit_rmro(as, irt_isnum(ir->t) ? XO_FSTPq : XO_FSTPd,
	    irt_isnum(ir->t) ? XOg_FSTPq : XOg_FSTPd, RID_ESP, ofs);
  if (((ir-1)->op2 & IRCONV_SRCMASK) == IRT_U64) {
    
    MCLabel l_end = emit_label(as);
    emit_rma(as, XO_FADDq, XOg_FADDq,
	     lj_ir_k64_find(as->J, U64x(43f00000,00000000)));
    emit_sjcc(as, CC_NS, l_end);
    emit_rr(as, XO_TEST, hi, hi);  
  } else {
    lua_assert(((ir-1)->op2 & IRCONV_SRCMASK) == IRT_I64);
  }
  emit_rmro(as, XO_FILDq, XOg_FILDq, RID_ESP, 0);
  
  emit_rmro(as, XO_MOVto, hi, RID_ESP, 4);
  emit_rmro(as, XO_MOVto, lo, RID_ESP, 0);
}


static void asm_conv_int64_fp(ASMState *as, IRIns *ir)
{
  IRType st = (IRType)((ir-1)->op2 & IRCONV_SRCMASK);
  IRType dt = (((ir-1)->op2 & IRCONV_DSTMASK) >> IRCONV_DSH);
  Reg lo, hi;
  lua_assert(st == IRT_NUM || st == IRT_FLOAT);
  lua_assert(dt == IRT_I64 || dt == IRT_U64);
  lua_assert(((ir-1)->op2 & IRCONV_TRUNC));
  hi = ra_dest(as, ir, RSET_GPR);
  lo = ra_dest(as, ir-1, rset_exclude(RSET_GPR, hi));
  if (ra_used(ir-1)) emit_rmro(as, XO_MOV, lo, RID_ESP, 0);
  
  if (!(as->flags & JIT_F_SSE3)) {  
    emit_rmro(as, XO_FLDCW, XOg_FLDCW, RID_ESP, 4);
    emit_rmro(as, XO_MOVto, lo, RID_ESP, 4);
    emit_gri(as, XG_ARITHi(XOg_AND), lo, 0xf3ff);
  }
  if (dt == IRT_U64) {
    
    MCLabel l_pop, l_end = emit_label(as);
    emit_x87op(as, XI_FPOP);
    l_pop = emit_label(as);
    emit_sjmp(as, l_end);
    emit_rmro(as, XO_MOV, hi, RID_ESP, 4);
    if ((as->flags & JIT_F_SSE3))
      emit_rmro(as, XO_FISTTPq, XOg_FISTTPq, RID_ESP, 0);
    else
      emit_rmro(as, XO_FISTPq, XOg_FISTPq, RID_ESP, 0);
    emit_rma(as, XO_FADDq, XOg_FADDq,
	     lj_ir_k64_find(as->J, U64x(c3f00000,00000000)));
    emit_sjcc(as, CC_NS, l_pop);
    emit_rr(as, XO_TEST, hi, hi);  
  }
  emit_rmro(as, XO_MOV, hi, RID_ESP, 4);
  if ((as->flags & JIT_F_SSE3)) {  
    emit_rmro(as, XO_FISTTPq, XOg_FISTTPq, RID_ESP, 0);
  } else {  
    emit_rmro(as, XO_FISTPq, XOg_FISTPq, RID_ESP, 0);
    emit_rmro(as, XO_FLDCW, XOg_FLDCW, RID_ESP, 0);
    emit_rmro(as, XO_MOVtow, lo, RID_ESP, 0);
    emit_rmro(as, XO_ARITHw(XOg_OR), lo, RID_ESP, 0);
    emit_loadi(as, lo, 0xc00);
    emit_rmro(as, XO_FNSTCW, XOg_FNSTCW, RID_ESP, 0);
  }
  if (dt == IRT_U64)
    emit_x87op(as, XI_FDUP);
  emit_mrm(as, st == IRT_NUM ? XO_FLDq : XO_FLDd,
	   st == IRT_NUM ? XOg_FLDq: XOg_FLDd,
	   asm_fuseload(as, ir->op1, RSET_EMPTY));
}
#endif

static void asm_strto(ASMState *as, IRIns *ir)
{
  
  const CCallInfo *ci = &lj_ir_callinfo[IRCALL_lj_strscan_num];
  IRRef args[2];
  RegSet drop = RSET_SCRATCH;
  if ((drop & RSET_FPR) != RSET_FPR && ra_hasreg(ir->r))
    rset_set(drop, ir->r);  
  ra_evictset(as, drop);
  asm_guardcc(as, CC_E);
  emit_rr(as, XO_TEST, RID_RET, RID_RET);  
  args[0] = ir->op1;      
  args[1] = ASMREF_TMP1;  
  asm_gencall(as, ci, args);
  
  emit_rmro(as, XO_LEA, ra_releasetmp(as, ASMREF_TMP1)|REX_64,
	    RID_ESP, sps_scale(ir->s));
}

static void asm_tostr(ASMState *as, IRIns *ir)
{
  IRIns *irl = IR(ir->op1);
  IRRef args[2];
  args[0] = ASMREF_L;
  as->gcsteps++;
  if (irt_isnum(irl->t)) {
    const CCallInfo *ci = &lj_ir_callinfo[IRCALL_lj_str_fromnum];
    args[1] = ASMREF_TMP1;  
    asm_setupresult(as, ir, ci);  
    asm_gencall(as, ci, args);
    emit_rmro(as, XO_LEA, ra_releasetmp(as, ASMREF_TMP1)|REX_64,
	      RID_ESP, ra_spill(as, irl));
  } else {
    const CCallInfo *ci = &lj_ir_callinfo[IRCALL_lj_str_fromint];
    args[1] = ir->op1;  
    asm_setupresult(as, ir, ci);  
    asm_gencall(as, ci, args);
  }
}



static void asm_aref(ASMState *as, IRIns *ir)
{
  Reg dest = ra_dest(as, ir, RSET_GPR);
  asm_fusearef(as, ir, RSET_GPR);
  if (!(as->mrm.idx == RID_NONE && as->mrm.ofs == 0))
    emit_mrm(as, XO_LEA, dest, RID_MRM);
  else if (as->mrm.base != dest)
    emit_rr(as, XO_MOV, dest, as->mrm.base);
}


static MCode *merge_href_niltv(ASMState *as, IRIns *ir)
{
  
  if ((ir[1].o == IR_NE || ir[1].o == IR_EQ) && ir[1].op1 == as->curins &&
      ra_hasreg(ir->r)) {
    MCode *p = as->mcp;
    p += (LJ_64 && *p != XI_ARITHi) ? 7+6 : 6+6;
    
    if (p[-6] == 0x0f && p[-5] == XI_JCCn+(CC_NE^(ir[1].o & 1))) {
      as->mcp = p;  
      return p + *(int32_t *)(p-4);  
    }
  }
  return NULL;
}


static void asm_href(ASMState *as, IRIns *ir)
{
  MCode *nilexit = merge_href_niltv(as, ir);  
  RegSet allow = RSET_GPR;
  Reg dest = ra_dest(as, ir, allow);
  Reg tab = ra_alloc1(as, ir->op1, rset_clear(allow, dest));
  Reg key = RID_NONE, tmp = RID_NONE;
  IRIns *irkey = IR(ir->op2);
  int isk = irref_isk(ir->op2);
  IRType1 kt = irkey->t;
  uint32_t khash;
  MCLabel l_end, l_loop, l_next;

  if (!isk) {
    rset_clear(allow, tab);
    key = ra_alloc1(as, ir->op2, irt_isnum(kt) ? RSET_FPR : allow);
    if (!irt_isstr(kt))
      tmp = ra_scratch(as, rset_exclude(allow, key));
  }

  
  l_end = emit_label(as);
  if (nilexit && ir[1].o == IR_NE) {
    emit_jcc(as, CC_E, nilexit);  
    nilexit = NULL;
  } else {
    emit_loada(as, dest, niltvg(J2G(as->J)));
  }

  
  l_loop = emit_sjcc_label(as, CC_NZ);
  emit_rr(as, XO_TEST, dest, dest);
  emit_rmro(as, XO_MOV, dest, dest, offsetof(Node, next));
  l_next = emit_label(as);

  
  if (nilexit)
    emit_jcc(as, CC_E, nilexit);
  else
    emit_sjcc(as, CC_E, l_end);
  if (irt_isnum(kt)) {
    if (isk) {
      
      emit_gmroi(as, XG_ARITHi(XOg_CMP), dest, offsetof(Node, key.u32.lo),
		 (int32_t)ir_knum(irkey)->u32.lo);
      emit_sjcc(as, CC_NE, l_next);
      emit_gmroi(as, XG_ARITHi(XOg_CMP), dest, offsetof(Node, key.u32.hi),
		 (int32_t)ir_knum(irkey)->u32.hi);
    } else {
      emit_sjcc(as, CC_P, l_next);
      emit_rmro(as, XO_UCOMISD, key, dest, offsetof(Node, key.n));
      emit_sjcc(as, CC_AE, l_next);
      
#if LJ_64
      emit_u32(as, LJ_TISNUM);
      emit_rmro(as, XO_ARITHi, XOg_CMP, dest, offsetof(Node, key.it));
#else
      emit_i8(as, LJ_TISNUM);
      emit_rmro(as, XO_ARITHi8, XOg_CMP, dest, offsetof(Node, key.it));
#endif
    }
#if LJ_64
  } else if (irt_islightud(kt)) {
    emit_rmro(as, XO_CMP, key|REX_64, dest, offsetof(Node, key.u64));
#endif
  } else {
    if (!irt_ispri(kt)) {
      lua_assert(irt_isaddr(kt));
      if (isk)
	emit_gmroi(as, XG_ARITHi(XOg_CMP), dest, offsetof(Node, key.gcr),
		   ptr2addr(ir_kgc(irkey)));
      else
	emit_rmro(as, XO_CMP, key, dest, offsetof(Node, key.gcr));
      emit_sjcc(as, CC_NE, l_next);
    }
    lua_assert(!irt_isnil(kt));
    emit_i8(as, irt_toitype(kt));
    emit_rmro(as, XO_ARITHi8, XOg_CMP, dest, offsetof(Node, key.it));
  }
  emit_sfixup(as, l_loop);
  checkmclim(as);

  
  khash = isk ? ir_khash(irkey) : 1;
  if (khash == 0) {
    emit_rmro(as, XO_MOV, dest, tab, offsetof(GCtab, node));
  } else {
    emit_rmro(as, XO_ARITH(XOg_ADD), dest, tab, offsetof(GCtab, node));
    if ((as->flags & JIT_F_PREFER_IMUL)) {
      emit_i8(as, sizeof(Node));
      emit_rr(as, XO_IMULi8, dest, dest);
    } else {
      emit_shifti(as, XOg_SHL, dest, 3);
      emit_rmrxo(as, XO_LEA, dest, dest, dest, XM_SCALE2, 0);
    }
    if (isk) {
      emit_gri(as, XG_ARITHi(XOg_AND), dest, (int32_t)khash);
      emit_rmro(as, XO_MOV, dest, tab, offsetof(GCtab, hmask));
    } else if (irt_isstr(kt)) {
      emit_rmro(as, XO_ARITH(XOg_AND), dest, key, offsetof(GCstr, hash));
      emit_rmro(as, XO_MOV, dest, tab, offsetof(GCtab, hmask));
    } else {  
      emit_rmro(as, XO_ARITH(XOg_AND), dest, tab, offsetof(GCtab, hmask));
      emit_rr(as, XO_ARITH(XOg_SUB), dest, tmp);
      emit_shifti(as, XOg_ROL, tmp, HASH_ROT3);
      emit_rr(as, XO_ARITH(XOg_XOR), dest, tmp);
      emit_shifti(as, XOg_ROL, dest, HASH_ROT2);
      emit_rr(as, XO_ARITH(XOg_SUB), tmp, dest);
      emit_shifti(as, XOg_ROL, dest, HASH_ROT1);
      emit_rr(as, XO_ARITH(XOg_XOR), tmp, dest);
      if (irt_isnum(kt)) {
	emit_rr(as, XO_ARITH(XOg_ADD), dest, dest);
#if LJ_64
	emit_shifti(as, XOg_SHR|REX_64, dest, 32);
	emit_rr(as, XO_MOV, tmp, dest);
	emit_rr(as, XO_MOVDto, key|REX_64, dest);
#else
	emit_rmro(as, XO_MOV, dest, RID_ESP, ra_spill(as, irkey)+4);
	emit_rr(as, XO_MOVDto, key, tmp);
#endif
      } else {
	emit_rr(as, XO_MOV, tmp, key);
	emit_rmro(as, XO_LEA, dest, key, HASH_BIAS);
      }
    }
  }
}

static void asm_hrefk(ASMState *as, IRIns *ir)
{
  IRIns *kslot = IR(ir->op2);
  IRIns *irkey = IR(kslot->op1);
  int32_t ofs = (int32_t)(kslot->op2 * sizeof(Node));
  Reg dest = ra_used(ir) ? ra_dest(as, ir, RSET_GPR) : RID_NONE;
  Reg node = ra_alloc1(as, ir->op1, RSET_GPR);
#if !LJ_64
  MCLabel l_exit;
#endif
  lua_assert(ofs % sizeof(Node) == 0);
  if (ra_hasreg(dest)) {
    if (ofs != 0) {
      if (dest == node && !(as->flags & JIT_F_LEA_AGU))
	emit_gri(as, XG_ARITHi(XOg_ADD), dest, ofs);
      else
	emit_rmro(as, XO_LEA, dest, node, ofs);
    } else if (dest != node) {
      emit_rr(as, XO_MOV, dest, node);
    }
  }
  asm_guardcc(as, CC_NE);
#if LJ_64
  if (!irt_ispri(irkey->t)) {
    Reg key = ra_scratch(as, rset_exclude(RSET_GPR, node));
    emit_rmro(as, XO_CMP, key|REX_64, node,
	       ofs + (int32_t)offsetof(Node, key.u64));
    lua_assert(irt_isnum(irkey->t) || irt_isgcv(irkey->t));
    
    emit_loadu64(as, key, irt_isnum(irkey->t) ? ir_knum(irkey)->u64 :
			  ((uint64_t)irt_toitype(irkey->t) << 32) |
			  (uint64_t)(uint32_t)ptr2addr(ir_kgc(irkey)));
  } else {
    lua_assert(!irt_isnil(irkey->t));
    emit_i8(as, irt_toitype(irkey->t));
    emit_rmro(as, XO_ARITHi8, XOg_CMP, node,
	      ofs + (int32_t)offsetof(Node, key.it));
  }
#else
  l_exit = emit_label(as);
  if (irt_isnum(irkey->t)) {
    
    emit_gmroi(as, XG_ARITHi(XOg_CMP), node,
	       ofs + (int32_t)offsetof(Node, key.u32.lo),
	       (int32_t)ir_knum(irkey)->u32.lo);
    emit_sjcc(as, CC_NE, l_exit);
    emit_gmroi(as, XG_ARITHi(XOg_CMP), node,
	       ofs + (int32_t)offsetof(Node, key.u32.hi),
	       (int32_t)ir_knum(irkey)->u32.hi);
  } else {
    if (!irt_ispri(irkey->t)) {
      lua_assert(irt_isgcv(irkey->t));
      emit_gmroi(as, XG_ARITHi(XOg_CMP), node,
		 ofs + (int32_t)offsetof(Node, key.gcr),
		 ptr2addr(ir_kgc(irkey)));
      emit_sjcc(as, CC_NE, l_exit);
    }
    lua_assert(!irt_isnil(irkey->t));
    emit_i8(as, irt_toitype(irkey->t));
    emit_rmro(as, XO_ARITHi8, XOg_CMP, node,
	      ofs + (int32_t)offsetof(Node, key.it));
  }
#endif
}

static void asm_newref(ASMState *as, IRIns *ir)
{
  const CCallInfo *ci = &lj_ir_callinfo[IRCALL_lj_tab_newkey];
  IRRef args[3];
  IRIns *irkey;
  Reg tmp;
  if (ir->r == RID_SINK)
    return;
  args[0] = ASMREF_L;     
  args[1] = ir->op1;      
  args[2] = ASMREF_TMP1;  
  asm_setupresult(as, ir, ci);  
  asm_gencall(as, ci, args);
  tmp = ra_releasetmp(as, ASMREF_TMP1);
  irkey = IR(ir->op2);
  if (irt_isnum(irkey->t)) {
    
    if (irref_isk(ir->op2))
      emit_loada(as, tmp, ir_knum(irkey));
    else
      emit_rmro(as, XO_LEA, tmp|REX_64, RID_ESP, ra_spill(as, irkey));
  } else {
    
    if (!irref_isk(ir->op2)) {
      Reg src = ra_alloc1(as, ir->op2, rset_exclude(RSET_GPR, tmp));
      emit_movtomro(as, REX_64IR(irkey, src), tmp, 0);
    } else if (!irt_ispri(irkey->t)) {
      emit_movmroi(as, tmp, 0, irkey->i);
    }
    if (!(LJ_64 && irt_islightud(irkey->t)))
      emit_movmroi(as, tmp, 4, irt_toitype(irkey->t));
    emit_loada(as, tmp, &J2G(as->J)->tmptv);
  }
}

static void asm_uref(ASMState *as, IRIns *ir)
{
  Reg dest = ra_dest(as, ir, RSET_GPR);
  if (irref_isk(ir->op1)) {
    GCfunc *fn = ir_kfunc(IR(ir->op1));
    MRef *v = &gcref(fn->l.uvptr[(ir->op2 >> 8)])->uv.v;
    emit_rma(as, XO_MOV, dest, v);
  } else {
    Reg uv = ra_scratch(as, RSET_GPR);
    Reg func = ra_alloc1(as, ir->op1, RSET_GPR);
    if (ir->o == IR_UREFC) {
      emit_rmro(as, XO_LEA, dest, uv, offsetof(GCupval, tv));
      asm_guardcc(as, CC_NE);
      emit_i8(as, 1);
      emit_rmro(as, XO_ARITHib, XOg_CMP, uv, offsetof(GCupval, closed));
    } else {
      emit_rmro(as, XO_MOV, dest, uv, offsetof(GCupval, v));
    }
    emit_rmro(as, XO_MOV, uv, func,
	      (int32_t)offsetof(GCfuncL, uvptr) + 4*(int32_t)(ir->op2 >> 8));
  }
}

static void asm_fref(ASMState *as, IRIns *ir)
{
  Reg dest = ra_dest(as, ir, RSET_GPR);
  asm_fusefref(as, ir, RSET_GPR);
  emit_mrm(as, XO_LEA, dest, RID_MRM);
}

static void asm_strref(ASMState *as, IRIns *ir)
{
  Reg dest = ra_dest(as, ir, RSET_GPR);
  asm_fusestrref(as, ir, RSET_GPR);
  if (as->mrm.base == RID_NONE)
    emit_loadi(as, dest, as->mrm.ofs);
  else if (as->mrm.base == dest && as->mrm.idx == RID_NONE)
    emit_gri(as, XG_ARITHi(XOg_ADD), dest, as->mrm.ofs);
  else
    emit_mrm(as, XO_LEA, dest, RID_MRM);
}



static void asm_fxload(ASMState *as, IRIns *ir)
{
  Reg dest = ra_dest(as, ir, irt_isfp(ir->t) ? RSET_FPR : RSET_GPR);
  x86Op xo;
  if (ir->o == IR_FLOAD)
    asm_fusefref(as, ir, RSET_GPR);
  else
    asm_fusexref(as, ir->op1, RSET_GPR);
  
  switch (irt_type(ir->t)) {
  case IRT_I8: xo = XO_MOVSXb; break;
  case IRT_U8: xo = XO_MOVZXb; break;
  case IRT_I16: xo = XO_MOVSXw; break;
  case IRT_U16: xo = XO_MOVZXw; break;
  case IRT_NUM: xo = XMM_MOVRM(as); break;
  case IRT_FLOAT: xo = XO_MOVSS; break;
  default:
    if (LJ_64 && irt_is64(ir->t))
      dest |= REX_64;
    else
      lua_assert(irt_isint(ir->t) || irt_isu32(ir->t) || irt_isaddr(ir->t));
    xo = XO_MOV;
    break;
  }
  emit_mrm(as, xo, dest, RID_MRM);
}

static void asm_fxstore(ASMState *as, IRIns *ir)
{
  RegSet allow = RSET_GPR;
  Reg src = RID_NONE, osrc = RID_NONE;
  int32_t k = 0;
  if (ir->r == RID_SINK)
    return;
  
  if (irt_isi16(ir->t) || irt_isu16(ir->t) || irt_isfp(ir->t) ||
      !asm_isk32(as, ir->op2, &k)) {
    RegSet allow8 = irt_isfp(ir->t) ? RSET_FPR :
		    (irt_isi8(ir->t) || irt_isu8(ir->t)) ? RSET_GPR8 : RSET_GPR;
    src = osrc = ra_alloc1(as, ir->op2, allow8);
    if (!LJ_64 && !rset_test(allow8, src)) {  
      rset_clear(allow, osrc);
      src = ra_scratch(as, allow8);
    }
    rset_clear(allow, src);
  }
  if (ir->o == IR_FSTORE) {
    asm_fusefref(as, IR(ir->op1), allow);
  } else {
    asm_fusexref(as, ir->op1, allow);
    if (LJ_32 && ir->o == IR_HIOP) as->mrm.ofs += 4;
  }
  if (ra_hasreg(src)) {
    x86Op xo;
    switch (irt_type(ir->t)) {
    case IRT_I8: case IRT_U8: xo = XO_MOVtob; src |= FORCE_REX; break;
    case IRT_I16: case IRT_U16: xo = XO_MOVtow; break;
    case IRT_NUM: xo = XO_MOVSDto; break;
    case IRT_FLOAT: xo = XO_MOVSSto; break;
#if LJ_64
    case IRT_LIGHTUD: lua_assert(0);  
#endif
    default:
      if (LJ_64 && irt_is64(ir->t))
	src |= REX_64;
      else
	lua_assert(irt_isint(ir->t) || irt_isu32(ir->t) || irt_isaddr(ir->t));
      xo = XO_MOVto;
      break;
    }
    emit_mrm(as, xo, src, RID_MRM);
    if (!LJ_64 && src != osrc) {
      ra_noweak(as, osrc);
      emit_rr(as, XO_MOV, src, osrc);
    }
  } else {
    if (irt_isi8(ir->t) || irt_isu8(ir->t)) {
      emit_i8(as, k);
      emit_mrm(as, XO_MOVmib, 0, RID_MRM);
    } else {
      lua_assert(irt_is64(ir->t) || irt_isint(ir->t) || irt_isu32(ir->t) ||
		 irt_isaddr(ir->t));
      emit_i32(as, k);
      emit_mrm(as, XO_MOVmi, REX_64IR(ir, 0), RID_MRM);
    }
  }
}

#if LJ_64
static Reg asm_load_lightud64(ASMState *as, IRIns *ir, int typecheck)
{
  if (ra_used(ir) || typecheck) {
    Reg dest = ra_dest(as, ir, RSET_GPR);
    if (typecheck) {
      Reg tmp = ra_scratch(as, rset_exclude(RSET_GPR, dest));
      asm_guardcc(as, CC_NE);
      emit_i8(as, -2);
      emit_rr(as, XO_ARITHi8, XOg_CMP, tmp);
      emit_shifti(as, XOg_SAR|REX_64, tmp, 47);
      emit_rr(as, XO_MOV, tmp|REX_64, dest);
    }
    return dest;
  } else {
    return RID_NONE;
  }
}
#endif

static void asm_ahuvload(ASMState *as, IRIns *ir)
{
  lua_assert(irt_isnum(ir->t) || irt_ispri(ir->t) || irt_isaddr(ir->t) ||
	     (LJ_DUALNUM && irt_isint(ir->t)));
#if LJ_64
  if (irt_islightud(ir->t)) {
    Reg dest = asm_load_lightud64(as, ir, 1);
    if (ra_hasreg(dest)) {
      asm_fuseahuref(as, ir->op1, RSET_GPR);
      emit_mrm(as, XO_MOV, dest|REX_64, RID_MRM);
    }
    return;
  } else
#endif
  if (ra_used(ir)) {
    RegSet allow = irt_isnum(ir->t) ? RSET_FPR : RSET_GPR;
    Reg dest = ra_dest(as, ir, allow);
    asm_fuseahuref(as, ir->op1, RSET_GPR);
    emit_mrm(as, dest < RID_MAX_GPR ? XO_MOV : XMM_MOVRM(as), dest, RID_MRM);
  } else {
    asm_fuseahuref(as, ir->op1, RSET_GPR);
  }
  
  as->mrm.ofs += 4;
  asm_guardcc(as, irt_isnum(ir->t) ? CC_AE : CC_NE);
  if (LJ_64 && irt_type(ir->t) >= IRT_NUM) {
    lua_assert(irt_isinteger(ir->t) || irt_isnum(ir->t));
    emit_u32(as, LJ_TISNUM);
    emit_mrm(as, XO_ARITHi, XOg_CMP, RID_MRM);
  } else {
    emit_i8(as, irt_toitype(ir->t));
    emit_mrm(as, XO_ARITHi8, XOg_CMP, RID_MRM);
  }
}

static void asm_ahustore(ASMState *as, IRIns *ir)
{
  if (ir->r == RID_SINK)
    return;
  if (irt_isnum(ir->t)) {
    Reg src = ra_alloc1(as, ir->op2, RSET_FPR);
    asm_fuseahuref(as, ir->op1, RSET_GPR);
    emit_mrm(as, XO_MOVSDto, src, RID_MRM);
#if LJ_64
  } else if (irt_islightud(ir->t)) {
    Reg src = ra_alloc1(as, ir->op2, RSET_GPR);
    asm_fuseahuref(as, ir->op1, rset_exclude(RSET_GPR, src));
    emit_mrm(as, XO_MOVto, src|REX_64, RID_MRM);
#endif
  } else {
    IRIns *irr = IR(ir->op2);
    RegSet allow = RSET_GPR;
    Reg src = RID_NONE;
    if (!irref_isk(ir->op2)) {
      src = ra_alloc1(as, ir->op2, allow);
      rset_clear(allow, src);
    }
    asm_fuseahuref(as, ir->op1, allow);
    if (ra_hasreg(src)) {
      emit_mrm(as, XO_MOVto, src, RID_MRM);
    } else if (!irt_ispri(irr->t)) {
      lua_assert(irt_isaddr(ir->t) || (LJ_DUALNUM && irt_isinteger(ir->t)));
      emit_i32(as, irr->i);
      emit_mrm(as, XO_MOVmi, 0, RID_MRM);
    }
    as->mrm.ofs += 4;
    emit_i32(as, (int32_t)irt_toitype(ir->t));
    emit_mrm(as, XO_MOVmi, 0, RID_MRM);
  }
}

static void asm_sload(ASMState *as, IRIns *ir)
{
  int32_t ofs = 8*((int32_t)ir->op1-1) + ((ir->op2 & IRSLOAD_FRAME) ? 4 : 0);
  IRType1 t = ir->t;
  Reg base;
  lua_assert(!(ir->op2 & IRSLOAD_PARENT));  
  lua_assert(irt_isguard(t) || !(ir->op2 & IRSLOAD_TYPECHECK));
  lua_assert(LJ_DUALNUM ||
	     !irt_isint(t) || (ir->op2 & (IRSLOAD_CONVERT|IRSLOAD_FRAME)));
  if ((ir->op2 & IRSLOAD_CONVERT) && irt_isguard(t) && irt_isint(t)) {
    Reg left = ra_scratch(as, RSET_FPR);
    asm_tointg(as, ir, left);  
    base = ra_alloc1(as, REF_BASE, RSET_GPR);
    emit_rmro(as, XMM_MOVRM(as), left, base, ofs);
    t.irt = IRT_NUM;  
#if LJ_64
  } else if (irt_islightud(t)) {
    Reg dest = asm_load_lightud64(as, ir, (ir->op2 & IRSLOAD_TYPECHECK));
    if (ra_hasreg(dest)) {
      base = ra_alloc1(as, REF_BASE, RSET_GPR);
      emit_rmro(as, XO_MOV, dest|REX_64, base, ofs);
    }
    return;
#endif
  } else if (ra_used(ir)) {
    RegSet allow = irt_isnum(t) ? RSET_FPR : RSET_GPR;
    Reg dest = ra_dest(as, ir, allow);
    base = ra_alloc1(as, REF_BASE, RSET_GPR);
    lua_assert(irt_isnum(t) || irt_isint(t) || irt_isaddr(t));
    if ((ir->op2 & IRSLOAD_CONVERT)) {
      t.irt = irt_isint(t) ? IRT_NUM : IRT_INT;  
      emit_rmro(as, irt_isint(t) ? XO_CVTSI2SD : XO_CVTSD2SI, dest, base, ofs);
    } else if (irt_isnum(t)) {
      emit_rmro(as, XMM_MOVRM(as), dest, base, ofs);
    } else {
      emit_rmro(as, XO_MOV, dest, base, ofs);
    }
  } else {
    if (!(ir->op2 & IRSLOAD_TYPECHECK))
      return;  
    base = ra_alloc1(as, REF_BASE, RSET_GPR);
  }
  if ((ir->op2 & IRSLOAD_TYPECHECK)) {
    
    asm_guardcc(as, irt_isnum(t) ? CC_AE : CC_NE);
    if (LJ_64 && irt_type(t) >= IRT_NUM) {
      lua_assert(irt_isinteger(t) || irt_isnum(t));
      emit_u32(as, LJ_TISNUM);
      emit_rmro(as, XO_ARITHi, XOg_CMP, base, ofs+4);
    } else {
      emit_i8(as, irt_toitype(t));
      emit_rmro(as, XO_ARITHi8, XOg_CMP, base, ofs+4);
    }
  }
}



#if LJ_HASFFI
static void asm_cnew(ASMState *as, IRIns *ir)
{
  CTState *cts = ctype_ctsG(J2G(as->J));
  CTypeID ctypeid = (CTypeID)IR(ir->op1)->i;
  CTSize sz = (ir->o == IR_CNEWI || ir->op2 == REF_NIL) ?
	      lj_ctype_size(cts, ctypeid) : (CTSize)IR(ir->op2)->i;
  const CCallInfo *ci = &lj_ir_callinfo[IRCALL_lj_mem_newgco];
  IRRef args[2];
  lua_assert(sz != CTSIZE_INVALID);

  args[0] = ASMREF_L;     
  args[1] = ASMREF_TMP1;  
  as->gcsteps++;
  asm_setupresult(as, ir, ci);  

  
  if (ir->o == IR_CNEWI) {
    RegSet allow = (RSET_GPR & ~RSET_SCRATCH);
#if LJ_64
    Reg r64 = sz == 8 ? REX_64 : 0;
    if (irref_isk(ir->op2)) {
      IRIns *irk = IR(ir->op2);
      uint64_t k = irk->o == IR_KINT64 ? ir_k64(irk)->u64 :
					 (uint64_t)(uint32_t)irk->i;
      if (sz == 4 || checki32((int64_t)k)) {
	emit_i32(as, (int32_t)k);
	emit_rmro(as, XO_MOVmi, r64, RID_RET, sizeof(GCcdata));
      } else {
	emit_movtomro(as, RID_ECX + r64, RID_RET, sizeof(GCcdata));
	emit_loadu64(as, RID_ECX, k);
      }
    } else {
      Reg r = ra_alloc1(as, ir->op2, allow);
      emit_movtomro(as, r + r64, RID_RET, sizeof(GCcdata));
    }
#else
    int32_t ofs = sizeof(GCcdata);
    if (sz == 8) {
      ofs += 4; ir++;
      lua_assert(ir->o == IR_HIOP);
    }
    do {
      if (irref_isk(ir->op2)) {
	emit_movmroi(as, RID_RET, ofs, IR(ir->op2)->i);
      } else {
	Reg r = ra_alloc1(as, ir->op2, allow);
	emit_movtomro(as, r, RID_RET, ofs);
	rset_clear(allow, r);
      }
      if (ofs == sizeof(GCcdata)) break;
      ofs -= 4; ir--;
    } while (1);
#endif
    lua_assert(sz == 4 || sz == 8);
  }

  
  emit_movtomro(as, RID_ECX, RID_RET, offsetof(GCcdata, marked));
  emit_gri(as, XG_ARITHi(XOg_OR), RID_ECX,
	   (int32_t)((~LJ_TCDATA<<8)+(ctypeid<<16)));
  emit_gri(as, XG_ARITHi(XOg_AND), RID_ECX, LJ_GC_WHITES);
  emit_opgl(as, XO_MOVZXb, RID_ECX, gc.currentwhite);

  asm_gencall(as, ci, args);
  emit_loadi(as, ra_releasetmp(as, ASMREF_TMP1), (int32_t)(sz+sizeof(GCcdata)));
}
#else
#define asm_cnew(as, ir)	((void)0)
#endif



static void asm_tbar(ASMState *as, IRIns *ir)
{
  Reg tab = ra_alloc1(as, ir->op1, RSET_GPR);
  Reg tmp = ra_scratch(as, rset_exclude(RSET_GPR, tab));
  MCLabel l_end = emit_label(as);
  emit_movtomro(as, tmp, tab, offsetof(GCtab, gclist));
  emit_setgl(as, tab, gc.grayagain);
  emit_getgl(as, tmp, gc.grayagain);
  emit_i8(as, ~LJ_GC_BLACK);
  emit_rmro(as, XO_ARITHib, XOg_AND, tab, offsetof(GCtab, marked));
  emit_sjcc(as, CC_Z, l_end);
  emit_i8(as, LJ_GC_BLACK);
  emit_rmro(as, XO_GROUP3b, XOg_TEST, tab, offsetof(GCtab, marked));
}

static void asm_obar(ASMState *as, IRIns *ir)
{
  const CCallInfo *ci = &lj_ir_callinfo[IRCALL_lj_gc_barrieruv];
  IRRef args[2];
  MCLabel l_end;
  Reg obj;
  
  lua_assert(IR(ir->op1)->o == IR_UREFC);
  ra_evictset(as, RSET_SCRATCH);
  l_end = emit_label(as);
  args[0] = ASMREF_TMP1;  
  args[1] = ir->op1;      
  asm_gencall(as, ci, args);
  emit_loada(as, ra_releasetmp(as, ASMREF_TMP1), J2G(as->J));
  obj = IR(ir->op1)->r;
  emit_sjcc(as, CC_Z, l_end);
  emit_i8(as, LJ_GC_WHITES);
  if (irref_isk(ir->op2)) {
    GCobj *vp = ir_kgc(IR(ir->op2));
    emit_rma(as, XO_GROUP3b, XOg_TEST, &vp->gch.marked);
  } else {
    Reg val = ra_alloc1(as, ir->op2, rset_exclude(RSET_SCRATCH&RSET_GPR, obj));
    emit_rmro(as, XO_GROUP3b, XOg_TEST, val, (int32_t)offsetof(GChead, marked));
  }
  emit_sjcc(as, CC_Z, l_end);
  emit_i8(as, LJ_GC_BLACK);
  emit_rmro(as, XO_GROUP3b, XOg_TEST, obj,
	    (int32_t)offsetof(GCupval, marked)-(int32_t)offsetof(GCupval, tv));
}




static void asm_x87load(ASMState *as, IRRef ref)
{
  IRIns *ir = IR(ref);
  if (ir->o == IR_KNUM) {
    cTValue *tv = ir_knum(ir);
    if (tvispzero(tv))  
      emit_x87op(as, XI_FLDZ);
    else if (tvispone(tv))
      emit_x87op(as, XI_FLD1);
    else
      emit_rma(as, XO_FLDq, XOg_FLDq, tv);
  } else if (ir->o == IR_CONV && ir->op2 == IRCONV_NUM_INT && !ra_used(ir) &&
	     !irref_isk(ir->op1) && mayfuse(as, ir->op1)) {
    IRIns *iri = IR(ir->op1);
    emit_rmro(as, XO_FILDd, XOg_FILDd, RID_ESP, ra_spill(as, iri));
  } else {
    emit_mrm(as, XO_FLDq, XOg_FLDq, asm_fuseload(as, ref, RSET_EMPTY));
  }
}


static int fpmjoin_pow(ASMState *as, IRIns *ir)
{
  IRIns *irp = IR(ir->op1);
  if (irp == ir-1 && irp->o == IR_MUL && !ra_used(irp)) {
    IRIns *irpp = IR(irp->op1);
    if (irpp == ir-2 && irpp->o == IR_FPMATH &&
	irpp->op2 == IRFPM_LOG2 && !ra_used(irpp)) {
      
      RegSet drop = RSET_RANGE(RID_XMM0, RID_XMM2+1)|RID2RSET(RID_EAX);
      IRIns *irx;
      if (ra_hasreg(ir->r))
	rset_clear(drop, ir->r);  
      ra_evictset(as, drop);
      ra_destreg(as, ir, RID_XMM0);
      emit_call(as, lj_vm_pow_sse);
      irx = IR(irpp->op1);
      if (ra_noreg(irx->r) && ra_gethint(irx->r) == RID_XMM1)
	irx->r = RID_INIT;  
      ra_left(as, RID_XMM0, irpp->op1);
      ra_left(as, RID_XMM1, irp->op2);
      return 1;
    }
  }
  return 0;
}

static void asm_fpmath(ASMState *as, IRIns *ir)
{
  IRFPMathOp fpm = ir->o == IR_FPMATH ? (IRFPMathOp)ir->op2 : IRFPM_OTHER;
  if (fpm == IRFPM_SQRT) {
    Reg dest = ra_dest(as, ir, RSET_FPR);
    Reg left = asm_fuseload(as, ir->op1, RSET_FPR);
    emit_mrm(as, XO_SQRTSD, dest, left);
  } else if (fpm <= IRFPM_TRUNC) {
    if (as->flags & JIT_F_SSE4_1) {  
      Reg dest = ra_dest(as, ir, RSET_FPR);
      Reg left = asm_fuseload(as, ir->op1, RSET_FPR);
      
      
      emit_i8(as, 0x09 + fpm);
      emit_mrm(as, XO_ROUNDSD, dest, left);
      if (LJ_64 && as->mcp[1] != (MCode)(XO_ROUNDSD >> 16)) {
	as->mcp[0] = as->mcp[1]; as->mcp[1] = 0x0f;  
      }
      *--as->mcp = 0x66;  
    } else {  
      
      RegSet drop = RSET_RANGE(RID_XMM0, RID_XMM3+1)|RID2RSET(RID_EAX);
      if (ra_hasreg(ir->r))
	rset_clear(drop, ir->r);  
      ra_evictset(as, drop);
      ra_destreg(as, ir, RID_XMM0);
      emit_call(as, fpm == IRFPM_FLOOR ? lj_vm_floor_sse :
		    fpm == IRFPM_CEIL ? lj_vm_ceil_sse : lj_vm_trunc_sse);
      ra_left(as, RID_XMM0, ir->op1);
    }
  } else if (fpm == IRFPM_EXP2 && fpmjoin_pow(as, ir)) {
    
  } else {  
    int32_t ofs = sps_scale(ir->s);  
    Reg dest = ir->r;
    if (ra_hasreg(dest)) {
      ra_free(as, dest);
      ra_modified(as, dest);
      emit_rmro(as, XMM_MOVRM(as), dest, RID_ESP, ofs);
    }
    emit_rmro(as, XO_FSTPq, XOg_FSTPq, RID_ESP, ofs);
    switch (fpm) {  
    case IRFPM_EXP: emit_call(as, lj_vm_exp_x87); break;
    case IRFPM_EXP2: emit_call(as, lj_vm_exp2_x87); break;
    case IRFPM_SIN: emit_x87op(as, XI_FSIN); break;
    case IRFPM_COS: emit_x87op(as, XI_FCOS); break;
    case IRFPM_TAN: emit_x87op(as, XI_FPOP); emit_x87op(as, XI_FPTAN); break;
    case IRFPM_LOG: case IRFPM_LOG2: case IRFPM_LOG10:
      
      emit_x87op(as, XI_FYL2X); break;
    case IRFPM_OTHER:
      switch (ir->o) {
      case IR_ATAN2:
	emit_x87op(as, XI_FPATAN); asm_x87load(as, ir->op2); break;
      case IR_LDEXP:
	emit_x87op(as, XI_FPOP1); emit_x87op(as, XI_FSCALE); break;
      default: lua_assert(0); break;
      }
      break;
    default: lua_assert(0); break;
    }
    asm_x87load(as, ir->op1);
    switch (fpm) {
    case IRFPM_LOG: emit_x87op(as, XI_FLDLN2); break;
    case IRFPM_LOG2: emit_x87op(as, XI_FLD1); break;
    case IRFPM_LOG10: emit_x87op(as, XI_FLDLG2); break;
    case IRFPM_OTHER:
      if (ir->o == IR_LDEXP) asm_x87load(as, ir->op2);
      break;
    default: break;
    }
  }
}

static void asm_fppowi(ASMState *as, IRIns *ir)
{
  
  RegSet drop = RSET_RANGE(RID_XMM0, RID_XMM1+1)|RID2RSET(RID_EAX);
  if (ra_hasreg(ir->r))
    rset_clear(drop, ir->r);  
  ra_evictset(as, drop);
  ra_destreg(as, ir, RID_XMM0);
  emit_call(as, lj_vm_powi_sse);
  ra_left(as, RID_XMM0, ir->op1);
  ra_left(as, RID_EAX, ir->op2);
}

#if LJ_64 && LJ_HASFFI
static void asm_arith64(ASMState *as, IRIns *ir, IRCallID id)
{
  const CCallInfo *ci = &lj_ir_callinfo[id];
  IRRef args[2];
  args[0] = ir->op1;
  args[1] = ir->op2;
  asm_setupresult(as, ir, ci);
  asm_gencall(as, ci, args);
}
#endif

static void asm_intmod(ASMState *as, IRIns *ir)
{
  const CCallInfo *ci = &lj_ir_callinfo[IRCALL_lj_vm_modi];
  IRRef args[2];
  args[0] = ir->op1;
  args[1] = ir->op2;
  asm_setupresult(as, ir, ci);
  asm_gencall(as, ci, args);
}

static int asm_swapops(ASMState *as, IRIns *ir)
{
  IRIns *irl = IR(ir->op1);
  IRIns *irr = IR(ir->op2);
  lua_assert(ra_noreg(irr->r));
  if (!irm_iscomm(lj_ir_mode[ir->o]))
    return 0;  
  if (irref_isk(ir->op2))
    return 0;  
  if (ra_hasreg(irl->r))
    return 1;  
  if (ra_samehint(ir->r, irr->r))
    return 1;  
  if (as->curins > as->loopref) {  
    if (ir->op2 < as->loopref && !irt_isphi(irr->t))
      return 0;  
    if (ir->op1 < as->loopref && !irt_isphi(irl->t))
      return 1;  
  }
  if (opisfusableload(irl->o))
    return 1;  
  return 0;  
}

static void asm_fparith(ASMState *as, IRIns *ir, x86Op xo)
{
  IRRef lref = ir->op1;
  IRRef rref = ir->op2;
  RegSet allow = RSET_FPR;
  Reg dest;
  Reg right = IR(rref)->r;
  if (ra_hasreg(right)) {
    rset_clear(allow, right);
    ra_noweak(as, right);
  }
  dest = ra_dest(as, ir, allow);
  if (lref == rref) {
    right = dest;
  } else if (ra_noreg(right)) {
    if (asm_swapops(as, ir)) {
      IRRef tmp = lref; lref = rref; rref = tmp;
    }
    right = asm_fuseload(as, rref, rset_clear(allow, dest));
  }
  emit_mrm(as, xo, dest, right);
  ra_left(as, dest, lref);
}

static void asm_intarith(ASMState *as, IRIns *ir, x86Arith xa)
{
  IRRef lref = ir->op1;
  IRRef rref = ir->op2;
  RegSet allow = RSET_GPR;
  Reg dest, right;
  int32_t k = 0;
  if (as->flagmcp == as->mcp) {  
    MCode *p = as->mcp + ((LJ_64 && *as->mcp < XI_TESTb) ? 3 : 2);
    if ((p[1] & 15) < 14) {
      if ((p[1] & 15) >= 12) p[1] -= 4;  
      as->flagmcp = NULL;
      as->mcp = p;
    }  
  }
  right = IR(rref)->r;
  if (ra_hasreg(right)) {
    rset_clear(allow, right);
    ra_noweak(as, right);
  }
  dest = ra_dest(as, ir, allow);
  if (lref == rref) {
    right = dest;
  } else if (ra_noreg(right) && !asm_isk32(as, rref, &k)) {
    if (asm_swapops(as, ir)) {
      IRRef tmp = lref; lref = rref; rref = tmp;
    }
    right = asm_fuseloadm(as, rref, rset_clear(allow, dest), irt_is64(ir->t));
  }
  if (irt_isguard(ir->t))  
    asm_guardcc(as, CC_O);
  if (xa != XOg_X_IMUL) {
    if (ra_hasreg(right))
      emit_mrm(as, XO_ARITH(xa), REX_64IR(ir, dest), right);
    else
      emit_gri(as, XG_ARITHi(xa), REX_64IR(ir, dest), k);
  } else if (ra_hasreg(right)) {  
    emit_mrm(as, XO_IMUL, REX_64IR(ir, dest), right);
  } else {  
    
    Reg left = asm_fuseloadm(as, lref, RSET_GPR, irt_is64(ir->t));
    x86Op xo;
    if (checki8(k)) { emit_i8(as, k); xo = XO_IMULi8;
    } else { emit_i32(as, k); xo = XO_IMULi; }
    emit_mrm(as, xo, REX_64IR(ir, dest), left);
    return;
  }
  ra_left(as, dest, lref);
}


static int asm_lea(ASMState *as, IRIns *ir)
{
  IRIns *irl = IR(ir->op1);
  IRIns *irr = IR(ir->op2);
  RegSet allow = RSET_GPR;
  Reg dest;
  as->mrm.base = as->mrm.idx = RID_NONE;
  as->mrm.scale = XM_SCALE1;
  as->mrm.ofs = 0;
  if (ra_hasreg(irl->r)) {
    rset_clear(allow, irl->r);
    ra_noweak(as, irl->r);
    as->mrm.base = irl->r;
    if (irref_isk(ir->op2) || ra_hasreg(irr->r)) {
      
      if (ra_hasreg(ir->r) &&
	  ((irt_isphi(irl->t) && as->phireg[ir->r] == ir->op1) ||
	   (irt_isphi(irr->t) && as->phireg[ir->r] == ir->op2)))
	return 0;
      if (irref_isk(ir->op2)) {
	as->mrm.ofs = irr->i;
      } else {
	rset_clear(allow, irr->r);
	ra_noweak(as, irr->r);
	as->mrm.idx = irr->r;
      }
    } else if (irr->o == IR_ADD && mayfuse(as, ir->op2) &&
	       irref_isk(irr->op2)) {
      Reg idx = ra_alloc1(as, irr->op1, allow);
      rset_clear(allow, idx);
      as->mrm.idx = (uint8_t)idx;
      as->mrm.ofs = IR(irr->op2)->i;
    } else {
      return 0;
    }
  } else if (ir->op1 != ir->op2 && irl->o == IR_ADD && mayfuse(as, ir->op1) &&
	     (irref_isk(ir->op2) || irref_isk(irl->op2))) {
    Reg idx, base = ra_alloc1(as, irl->op1, allow);
    rset_clear(allow, base);
    as->mrm.base = (uint8_t)base;
    if (irref_isk(ir->op2)) {
      as->mrm.ofs = irr->i;
      idx = ra_alloc1(as, irl->op2, allow);
    } else {
      as->mrm.ofs = IR(irl->op2)->i;
      idx = ra_alloc1(as, ir->op2, allow);
    }
    rset_clear(allow, idx);
    as->mrm.idx = (uint8_t)idx;
  } else {
    return 0;
  }
  dest = ra_dest(as, ir, allow);
  emit_mrm(as, XO_LEA, dest, RID_MRM);
  return 1;  
}

static void asm_add(ASMState *as, IRIns *ir)
{
  if (irt_isnum(ir->t))
    asm_fparith(as, ir, XO_ADDSD);
  else if ((as->flags & JIT_F_LEA_AGU) || as->flagmcp == as->mcp ||
	   irt_is64(ir->t) || !asm_lea(as, ir))
    asm_intarith(as, ir, XOg_ADD);
}

static void asm_neg_not(ASMState *as, IRIns *ir, x86Group3 xg)
{
  Reg dest = ra_dest(as, ir, RSET_GPR);
  emit_rr(as, XO_GROUP3, REX_64IR(ir, xg), dest);
  ra_left(as, dest, ir->op1);
}

static void asm_min_max(ASMState *as, IRIns *ir, int cc)
{
  Reg right, dest = ra_dest(as, ir, RSET_GPR);
  IRRef lref = ir->op1, rref = ir->op2;
  if (irref_isk(rref)) { lref = rref; rref = ir->op1; }
  right = ra_alloc1(as, rref, rset_exclude(RSET_GPR, dest));
  emit_rr(as, XO_CMOV + (cc<<24), REX_64IR(ir, dest), right);
  emit_rr(as, XO_CMP, REX_64IR(ir, dest), right);
  ra_left(as, dest, lref);
}

static void asm_bitswap(ASMState *as, IRIns *ir)
{
  Reg dest = ra_dest(as, ir, RSET_GPR);
  as->mcp = emit_op(XO_BSWAP + ((dest&7) << 24),
		    REX_64IR(ir, 0), dest, 0, as->mcp, 1);
  ra_left(as, dest, ir->op1);
}

static void asm_bitshift(ASMState *as, IRIns *ir, x86Shift xs)
{
  IRRef rref = ir->op2;
  IRIns *irr = IR(rref);
  Reg dest;
  if (irref_isk(rref)) {  
    int shift;
    dest = ra_dest(as, ir, RSET_GPR);
    shift = irr->i & (irt_is64(ir->t) ? 63 : 31);
    switch (shift) {
    case 0: break;
    case 1: emit_rr(as, XO_SHIFT1, REX_64IR(ir, xs), dest); break;
    default: emit_shifti(as, REX_64IR(ir, xs), dest, shift); break;
    }
  } else {  
    Reg right;
    dest = ra_dest(as, ir, rset_exclude(RSET_GPR, RID_ECX));
    if (dest == RID_ECX) {
      dest = ra_scratch(as, rset_exclude(RSET_GPR, RID_ECX));
      emit_rr(as, XO_MOV, RID_ECX, dest);
    }
    right = irr->r;
    if (ra_noreg(right))
      right = ra_allocref(as, rref, RID2RSET(RID_ECX));
    else if (right != RID_ECX)
      ra_scratch(as, RID2RSET(RID_ECX));
    emit_rr(as, XO_SHIFTcl, REX_64IR(ir, xs), dest);
    ra_noweak(as, right);
    if (right != RID_ECX)
      emit_rr(as, XO_MOV, RID_ECX, right);
  }
  ra_left(as, dest, ir->op1);
  
}




#define VCC_U	0x1000		
#define VCC_P	0x2000		
#define VCC_S	0x4000		
#define VCC_PS	(VCC_P|VCC_S)


#define COMPFLAGS(ci, cin, cu, cf)	((ci)+((cu)<<4)+((cin)<<8)+(cf))
static const uint16_t asm_compmap[IR_ABC+1] = {
  
   COMPFLAGS(CC_GE, CC_G,  CC_AE, VCC_PS),
   COMPFLAGS(CC_L,  CC_L,  CC_B,  0),
   COMPFLAGS(CC_G,  CC_G,  CC_A,  VCC_PS),
   COMPFLAGS(CC_LE, CC_L,  CC_BE, 0),
   COMPFLAGS(CC_AE, CC_A,  CC_AE, VCC_U),
   COMPFLAGS(CC_B,  CC_B,  CC_B,  VCC_U|VCC_PS),
   COMPFLAGS(CC_A,  CC_A,  CC_A,  VCC_U),
   COMPFLAGS(CC_BE, CC_B,  CC_BE, VCC_U|VCC_PS),
   COMPFLAGS(CC_NE, CC_NE, CC_NE, VCC_P),
   COMPFLAGS(CC_E,  CC_E,  CC_E,  VCC_U|VCC_P),
   COMPFLAGS(CC_BE, CC_B,  CC_BE, VCC_U|VCC_PS)  
};


static void asm_comp(ASMState *as, IRIns *ir, uint32_t cc)
{
  if (irt_isnum(ir->t)) {
    IRRef lref = ir->op1;
    IRRef rref = ir->op2;
    Reg left, right;
    MCLabel l_around;
    
    if (cc & VCC_S) {  
      IRRef tmp = lref; lref = rref; rref = tmp;
      cc ^= (VCC_PS|(5<<4));  
    }
    left = ra_alloc1(as, lref, RSET_FPR);
    right = asm_fuseload(as, rref, rset_exclude(RSET_FPR, left));
    l_around = emit_label(as);
    asm_guardcc(as, cc >> 4);
    if (cc & VCC_P) {  
      if (!(cc & VCC_U)) {
	asm_guardcc(as, CC_P);  
      } else if (l_around != as->invmcp) {
	emit_sjcc(as, CC_P, l_around);  
      } else {
	
	as->loopinv = 2;
	if (as->realign)
	  emit_sjcc(as, CC_P, as->mcp);
	else
	  emit_jcc(as, CC_P, as->mcp);
      }
    }
    emit_mrm(as, XO_UCOMISD, left, right);
  } else {
    IRRef lref = ir->op1, rref = ir->op2;
    IROp leftop = (IROp)(IR(lref)->o);
    Reg r64 = REX_64IR(ir, 0);
    int32_t imm = 0;
    lua_assert(irt_is64(ir->t) || irt_isint(ir->t) ||
	       irt_isu32(ir->t) || irt_isaddr(ir->t) || irt_isu8(ir->t));
    
    if (irref_isk(lref) || (!irref_isk(rref) && opisfusableload(leftop))) {
      if ((cc & 0xc) == 0xc) cc ^= 0x53;  
      else if ((cc & 0xa) == 0x2) cc ^= 0x55;  
      lref = ir->op2; rref = ir->op1;
    }
    if (asm_isk32(as, rref, &imm)) {
      IRIns *irl = IR(lref);
      
      int usetest = (imm == 0 && (cc & 0xa) != 0x2);
      if (usetest && irl->o == IR_BAND && irl+1 == ir && !ra_used(irl)) {
	
	Reg right, left = RID_NONE;
	RegSet allow = RSET_GPR;
	if (!asm_isk32(as, irl->op2, &imm)) {
	  left = ra_alloc1(as, irl->op2, allow);
	  rset_clear(allow, left);
	} else {  
	  IRIns *irll = IR(irl->op1);
	  if (opisfusableload((IROp)irll->o) &&
	      (irt_isi8(irll->t) || irt_isu8(irll->t))) {
	    IRType1 origt = irll->t;  
	    irll->t.irt = (irll->t.irt & ~IRT_TYPE) | IRT_INT;
	    as->curins--;  
	    right = asm_fuseload(as, irl->op1, RSET_GPR);
	    as->curins++;
	    irll->t = origt;
	    if (right != RID_MRM) goto test_nofuse;
	    
	    asm_guardcc(as, cc);
	    emit_i8(as, (imm & 0xff));
	    emit_mrm(as, XO_GROUP3b, XOg_TEST, RID_MRM);
	    return;
	  }
	}
	as->curins--;  
	right = asm_fuseloadm(as, irl->op1, allow, r64);
	as->curins++;  
      test_nofuse:
	asm_guardcc(as, cc);
	if (ra_noreg(left)) {
	  emit_i32(as, imm);
	  emit_mrm(as, XO_GROUP3, r64 + XOg_TEST, right);
	} else {
	  emit_mrm(as, XO_TEST, r64 + left, right);
	}
      } else {
	Reg left;
	if (opisfusableload((IROp)irl->o) &&
	    ((irt_isu8(irl->t) && checku8(imm)) ||
	     ((irt_isi8(irl->t) || irt_isi16(irl->t)) && checki8(imm)) ||
	     (irt_isu16(irl->t) && checku16(imm) && checki8((int16_t)imm)))) {
	  
	  IRType1 origt = irl->t;  
	  irl->t.irt = (irl->t.irt & ~IRT_TYPE) | IRT_INT;
	  left = asm_fuseload(as, lref, RSET_GPR);
	  irl->t = origt;
	  if (left == RID_MRM) {  
	    if (irt_isu8(irl->t) || irt_isu16(irl->t))
	      cc >>= 4;  
	    asm_guardcc(as, cc);
	    emit_i8(as, imm);
	    emit_mrm(as, (irt_isi8(origt) || irt_isu8(origt)) ?
			 XO_ARITHib : XO_ARITHiw8, r64 + XOg_CMP, RID_MRM);
	    return;
	  }  
	} else {
	  left = asm_fuseloadm(as, lref,
			       irt_isu8(ir->t) ? RSET_GPR8 : RSET_GPR, r64);
	}
	asm_guardcc(as, cc);
	if (usetest && left != RID_MRM) {
	  
	  x86Op xo = XO_TEST;
	  if (irt_isu8(ir->t)) {
	    lua_assert(ir->o == IR_EQ || ir->o == IR_NE);
	    xo = XO_TESTb;
	    if (!rset_test(RSET_RANGE(RID_EAX, RID_EBX+1), left)) {
	      if (LJ_64) {
		left |= FORCE_REX;
	      } else {
		emit_i32(as, 0xff);
		emit_mrm(as, XO_GROUP3, XOg_TEST, left);
		return;
	      }
	    }
	  }
	  emit_rr(as, xo, r64 + left, left);
	  if (irl+1 == ir)  
	    as->flagmcp = as->mcp;  
	} else {
	  emit_gmrmi(as, XG_ARITHi(XOg_CMP), r64 + left, imm);
	}
      }
    } else {
      Reg left = ra_alloc1(as, lref, RSET_GPR);
      Reg right = asm_fuseloadm(as, rref, rset_exclude(RSET_GPR, left), r64);
      asm_guardcc(as, cc);
      emit_mrm(as, XO_CMP, r64 + left, right);
    }
  }
}

#if LJ_32 && LJ_HASFFI

static void asm_comp_int64(ASMState *as, IRIns *ir)
{
  uint32_t cc = asm_compmap[(ir-1)->o];
  RegSet allow = RSET_GPR;
  Reg lefthi = RID_NONE, leftlo = RID_NONE;
  Reg righthi = RID_NONE, rightlo = RID_NONE;
  MCLabel l_around;
  x86ModRM mrm;

  as->curins--;  

  
  if (irref_isk(ir->op2)) {
    lefthi = asm_fuseload(as, ir->op1, allow);
  } else {
    lefthi = ra_alloc1(as, ir->op1, allow);
    rset_clear(allow, lefthi);
    righthi = asm_fuseload(as, ir->op2, allow);
    if (righthi == RID_MRM) {
      if (as->mrm.base != RID_NONE) rset_clear(allow, as->mrm.base);
      if (as->mrm.idx != RID_NONE) rset_clear(allow, as->mrm.idx);
    } else {
      rset_clear(allow, righthi);
    }
  }
  mrm = as->mrm;  

  
  if (irref_isk((ir-1)->op2)) {
    leftlo = asm_fuseload(as, (ir-1)->op1, allow);
  } else {
    leftlo = ra_alloc1(as, (ir-1)->op1, allow);
    rset_clear(allow, leftlo);
    rightlo = asm_fuseload(as, (ir-1)->op2, allow);
  }

  
  l_around = emit_label(as);
  as->invmcp = as->flagmcp = NULL;  

  
  asm_guardcc(as, cc >> 4);  
  if (ra_noreg(rightlo)) {
    int32_t imm = IR((ir-1)->op2)->i;
    if (imm == 0 && ((cc >> 4) & 0xa) != 0x2 && leftlo != RID_MRM)
      emit_rr(as, XO_TEST, leftlo, leftlo);
    else
      emit_gmrmi(as, XG_ARITHi(XOg_CMP), leftlo, imm);
  } else {
    emit_mrm(as, XO_CMP, leftlo, rightlo);
  }

  
  if ((cc & 15) != CC_NE)
    emit_sjcc(as, CC_NE, l_around);  
  if ((cc & 15) != CC_E)
    asm_guardcc(as, cc >> 8);  
  as->mrm = mrm;  
  if (ra_noreg(righthi)) {
    int32_t imm = IR(ir->op2)->i;
    if (imm == 0 && (cc & 0xa) != 0x2 && lefthi != RID_MRM)
      emit_rr(as, XO_TEST, lefthi, lefthi);
    else
      emit_gmrmi(as, XG_ARITHi(XOg_CMP), lefthi, imm);
  } else {
    emit_mrm(as, XO_CMP, lefthi, righthi);
  }
}
#endif




static void asm_hiop(ASMState *as, IRIns *ir)
{
#if LJ_32 && LJ_HASFFI
  
  int uselo = ra_used(ir-1), usehi = ra_used(ir);  
  if (LJ_UNLIKELY(!(as->flags & JIT_F_OPT_DCE))) uselo = usehi = 1;
  if ((ir-1)->o == IR_CONV) {  
    if (usehi || uselo) {
      if (irt_isfp(ir->t))
	asm_conv_fp_int64(as, ir);
      else
	asm_conv_int64_fp(as, ir);
    }
    as->curins--;  
    return;
  } else if ((ir-1)->o <= IR_NE) {  
    asm_comp_int64(as, ir);
    return;
  } else if ((ir-1)->o == IR_XSTORE) {
    if ((ir-1)->r != RID_SINK)
      asm_fxstore(as, ir);
    return;
  }
  if (!usehi) return;  
  switch ((ir-1)->o) {
  case IR_ADD:
    as->flagmcp = NULL;
    as->curins--;
    asm_intarith(as, ir, XOg_ADC);
    asm_intarith(as, ir-1, XOg_ADD);
    break;
  case IR_SUB:
    as->flagmcp = NULL;
    as->curins--;
    asm_intarith(as, ir, XOg_SBB);
    asm_intarith(as, ir-1, XOg_SUB);
    break;
  case IR_NEG: {
    Reg dest = ra_dest(as, ir, RSET_GPR);
    emit_rr(as, XO_GROUP3, XOg_NEG, dest);
    emit_i8(as, 0);
    emit_rr(as, XO_ARITHi8, XOg_ADC, dest);
    ra_left(as, dest, ir->op1);
    as->curins--;
    asm_neg_not(as, ir-1, XOg_NEG);
    break;
    }
  case IR_CALLN:
  case IR_CALLXS:
    if (!uselo)
      ra_allocref(as, ir->op1, RID2RSET(RID_RETLO));  
    break;
  case IR_CNEWI:
    
    break;
  default: lua_assert(0); break;
  }
#else
  UNUSED(as); UNUSED(ir); lua_assert(0);  
#endif
}




static void asm_stack_check(ASMState *as, BCReg topslot,
			    IRIns *irp, RegSet allow, ExitNo exitno)
{
  
  Reg pbase = irp ? irp->r : RID_BASE;
  Reg r = allow ? rset_pickbot(allow) : RID_EAX;
  emit_jcc(as, CC_B, exitstub_addr(as->J, exitno));
  if (allow == RSET_EMPTY)  
    emit_rmro(as, XO_MOV, r|REX_64, RID_ESP, 0);
  else
    ra_modified(as, r);
  emit_gri(as, XG_ARITHi(XOg_CMP), r, (int32_t)(8*topslot));
  if (ra_hasreg(pbase) && pbase != r)
    emit_rr(as, XO_ARITH(XOg_SUB), r, pbase);
  else
    emit_rmro(as, XO_ARITH(XOg_SUB), r, RID_NONE,
	      ptr2addr(&J2G(as->J)->jit_base));
  emit_rmro(as, XO_MOV, r, r, offsetof(lua_State, maxstack));
  emit_getgl(as, r, jit_L);
  if (allow == RSET_EMPTY)  
    emit_rmro(as, XO_MOVto, r|REX_64, RID_ESP, 0);
}


static void asm_stack_restore(ASMState *as, SnapShot *snap)
{
  SnapEntry *map = &as->T->snapmap[snap->mapofs];
  SnapEntry *flinks = &as->T->snapmap[snap_nextofs(as->T, snap)-1];
  MSize n, nent = snap->nent;
  
  for (n = 0; n < nent; n++) {
    SnapEntry sn = map[n];
    BCReg s = snap_slot(sn);
    int32_t ofs = 8*((int32_t)s-1);
    IRRef ref = snap_ref(sn);
    IRIns *ir = IR(ref);
    if ((sn & SNAP_NORESTORE))
      continue;
    if (irt_isnum(ir->t)) {
      Reg src = ra_alloc1(as, ref, RSET_FPR);
      emit_rmro(as, XO_MOVSDto, src, RID_BASE, ofs);
    } else {
      lua_assert(irt_ispri(ir->t) || irt_isaddr(ir->t) ||
		 (LJ_DUALNUM && irt_isinteger(ir->t)));
      if (!irref_isk(ref)) {
	Reg src = ra_alloc1(as, ref, rset_exclude(RSET_GPR, RID_BASE));
	emit_movtomro(as, REX_64IR(ir, src), RID_BASE, ofs);
      } else if (!irt_ispri(ir->t)) {
	emit_movmroi(as, RID_BASE, ofs, ir->i);
      }
      if ((sn & (SNAP_CONT|SNAP_FRAME))) {
	if (s != 0)  
	  emit_movmroi(as, RID_BASE, ofs+4, (int32_t)(*flinks--));
      } else {
	if (!(LJ_64 && irt_islightud(ir->t)))
	  emit_movmroi(as, RID_BASE, ofs+4, irt_toitype(ir->t));
      }
    }
    checkmclim(as);
  }
  lua_assert(map + nent == flinks);
}




static void asm_gc_check(ASMState *as)
{
  const CCallInfo *ci = &lj_ir_callinfo[IRCALL_lj_gc_step_jit];
  IRRef args[2];
  MCLabel l_end;
  Reg tmp;
  ra_evictset(as, RSET_SCRATCH);
  l_end = emit_label(as);
  
  asm_guardcc(as, CC_NE);  
  emit_rr(as, XO_TEST, RID_RET, RID_RET);
  args[0] = ASMREF_TMP1;  
  args[1] = ASMREF_TMP2;  
  asm_gencall(as, ci, args);
  tmp = ra_releasetmp(as, ASMREF_TMP1);
  emit_loada(as, tmp, J2G(as->J));
  emit_loadi(as, ra_releasetmp(as, ASMREF_TMP2), as->gcsteps);
  
  emit_sjcc(as, CC_B, l_end);
  emit_opgl(as, XO_ARITH(XOg_CMP), tmp, gc.threshold);
  emit_getgl(as, tmp, gc.total);
  as->gcsteps = 0;
  checkmclim(as);
}




static void asm_loop_fixup(ASMState *as)
{
  MCode *p = as->mctop;
  MCode *target = as->mcp;
  if (as->realign) {  
    as->realign = NULL;  
    lua_assert(((intptr_t)target & 15) == 0);
    if (as->loopinv) {  
      p -= 5;
      p[0] = XI_JMP;
      lua_assert(target - p >= -128);
      p[-1] = (MCode)(target - p);  
      if (as->loopinv == 2)
	p[-3] = (MCode)(target - p + 2);  
    } else {
      lua_assert(target - p >= -128);
      p[-1] = (MCode)(int8_t)(target - p);  
      p[-2] = XI_JMPs;
    }
  } else {
    MCode *newloop;
    p[-5] = XI_JMP;
    if (as->loopinv) {  
      
      p -= 5;
      newloop = target+4;
      *(int32_t *)(p-4) = (int32_t)(target - p);  
      if (as->loopinv == 2) {
	*(int32_t *)(p-10) = (int32_t)(target - p + 6);  
	newloop = target+8;
      }
    } else {  
      *(int32_t *)(p-4) = (int32_t)(target - p);
      newloop = target+3;
    }
    
    if (newloop >= p - 128) {
      as->realign = newloop;  
      as->curins = as->stopins;  
      as->T->nins = as->orignins;  
    }
  }
}




static void asm_head_root_base(ASMState *as)
{
  IRIns *ir = IR(REF_BASE);
  Reg r = ir->r;
  if (ra_hasreg(r)) {
    ra_free(as, r);
    if (rset_test(as->modset, r) || irt_ismarked(ir->t))
      ir->r = RID_INIT;  
    if (r != RID_BASE)
      emit_rr(as, XO_MOV, r, RID_BASE);
  }
}


static RegSet asm_head_side_base(ASMState *as, IRIns *irp, RegSet allow)
{
  IRIns *ir = IR(REF_BASE);
  Reg r = ir->r;
  if (ra_hasreg(r)) {
    ra_free(as, r);
    if (rset_test(as->modset, r) || irt_ismarked(ir->t))
      ir->r = RID_INIT;  
    if (irp->r == r) {
      rset_clear(allow, r);  
    } else if (ra_hasreg(irp->r) && rset_test(as->freeset, irp->r)) {
      rset_clear(allow, irp->r);
      emit_rr(as, XO_MOV, r, irp->r);  
    } else {
      emit_getgl(as, r, jit_base);  
    }
  }
  return allow;
}




static void asm_tail_fixup(ASMState *as, TraceNo lnk)
{
  
  MCode *p = as->mctop;
  MCode *target, *q;
  int32_t spadj = as->T->spadjust;
  if (spadj == 0) {
    p -= ((as->flags & JIT_F_LEA_AGU) ? 7 : 6) + (LJ_64 ? 1 : 0);
  } else {
    MCode *p1;
    
    if (checki8(spadj)) {
      p -= 3;
      p1 = p-6;
      *p1 = (MCode)spadj;
    } else {
      p1 = p-9;
      *(int32_t *)p1 = spadj;
    }
    if ((as->flags & JIT_F_LEA_AGU)) {
#if LJ_64
      p1[-4] = 0x48;
#endif
      p1[-3] = (MCode)XI_LEA;
      p1[-2] = MODRM(checki8(spadj) ? XM_OFS8 : XM_OFS32, RID_ESP, RID_ESP);
      p1[-1] = MODRM(XM_SCALE1, RID_ESP, RID_ESP);
    } else {
#if LJ_64
      p1[-3] = 0x48;
#endif
      p1[-2] = (MCode)(checki8(spadj) ? XI_ARITHi8 : XI_ARITHi);
      p1[-1] = MODRM(XM_REG, XOg_ADD, RID_ESP);
    }
  }
  
  target = lnk ? traceref(as->J, lnk)->mcode : (MCode *)lj_vm_exit_interp;
  *(int32_t *)(p-4) = jmprel(p, target);
  p[-5] = XI_JMP;
  
  for (q = as->mctop-1; q >= p; q--)
    *q = XI_NOP;
  as->mctop = p;
}


static void asm_tail_prep(ASMState *as)
{
  MCode *p = as->mctop;
  
  if (as->realign) {
    int i = ((int)(intptr_t)as->realign) & 15;
    
    while (i-- > 0)
      *--p = XI_NOP;
    as->mctop = p;
    p -= (as->loopinv ? 5 : 2);  
  } else {
    p -= 5;  
  }
  if (as->loopref) {
    as->invmcp = as->mcp = p;
  } else {
    
    as->mcp = p - (((as->flags & JIT_F_LEA_AGU) ? 7 : 6)  + (LJ_64 ? 1 : 0));
    as->invmcp = NULL;
  }
}




static void asm_ir(ASMState *as, IRIns *ir)
{
  switch ((IROp)ir->o) {
  
  case IR_LOOP: asm_loop(as); break;
  case IR_NOP: case IR_XBAR: lua_assert(!ra_used(ir)); break;
  case IR_USE:
    ra_alloc1(as, ir->op1, irt_isfp(ir->t) ? RSET_FPR : RSET_GPR); break;
  case IR_PHI: asm_phi(as, ir); break;
  case IR_HIOP: asm_hiop(as, ir); break;
  case IR_GCSTEP: asm_gcstep(as, ir); break;

  
  case IR_LT: case IR_GE: case IR_LE: case IR_GT:
  case IR_ULT: case IR_UGE: case IR_ULE: case IR_UGT:
  case IR_EQ: case IR_NE: case IR_ABC:
    asm_comp(as, ir, asm_compmap[ir->o]);
    break;

  case IR_RETF: asm_retf(as, ir); break;

  
  case IR_BNOT: asm_neg_not(as, ir, XOg_NOT); break;
  case IR_BSWAP: asm_bitswap(as, ir); break;

  case IR_BAND: asm_intarith(as, ir, XOg_AND); break;
  case IR_BOR:  asm_intarith(as, ir, XOg_OR); break;
  case IR_BXOR: asm_intarith(as, ir, XOg_XOR); break;

  case IR_BSHL: asm_bitshift(as, ir, XOg_SHL); break;
  case IR_BSHR: asm_bitshift(as, ir, XOg_SHR); break;
  case IR_BSAR: asm_bitshift(as, ir, XOg_SAR); break;
  case IR_BROL: asm_bitshift(as, ir, XOg_ROL); break;
  case IR_BROR: asm_bitshift(as, ir, XOg_ROR); break;

  
  case IR_ADD: asm_add(as, ir); break;
  case IR_SUB:
    if (irt_isnum(ir->t))
      asm_fparith(as, ir, XO_SUBSD);
    else  
      asm_intarith(as, ir, XOg_SUB);
    break;
  case IR_MUL:
    if (irt_isnum(ir->t))
      asm_fparith(as, ir, XO_MULSD);
    else
      asm_intarith(as, ir, XOg_X_IMUL);
    break;
  case IR_DIV:
#if LJ_64 && LJ_HASFFI
    if (!irt_isnum(ir->t))
      asm_arith64(as, ir, irt_isi64(ir->t) ? IRCALL_lj_carith_divi64 :
					     IRCALL_lj_carith_divu64);
    else
#endif
      asm_fparith(as, ir, XO_DIVSD);
    break;
  case IR_MOD:
#if LJ_64 && LJ_HASFFI
    if (!irt_isint(ir->t))
      asm_arith64(as, ir, irt_isi64(ir->t) ? IRCALL_lj_carith_modi64 :
					     IRCALL_lj_carith_modu64);
    else
#endif
      asm_intmod(as, ir);
    break;

  case IR_NEG:
    if (irt_isnum(ir->t))
      asm_fparith(as, ir, XO_XORPS);
    else
      asm_neg_not(as, ir, XOg_NEG);
    break;
  case IR_ABS: asm_fparith(as, ir, XO_ANDPS); break;

  case IR_MIN:
    if (irt_isnum(ir->t))
      asm_fparith(as, ir, XO_MINSD);
    else
      asm_min_max(as, ir, CC_G);
    break;
  case IR_MAX:
    if (irt_isnum(ir->t))
      asm_fparith(as, ir, XO_MAXSD);
    else
      asm_min_max(as, ir, CC_L);
    break;

  case IR_FPMATH: case IR_ATAN2: case IR_LDEXP:
    asm_fpmath(as, ir);
    break;
  case IR_POW:
#if LJ_64 && LJ_HASFFI
    if (!irt_isnum(ir->t))
      asm_arith64(as, ir, irt_isi64(ir->t) ? IRCALL_lj_carith_powi64 :
					     IRCALL_lj_carith_powu64);
    else
#endif
      asm_fppowi(as, ir);
    break;

  
  case IR_ADDOV: asm_intarith(as, ir, XOg_ADD); break;
  case IR_SUBOV: asm_intarith(as, ir, XOg_SUB); break;
  case IR_MULOV: asm_intarith(as, ir, XOg_X_IMUL); break;

  
  case IR_AREF: asm_aref(as, ir); break;
  case IR_HREF: asm_href(as, ir); break;
  case IR_HREFK: asm_hrefk(as, ir); break;
  case IR_NEWREF: asm_newref(as, ir); break;
  case IR_UREFO: case IR_UREFC: asm_uref(as, ir); break;
  case IR_FREF: asm_fref(as, ir); break;
  case IR_STRREF: asm_strref(as, ir); break;

  
  case IR_ALOAD: case IR_HLOAD: case IR_ULOAD: case IR_VLOAD:
    asm_ahuvload(as, ir);
    break;
  case IR_FLOAD: case IR_XLOAD: asm_fxload(as, ir); break;
  case IR_SLOAD: asm_sload(as, ir); break;

  case IR_ASTORE: case IR_HSTORE: case IR_USTORE: asm_ahustore(as, ir); break;
  case IR_FSTORE: case IR_XSTORE: asm_fxstore(as, ir); break;

  
  case IR_SNEW: case IR_XSNEW: asm_snew(as, ir); break;
  case IR_TNEW: asm_tnew(as, ir); break;
  case IR_TDUP: asm_tdup(as, ir); break;
  case IR_CNEW: case IR_CNEWI: asm_cnew(as, ir); break;

  
  case IR_TBAR: asm_tbar(as, ir); break;
  case IR_OBAR: asm_obar(as, ir); break;

  
  case IR_TOBIT: asm_tobit(as, ir); break;
  case IR_CONV: asm_conv(as, ir); break;
  case IR_TOSTR: asm_tostr(as, ir); break;
  case IR_STRTO: asm_strto(as, ir); break;

  
  case IR_CALLN: case IR_CALLL: case IR_CALLS: asm_call(as, ir); break;
  case IR_CALLXS: asm_callx(as, ir); break;
  case IR_CARG: break;

  default:
    setintV(&as->J->errinfo, ir->o);
    lj_trace_err_info(as->J, LJ_TRERR_NYIIR);
    break;
  }
}




static Reg asm_setup_call_slots(ASMState *as, IRIns *ir, const CCallInfo *ci)
{
  IRRef args[CCI_NARGS_MAX*2];
  int nslots;
  asm_collectargs(as, ir, ci, args);
  nslots = asm_count_call_slots(as, ci, args);
  if (nslots > as->evenspill)  
    as->evenspill = nslots;
#if LJ_64
  return irt_isfp(ir->t) ? REGSP_HINT(RID_FPRET) : REGSP_HINT(RID_RET);
#else
  return irt_isfp(ir->t) ? REGSP_INIT : REGSP_HINT(RID_RET);
#endif
}


static void asm_setup_target(ASMState *as)
{
  asm_exitstub_setup(as, as->T->nsnap);
}



static const uint8_t map_op1[256] = {
0x92,0x92,0x92,0x92,0x52,0x45,0x51,0x51,0x92,0x92,0x92,0x92,0x52,0x45,0x51,0x20,
0x92,0x92,0x92,0x92,0x52,0x45,0x51,0x51,0x92,0x92,0x92,0x92,0x52,0x45,0x51,0x51,
0x92,0x92,0x92,0x92,0x52,0x45,0x10,0x51,0x92,0x92,0x92,0x92,0x52,0x45,0x10,0x51,
0x92,0x92,0x92,0x92,0x52,0x45,0x10,0x51,0x92,0x92,0x92,0x92,0x52,0x45,0x10,0x51,
#if LJ_64
0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,
#else
0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,
#endif
0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,
0x51,0x51,0x92,0x92,0x10,0x10,0x12,0x11,0x45,0x86,0x52,0x93,0x51,0x51,0x51,0x51,
0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,
0x93,0x86,0x93,0x93,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,
0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x47,0x51,0x51,0x51,0x51,0x51,
#if LJ_64
0x59,0x59,0x59,0x59,0x51,0x51,0x51,0x51,0x52,0x45,0x51,0x51,0x51,0x51,0x51,0x51,
#else
0x55,0x55,0x55,0x55,0x51,0x51,0x51,0x51,0x52,0x45,0x51,0x51,0x51,0x51,0x51,0x51,
#endif
0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,
0x93,0x93,0x53,0x51,0x70,0x71,0x93,0x86,0x54,0x51,0x53,0x51,0x51,0x52,0x51,0x51,
0x92,0x92,0x92,0x92,0x52,0x52,0x51,0x51,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,
0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x45,0x45,0x47,0x52,0x51,0x51,0x51,0x51,
0x10,0x51,0x10,0x10,0x51,0x51,0x63,0x66,0x51,0x51,0x51,0x51,0x51,0x51,0x92,0x92
};

static const uint8_t map_op2[256] = {
0x93,0x93,0x93,0x93,0x52,0x52,0x52,0x52,0x52,0x52,0x51,0x52,0x51,0x93,0x52,0x94,
0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
0x53,0x53,0x53,0x53,0x53,0x53,0x53,0x53,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x34,0x51,0x35,0x51,0x51,0x51,0x51,0x51,
0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
0x53,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
0x94,0x54,0x54,0x54,0x93,0x93,0x93,0x52,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,
0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
0x52,0x52,0x52,0x93,0x94,0x93,0x51,0x51,0x52,0x52,0x52,0x93,0x94,0x93,0x93,0x93,
0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x94,0x93,0x93,0x93,0x93,0x93,
0x93,0x93,0x94,0x93,0x94,0x94,0x94,0x93,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,
0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,
0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x93,0x52
};

static uint32_t asm_x86_inslen(const uint8_t* p)
{
  uint32_t result = 0;
  uint32_t prefixes = 0;
  uint32_t x = map_op1[*p];
  for (;;) {
    switch (x >> 4) {
    case 0: return result + x + (prefixes & 4);
    case 1: prefixes |= x; x = map_op1[*++p]; result++; break;
    case 2: x = map_op2[*++p]; break;
    case 3: p++; goto mrm;
    case 4: result -= (prefixes & 2);  
    case 5: return result + (x & 15);
    case 6:  
      if (p[1] & 0x38) x = 2;
      else if ((prefixes & 2) && (x == 0x66)) x = 4;
      goto mrm;
    case 7: 
      if (LJ_32 && p[1] < 0xc0) {
	x = 2;
	goto mrm;
      }
      if (x == 0x70) {
	x = *++p & 0x1f;
	result++;
	if (x >= 2) {
	  p += 2;
	  result += 2;
	  goto mrm;
	}
      }
      p++;
      result++;
      x = map_op2[*++p];
      break;
    case 8: result -= (prefixes & 2);  
    case 9: mrm:  
      result += (x & 15);
      x = *++p;
      switch (x >> 6) {
      case 0: if ((x & 7) == 5) return result + 4; break;
      case 1: result++; break;
      case 2: result += 4; break;
      case 3: return result;
      }
      if ((x & 7) == 4) {
	result++;
	if (x < 0x40 && (p[1] & 7) == 5) result += 4;
      }
      return result;
    }
  }
}


void lj_asm_patchexit(jit_State *J, GCtrace *T, ExitNo exitno, MCode *target)
{
  MCode *p = T->mcode;
  MCode *mcarea = lj_mcode_patch(J, p, 0);
  MSize len = T->szmcode;
  MCode *px = exitstub_addr(J, exitno) - 6;
  MCode *pe = p+len-6;
  uint32_t stateaddr = u32ptr(&J2G(J)->vmstate);
  if (len > 5 && p[len-5] == XI_JMP && p+len-6 + *(int32_t *)(p+len-4) == px)
    *(int32_t *)(p+len-4) = jmprel(p+len, target);
  
  for (; p < pe; p += asm_x86_inslen(p))
    if (*(uint32_t *)(p+(LJ_64 ? 3 : 2)) == stateaddr && p[0] == XI_MOVmi)
      break;
  lua_assert(p < pe);
  for (; p < pe; p += asm_x86_inslen(p))
    if ((*(uint16_t *)p & 0xf0ff) == 0x800f && p + *(int32_t *)(p+2) == px)
      *(int32_t *)(p+2) = jmprel(p+6, target);
  lj_mcode_sync(T->mcode, T->mcode + T->szmcode);
  lj_mcode_patch(J, mcarea, 1);
}

