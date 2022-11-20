#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "mips1arch.h"
#include "loader.h"
#include "memory.h"
#include "regs.h"
#include "pc.h"
#include "pipelined.h"
#include "hazard.h"
#include "emu.h"
#include "stats.h"
#include "opts.h"
#include "util.h"
#include "bypass.h"

// syscall = 0
// rType = 1
// iType = 2
// jType = 3

instruction_t *bypass(instruction_t *inst, instruction_t *exmem_bypass, instruction_t *memwb_bypass)
{

  // if we have a bubble, return inst
  if (bubbleCase(inst) == 1)
    return inst;

  // if inst is a syscall, return inst
  if (instType(inst) == 0)
    return inst;

  // are we looking at two R type instructions with respect to memwb_bypass
  if ((instType(memwb_bypass) == 1) && (instType(inst) == 1))
  {
    // for handeling the special R cases where there is no rs register
    if ((isRegWrite(memwb_bypass) == 1) && (rCase(inst) == 1))
    {
      if (memwb_bypass->rd == inst->rt)
        inst->rt_value = memwb_bypass->rd_value;
    }
    // should we update both of inst rs and rt values
    else if ((isRegWrite(memwb_bypass) == 1) && (memwb_bypass->rd == inst->rt) && (memwb_bypass->rd == inst->rs))
      inst->rt_value = inst->rs_value = memwb_bypass->rd_value;
    // or only update the rt value
    else if ((isRegWrite(memwb_bypass) == 1) && (memwb_bypass->rd == inst->rt))
      inst->rt_value = memwb_bypass->rd_value;
    // or only update the rs value
    else if ((isRegWrite(memwb_bypass) == 1) && (memwb_bypass->rd == inst->rs))
      inst->rs_value = memwb_bypass->rs_value;
  }

  // are we looking at an R to I type instruction with respect to memwb_bypass
  if ((instType(memwb_bypass) == 1) && (instType(inst) == 2))
  {
    // update inst rs value if it is a dependency
    if ((isRegWrite(memwb_bypass) == 1) && (memwb_bypass->rd == inst->rs))
      inst->rs_value = memwb_bypass->rd_value;
  }

  // are we looking at an I to R type instruction
  if ((instType(memwb_bypass) == 2) && (instType(inst) == 1))
  { // for handeling the special R cases where there is no rs register
    if ((isRegWrite(memwb_bypass) == 1) && (rCase(inst) == 1))
    {
      // if there is a dependency, update the rt value
      if (memwb_bypass->rt == inst->rt)
        inst->rt_value = memwb_bypass->rt_value;
    }
    // should we update both of inst rs and rt values
    else if ((isRegWrite(memwb_bypass) == 1) && (memwb_bypass->rt == inst->rs) && (memwb_bypass->rt == inst->rt))
      inst->rs_value = inst->rt_value = memwb_bypass->rt_value;
    // or only update the rs value
    else if ((isRegWrite(memwb_bypass) == 1) && (memwb_bypass->rt == inst->rs))
      inst->rs_value = memwb_bypass->rt_value;
    // or only update the rt value
    else if ((isRegWrite(memwb_bypass) == 1) && (memwb_bypass->rt == inst->rt))
      inst->rt_value = memwb_bypass->rt_value;
  }

  // are we looking at two R type instructions with respect to exmem_bypass
  if ((instType(exmem_bypass) == 1) && (instType(inst) == 1))
  {
    if ((isRegWrite(exmem_bypass) == 1) && (rCase(inst) == 1))
    {
      if (exmem_bypass->rd == inst->rt)
        inst->rt_value = exmem_bypass->rd_value;
    }
    // should we update both of inst rs and rt values
    else if ((isRegWrite(exmem_bypass) == 1) && (exmem_bypass->rd == inst->rt) && (exmem_bypass->rd == inst->rs))
      inst->rt_value = inst->rs_value = exmem_bypass->rd_value;
    // or only update the rt value
    else if ((isRegWrite(exmem_bypass) == 1) && (exmem_bypass->rd == inst->rt))
      inst->rt_value = exmem_bypass->rd_value;
    // or only update the rs value
    else if ((isRegWrite(exmem_bypass) == 1) && (exmem_bypass->rd == inst->rs))
      inst->rs_value = exmem_bypass->rs_value;
  }

  // are we looking at an R to I type instruction with respect to exmem_bypass
  if ((instType(exmem_bypass) == 1) && (instType(inst) == 2))
  {
    // update inst rs value if it is a dependency
    if ((isRegWrite(exmem_bypass) == 1) && (exmem_bypass->rd == inst->rs))
      inst->rs_value = exmem_bypass->rd_value;
  }

  // are we looking at an I to R type instruction
  if ((instType(exmem_bypass) == 2) && (instType(inst) == 1))
  {
    // for handeling the special R cases where there is no rs register
    if ((isRegWrite(exmem_bypass) == 1) && (rCase(inst) == 1))
    {
      // if there is a dependency, update the rt value
      if (exmem_bypass->rt == inst->rt)
        inst->rt_value = exmem_bypass->rt_value;
    }
    // should we update both of inst rs and rt values
    else if ((isRegWrite(exmem_bypass) == 1) && (exmem_bypass->rt == inst->rs) && (exmem_bypass->rt == inst->rt))
      inst->rs_value = inst->rt_value = exmem_bypass->rt_value;
    // or only update the rs value
    else if ((isRegWrite(exmem_bypass) == 1) && (exmem_bypass->rt == inst->rs))
      inst->rs_value = exmem_bypass->rt_value;
    // or onluy update the rt value
    else if ((isRegWrite(exmem_bypass) == 1) && (exmem_bypass->rt == inst->rt))
      inst->rt_value = exmem_bypass->rt_value;
  }

  // finally, return inst
  return inst;
}

// return the instruction type, whether that is R,I or J type
int instType(instruction_t *inst)
{

  // where 1 is R type, 2 is I type, 3 is J type
  if (inst->mnemonic == inst_add)
    return 1;
  else if (inst->mnemonic == inst_addi)
    return 2;
  else if (inst->mnemonic == inst_addiu)
    return 2;
  else if (inst->mnemonic == inst_addu)
    return 1;
  else if (inst->mnemonic == inst_and)
    return 1;
  else if (inst->mnemonic == inst_andi)
    return 2;
  else if (inst->mnemonic == inst_beq)
    return 2;
  else if (inst->mnemonic == inst_bne)
    return 2;
  else if (inst->mnemonic == inst_j)
    return 3;
  else if (inst->mnemonic == inst_jal)
    return 3;
  else if (inst->mnemonic == inst_jr)
    return 1;
  else if (inst->mnemonic == inst_lbu)
    return 2;
  else if (inst->mnemonic == inst_lhu)
    return 2;
  else if (inst->mnemonic == inst_lui)
    return 2;
  else if (inst->mnemonic == inst_lw)
    return 2;
  else if (inst->mnemonic == inst_nor)
    return 1;
  else if (inst->mnemonic == inst_or)
    return 1;
  else if (inst->mnemonic == inst_ori)
    return 2;
  else if (inst->mnemonic == inst_slt)
    return 1;
  else if (inst->mnemonic == inst_slti)
    return 2;
  else if (inst->mnemonic == inst_sltiu)
    return 2;
  else if (inst->mnemonic == inst_sltu)
    return 1;
  else if (inst->mnemonic == inst_sll)
    return 1;
  else if (inst->mnemonic == inst_srl)
    return 1;
  else if (inst->mnemonic == inst_sb)
    return 2;
  else if (inst->mnemonic == inst_sh)
    return 2;
  else if (inst->mnemonic == inst_sw)
    return 2;
  else if (inst->mnemonic == inst_sub)
    return 1;
  else if (inst->mnemonic == inst_subu)
    return 1;
  else if (inst->mnemonic == inst_sra)
    return 1;
  else if (inst->mnemonic == inst_xor)
    return 1;
  else
    return 0; // otherwise it will be a syscall
}

// check to see if inst is an sll, srl, or sra instruction
int rCase(instruction_t *inst)
{

  if (inst->mnemonic == inst_sll || inst->mnemonic == inst_srl || inst->mnemonic == inst_sra)
    return 1;
  return 0;
}

// check to see if inst is null
int bubbleCase(instruction_t *inst)
{

  if (inst == NULL)
    return 1;
  return 0;
}
