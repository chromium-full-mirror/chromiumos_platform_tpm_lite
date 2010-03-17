/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Testing: ownership testing code, ForceClear, and nvram write limit.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <tss/tcs.h>

#include "tlcl.h"

/* These index values are used to create NVRAM spaces.  They only need to be
 * unique.
 */
#define INDEX0 0xda70
#define INDEX1 0xda71
#define INDEX2 0xda72
#define INDEX3 0xda73

int main(void) {
  int owned;

  TlclLibinit();

#if !USE_TPM_EMULATOR
  /* The emulator does these itself. */
  TlclStartup();
  TlclSelftestfull();
#endif

  TlclAssertPhysicalPresence();

  owned = TlclIsOwned();
  printf("tpm is %sowned\n", owned?  "" : "NOT ");
  if (owned) {
    TlclForceClear();
    printf("tpm was cleared\n");
  }

  return 0;
}
