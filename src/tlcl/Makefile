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

tlcl.o: tlcl.c tlcl_internal.h tlcl.h \
	update-structures structures.h update-version version.h

update-structures: generator
	./generator > structures.tmp
	cmp -s structures.tmp structures.h || \
		( echo "%% Updating structures.h %%" && \
		  cp structures.tmp structures.h )

update-version:
	find \( -name '*.[ch]' -o -name 'Makefile' \) -a \! -name version.h \
		| sort | xargs cat | md5sum | cut -c 25-32 > x.tmp
	echo "char* TlclVersion = \"TLCLv=$$(cat x.tmp)\";" > version.tmp
	cmp -s version.tmp version.h || \
		( echo "** Updating version.h **" && cp version.tmp version.h )

generator: generator.c tlcl.h
	$(HOSTCC) $(LOCAL_CFLAGS) -I$(ROOT)/usr/include \
	-fpack-struct generator.c -o generator

clean:
	rm -f generator *.o *.a version.tmp structures.tmp x.tmp *~
