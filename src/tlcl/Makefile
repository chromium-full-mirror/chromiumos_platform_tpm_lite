# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

INCLUDEDIRS = -I../../../../third_party/tpm-emulator/tpmd/unix
LOCAL_CFLAGS = -g -Wall -Werror $(INCLUDEDIRS)
CFLAGS += $(LOCAL_CFLAGS)
# CFLAGS += -ansi -pedantic
ifeq ($(USE_TPM_EMULATOR),1)
CFLAGS += -DUSE_TPM_EMULATOR=1
else
CFLAGS += -DUSE_TPM_EMULATOR=0
endif

CC ?= cc
HOSTCC = cc

libtlcl.a: tlcl.o
	ar rcs libtlcl.a tlcl.o

tlcl.o: tlcl.c tlcl_internal.h tlcl.h structures.h

structures.h: generator
	./generator > structures.h
	sed -e '0,/This file is automatically/d' < structures.h > x.tmp
	sed -e '0,/This file is automatically/d' < saved-structures.h > y.tmp
	cmp -s x.tmp y.tmp || \
		(echo "Please update saved-structures.h" && false)

generator: generator.c tlcl.h
	$(HOSTCC) $(LOCAL_CFLAGS) -I$(ROOT)/usr/include \
	-fpack-struct generator.c -o generator

clean:
	rm -f generator *.o *.a structures.h x.tmp y.tmp *~
