/*
 * sraiw.c
 *
 * RISC-V ISA: RV64I
 *
 * Copyright (C) 2017-2023 Tactical Computing Laboratories, LLC
 * All Rights Reserved
 * contact@tactcomplabs.com
 *
 * See LICENSE in the top level directory for licensing details
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "isa_test_macros.h"

int main(int argc, char **argv){

 // #-------------------------------------------------------------
 // # Arithmetic tests
 // #-------------------------------------------------------------

  TEST_IMM_OP( 2,  slliw, 0x0000000000000001, 0x0000000000000001, 0  );
  TEST_IMM_OP( 3,  slliw, 0x0000000000000002, 0x0000000000000001, 1  );
  TEST_IMM_OP( 4,  slliw, 0x0000000000000080, 0x0000000000000001, 7  );
  TEST_IMM_OP( 5,  slliw, 0x0000000000004000, 0x0000000000000001, 14 );
  TEST_IMM_OP( 6,  slliw, 0xffffffff80000000, 0x0000000000000001, 31 );

  TEST_IMM_OP( 7,  slliw, 0xffffffffffffffff, 0xffffffffffffffff, 0  );
  TEST_IMM_OP( 8,  slliw, 0xfffffffffffffffe, 0xffffffffffffffff, 1  );
  TEST_IMM_OP( 9,  slliw, 0xffffffffffffff80, 0xffffffffffffffff, 7  );
  TEST_IMM_OP( 10, slliw, 0xffffffffffffc000, 0xffffffffffffffff, 14 );
  TEST_IMM_OP( 11, slliw, 0xffffffff80000000, 0xffffffffffffffff, 31 );

  TEST_IMM_OP( 12, slliw, 0x0000000021212121, 0x0000000021212121, 0  );
  TEST_IMM_OP( 13, slliw, 0x0000000042424242, 0x0000000021212121, 1  );
  TEST_IMM_OP( 14, slliw, 0xffffffff90909080, 0x0000000021212121, 7  );
  TEST_IMM_OP( 15, slliw, 0x0000000048484000, 0x0000000021212121, 14 );
  TEST_IMM_OP( 16, slliw, 0xffffffff80000000, 0x0000000021212121, 31 );

  // # Verify that shifts ignore top 32 (using true 64-bit values)

  TEST_IMM_OP( 44, slliw, 0x0000000012345678, 0xffffffff12345678, 0 );
  TEST_IMM_OP( 45, slliw, 0x0000000023456780, 0xffffffff12345678, 4 );
  TEST_IMM_OP( 46, slliw, 0xffffffff92345678, 0x0000000092345678, 0 );
  TEST_IMM_OP( 47, slliw, 0xffffffff93456780, 0x0000000099345678, 4 );
  //-------------------------------------------------------------
  // Source/Destination tests
  //-------------------------------------------------------------

  //TEST_IMM_SRC1_EQ_DEST( 17, slliw, 0x00000080, 0x00000001, 7 );

asm volatile(" bne x0, gp, pass;");
asm volatile("pass:" ); 
     asm volatile("j continue");

asm volatile("fail:" ); 
      assert(false);

asm volatile("continue:");
asm volatile("li ra, 0x0");

  return 0;
}
