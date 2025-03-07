/*
 * addw.c
 *
 * RISC-V ISA: RV32I
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

//  #-------------------------------------------------------------
//  # Arithmetic tests
//  #-------------------------------------------------------------

TEST_IMM_OP( 2,  slli, 0x0000000000000001, 0x0000000000000001, 0  );
  TEST_IMM_OP( 3,  slli, 0x0000000000000002, 0x0000000000000001, 1  );
  TEST_IMM_OP( 4,  slli, 0x0000000000000080, 0x0000000000000001, 7  );
  TEST_IMM_OP( 5,  slli, 0x0000000000004000, 0x0000000000000001, 14 );
  TEST_IMM_OP( 6,  slli, 0x0000000080000000, 0x0000000000000001, 31 );

  TEST_IMM_OP( 7,  slli, 0xffffffffffffffff, 0xffffffffffffffff, 0  );
  TEST_IMM_OP( 8,  slli, 0xfffffffffffffffe, 0xffffffffffffffff, 1  );
  TEST_IMM_OP( 9,  slli, 0xffffffffffffff80, 0xffffffffffffffff, 7  );
  TEST_IMM_OP( 10, slli, 0xffffffffffffc000, 0xffffffffffffffff, 14 );
  TEST_IMM_OP( 11, slli, 0xffffffff80000000, 0xffffffffffffffff, 31 );

  TEST_IMM_OP( 12, slli, 0x0000000021212121, 0x0000000021212121, 0  );
  TEST_IMM_OP( 13, slli, 0x0000000042424242, 0x0000000021212121, 1  );
  TEST_IMM_OP( 14, slli, 0x0000001090909080, 0x0000000021212121, 7  );
  TEST_IMM_OP( 15, slli, 0x0000084848484000, 0x0000000021212121, 14 );
  TEST_IMM_OP( 16, slli, 0x1090909080000000, 0x0000000021212121, 31 );

#if __riscv_xlen == 64
  TEST_IMM_OP( 50, slli, 0x8000000000000000, 0x0000000000000001, 63 );
  TEST_IMM_OP( 51, slli, 0xffffff8000000000, 0xffffffffffffffff, 39 );
  TEST_IMM_OP( 52, slli, 0x0909080000000000, 0x0000000021212121, 43 );
#endif

 // #-------------------------------------------------------------
 // # Source/Destination tests
 // #-------------------------------------------------------------

//TEST_IMM_SRC1_EQ_DEST( 17, slli, 0x00000080, 0x00000001, 7 );

asm volatile(" bne x0, gp, pass;");
asm volatile("pass:" ); 
     asm volatile("j continue");

asm volatile("fail:" ); 
      assert(false);

asm volatile("continue:");
asm volatile("li ra, 0x0");

  return 0;
}
