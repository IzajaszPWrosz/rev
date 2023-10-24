//
// _RevExt_cc_
//
// Copyright (C) 2017-2023 Tactical Computing Laboratories, LLC
// All Rights Reserved
// contact@tactcomplabs.com
//
// See LICENSE in the top level directory for licensing details
//

#include "../include/RevExt.h"
using namespace SST::RevCPU;

/// Sets the Floating-Point Rounding Mode on the host
void RevExt::SetRoundingMode(FRMode rm, const RevRegFile* regFile){
  // If the encoded rounding mode is dynamic, load the frm register
  if(rm == FRMode::DYN){
    rm = regFile->GetFRM();
  }
  switch(rm){
    case FRMode::None:
      break;
    case FRMode::RNE:   // Round to Nearest, ties to Even
      RevFenv::SetRoundingMode(FE_TONEAREST);
      break;
    case FRMode::RTZ:   // Round towards Zero
      RevFenv::SetRoundingMode(FE_TOWARDZERO);
      break;
    case FRMode::RDN:   // Round Down (towards -Inf)
      RevFenv::SetRoundingMode(FE_DOWNWARD);
      break;
    case FRMode::RUP:   // Round Up (towards +Inf)
      RevFenv::SetRoundingMode(FE_UPWARD);
      break;
    case FRMode::RMM:   // Round to Nearest, ties to Max Magnitude
      output->fatal(CALL_INFO, -1,
                    "Error: Round to nearest Max Magnitude not implemented at PC = 0x%" PRIx64 "\n",
                    regFile->GetPC());
      break;
    default:
      output->fatal(CALL_INFO, -1, "Illegal instruction: Bad FP rounding mode at PC = 0x%" PRIx64 "\n",
                    regFile->GetPC());
      break;
  }
}

/// Execute an instruction
bool RevExt::Execute(unsigned Inst, const RevInst& payload, uint16_t HartID, RevRegFile* regFile){
  bool (*func)(RevFeature *,
               RevRegFile *,
               RevMem *,
               RevInst);

  if( payload.compressed ){
    // this is a compressed instruction, grab the compressed trampoline function
    func = Inst < ctable.size() ? ctable[Inst].func : nullptr;
  }else{
    // retrieve the function pointer for this instruction
    func = Inst < table.size() ? table[Inst].func : nullptr;
  }

  if( !func ){
    output->fatal(CALL_INFO, -1,
                  "Error: instruction at index=%u does not exist in extension=%s",
                  Inst,
                  name.data());
    return false;
  }

  // execute the instruction
  bool ret = false;

  // If instruction cannot raise FP exceptions, don't mess with FP environment
  if(!payload.raisefpe){
    ret = func(feature, regFile, mem, payload);
  }else{
    // saved_fenv represents a saved FP environment, which, when destroyed,
    // restores the original FP environment. We execute the instruction in a
    // default FP environment on the host with all FP exceptions cleared and
    // the rounding mode set according to the encoding and frm register. The FP
    // exceptions which occur are stored in FCSR when saved_fenv is destroyed.

    RevFenv saved_fenv{regFile->GetFCSR()};

    // Set the FP rounding mode, if it exists.
    if(payload.rm != FRMode::None){
      SetRoundingMode(payload.rm, regFile);
    }

    // Execute the instruction
    ret = func(feature, regFile, mem, payload);

    // Fall-through destroys saved_fenv, setting the FCSR's fflags and
    // restoring the host's FP environment
  }

  return ret;
}

// EOF
