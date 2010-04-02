# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LIBTLCL = ../tlcl/libtlcl.a

ifeq ($(USE_TPM_EMULATOR),1)
ifeq ($(LOCAL_COMPILATION),1)
LIBTPM = ../../../../third_party/tpm-emulator/build/tpm/libtpm.a
LIBCRYPTO = ../../../../third_party/tpm-emulator/build/crypto/libcrypto.a
LIBTPMEMU = ../../../../third_party/tpm-emulator/build/tpmd/unix/libtpmemu.a
else
LIBTPM = ${ROOT}/usr/lib/tpmemu/libtpm.a
LIBCRYPTO = ${ROOT}/usr/lib/tpmemu/libcrypto.a
LIBTPMEMU = ${ROOT}/usr/lib/tpmemu/libtpmemu.a
endif
LIBGMP = -lgmp
endif

LIBS = $(LIBTLCL) $(LIBTPMEMU) $(LIBTPM) $(LIBCRYPTO) $(LIBGMP)

CC ?= cc
CFLAGS += -Werror -Wall
#CFLAGS += -pedantic -ansi

TESTS = tpmtest_clear \
	tpmtest_enable \
	tpmtest_globallock \
        tpmtest_lock \
        tpmtest_readonly \
        tpmtest_writelimit \

all: $(TESTS)

$(TESTS): tpmtest_%: %.o $(LIBS)
	$(CC) $(LDFLAGS) -g $< -o $@ $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -g -c $< -I../include

clean:
	rm -f $(TESTS) *.o *~
