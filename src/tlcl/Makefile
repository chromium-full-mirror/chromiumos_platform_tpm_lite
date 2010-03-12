# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

INCLUDEDIRS = -I../include -I../../../../third_party/tpm-emulator/tpmd/unix
CFLAGS += -g -Wall -Werror $(INCLUDEDIRS)
# CFLAGS += -ansi -pedantic
ifeq ($(USE_TPM_EMULATOR),1)
CFLAGS += -DUSE_TPM_EMULATOR=1
else
CFLAGS += -DUSE_TPM_EMULATOR=0
endif

CC ?= cc

libtlcl.a: tlcl.o 
	ar rcs libtlcl.a tlcl.o

tlcl.o: tlcl.c tlcl_internal.h ../include/tlcl.h structures.h

structures.h: generator
	./generator > structures.h

generator: generator.c ../include/tlcl.h
	$(CC) $(CFLAGS) -fpack-struct generator.c -o generator

clean:
	rm -f generator *.o *.a structures.h *~
