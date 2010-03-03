# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LIBTLCL = ../tlcl/libtlcl.a
LIBTPM = ../../../../third_party/tpm-emulator/build/tpm/libtpm.a
LIBCRYPTO = ../../../../third_party/tpm-emulator/build/crypto/libcrypto.a
LIBTPMEMU = ../../../../third_party/tpm-emulator/build/tpmd/unix/libtpmemu.a
LIBS = $(LIBTLCL) $(LIBTPMEMU) $(LIBTPM) $(LIBCRYPTO)

CC ?= cc

readonly: readonly.o $(LIBS)
	$(CC) -g readonly.o -o readonly $(LIBS) -lgmp

readonly.o: readonly.c
	cc -g -c -Werror -Wall -pedantic -ansi readonly.c -I../include

clean:
	rm -f readonly *.o *~
