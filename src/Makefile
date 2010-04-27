# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Simple Makefile for local compilation on the platform and cross compilation
# using emerge.

USE_TPM_EMULATOR ?= 1

default:
	@echo "Usage: make { firmware | local | cross } [ USE_TPM_EMULATOR=0 ]"
	@exit 1

firmware:
	(cd tlcl; $(MAKE) -f Makefile.firmware)

local:
	(cd tlcl; $(MAKE) USE_TPM_EMULATOR=$(USE_TPM_EMULATOR))
	(cd testsuite; $(MAKE) USE_TPM_EMULATOR=$(USE_TPM_EMULATOR) \
	 LOCAL_COMPILATION=1)

cross:
	(cd tlcl; $(MAKE) USE_TPM_EMULATOR=$(USE_TPM_EMULATOR))
	(cd testsuite; $(MAKE) USE_TPM_EMULATOR=$(USE_TPM_EMULATOR))

clean:
	(cd tlcl; $(MAKE) clean)
	(cd testsuite; $(MAKE) clean)
