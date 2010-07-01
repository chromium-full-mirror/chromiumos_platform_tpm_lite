/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* A lightweight TPM command library.
 *
 * The general idea is that TPM commands are array of bytes whose fields are
 * mostly compile-time constant.  The goal is to build much of the commands at
 * compile time (or build time) and change some of the fields at run time as
 * needed.  The code in generator.c builds structures containing the commands,
 * as well as the offsets of the fields that need to be set at run time.
 */

#include "tlcl.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef FIRMWARE
#include "tss_constants.h"
#else
#include <tss/tcs.h>
#include "tpmextras.h"
#endif  /* FIRMWARE */

#include "structures.h"
#include "tlcl_internal.h"
#include "version.h"

#if USE_TPM_EMULATOR
#include "tpmemu.h"
#endif

#define TPM_DEVICE_PATH "/dev/tpm0"

/* The file descriptor for the TPM device.
 */
int tpm_fd = -1;

/* Log level.  0 is quietest.  Be verbose by default.
 */
int tlcl_log_level = 1;

/* Print |n| bytes from array |a|, with newlines.
 */
POSSIBLY_UNUSED static void PrintBytes(uint8_t* a, int n) {
  int i;
  for (i = 0; i < n; i++) {
    TlclLog("%02x ", a[i]);
    if ((i + 1) % 16 == 0) {
      TlclLog("\n");
    }
  }
  if (i % 16 != 0) {
    TlclLog("\n");
  }
}

/* Gets the tag field of a TPM command.
 */
POSSIBLY_UNUSED static INLINE int TpmTag(uint8_t* buffer) {
  uint16_t tag;
  FromTpmUint16(buffer, &tag);
  return (int) tag;
}

/* Sets the size field of a TPM command.
 */
static INLINE void SetTpmCommandSize(uint8_t* buffer, uint32_t size) {
  ToTpmUint32(buffer + sizeof(uint16_t), size);
}

/* Gets the size field of a TPM command.
 */
POSSIBLY_UNUSED static INLINE int TpmCommandSize(const uint8_t* buffer) {
  uint32_t size;
  FromTpmUint32(buffer + sizeof(uint16_t), &size);
  return (int) size;
}

/* Gets the code field of a TPM command.
 */
static INLINE int TpmCommandCode(const uint8_t* buffer) {
  uint32_t code;
  FromTpmUint32(buffer + sizeof(uint16_t) + sizeof(uint32_t), &code);
  return code;
}

/* Gets the return code field of a TPM result.
 */
static INLINE int TpmReturnCode(const uint8_t* buffer) {
  return TpmCommandCode(buffer);
}

/* Checks for errors in a TPM response.
 */
static void CheckResult(uint8_t* request, uint8_t* response, bool warn_only) {
  int command = TpmCommandCode(request);
  int result = TpmReturnCode(response);
  if (result != TPM_SUCCESS) {
    (warn_only? warning : error)("command %d 0x%x failed: %d 0x%x\n",
                                 command, command, result, result);
  }
}

#ifndef FIRMWARE
/* Executes a command on the TPM.
 */
void TpmExecute(const uint8_t *in, const uint32_t in_len,
                uint8_t *out, uint32_t *pout_len) {
  uint8_t response[TPM_MAX_COMMAND_SIZE];
  if (in_len <= 0) {
    error("invalid command length %d\n", in_len);
  } else if (tpm_fd < 0) {
    error("the TPM device was not opened.  Forgot to call TlclLibInit?\n");
  } else {
    int n = write(tpm_fd, in, in_len);
    if (n != in_len) {
      error("write failure to TPM device: %s\n", strerror(errno));
    }
    n = read(tpm_fd, response, sizeof(response));
    if (n == 0) {
      error("null read from TPM device\n");
    } else if (n < 0) {
      error("read failure from TPM device: %s\n", strerror(errno));
    } else {
      if (n > *pout_len) {
        error("TPM response too long for output buffer\n");
      } else {
        *pout_len = n;
        memcpy(out, response, n);
      }
    }
  }
}
#endif  /* !FIRMWARE */

/* Sends a request and receive a response.
 */
static void SendReceive(uint8_t* request, uint8_t* response, int max_length) {

#ifdef FIRMWARE
  /*
   * FIXME: This #if block should contain the equivalent API call for the
   * firmware TPM driver which takes a raw sequence of bytes as input
   * command and a pointer to the output buffer for putting in the results.
   *
   * For EFI firmwares, this can make use of the EFI TPM driver as follows
   * (based on page 16, of TCG EFI Protocol Specs Version 1.20 availaible from
   * the TCG website):
   *
   * EFI_STATUS status;
   * status = TcgProtocol->EFI_TCG_PASS_THROUGH_TO_TPM(TpmCommandSize(request),
   *                                                   request,
   *                                                   max_length,
   *                                                   response);
   * // Error checking depending on the value of the status above
   */
#else  /* FIRMWARE */
  uint32_t response_length = max_length;
  int tag, response_tag;
#if USE_TPM_EMULATOR
  tpmemu_execute(request, TpmCommandSize(request), response, &response_length);
#else
  struct timeval before, after;
  gettimeofday(&before, NULL);
  TpmExecute(request, TpmCommandSize(request), response, &response_length);
  gettimeofday(&after, NULL);
#endif  /* USE_TPM_EMULATOR */
  {
    int x = TpmCommandSize(request);
    int y = response_length;
    TlclLog("request (%d bytes): ", x);
    PrintBytes(request, 10);
    PrintBytes(request + 10, x - 10);
    TlclLog("response (%d bytes): ", y);
    PrintBytes(response, 10);
    PrintBytes(response + 10, y - 10);
#if !USE_TPM_EMULATOR
    TlclLog("execution time: %dms\n",
            (int) ((after.tv_sec - before.tv_sec) * 1000 +
                   (after.tv_usec - before.tv_usec) / 1000));
#endif  /* !USE_TPM_EMULATOR */
  }
  /* sanity checks */
  tag = TpmTag(request);
  response_tag = TpmTag(response);
  assert(
    (tag == TPM_TAG_RQU_COMMAND &&
     response_tag == TPM_TAG_RSP_COMMAND) ||
    (tag == TPM_TAG_RQU_AUTH1_COMMAND &&
     response_tag == TPM_TAG_RSP_AUTH1_COMMAND) ||
    (tag == TPM_TAG_RQU_AUTH2_COMMAND &&
     response_tag == TPM_TAG_RSP_AUTH2_COMMAND));
  assert(response_length == TpmCommandSize(response));
#endif  /* FIRMWARE */

}

/* Sends a command and returns the error code.
 */
static uint32_t Send(uint8_t* command) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  SendReceive(command, response, sizeof(response));
  return TpmReturnCode(response);
}

/* Exported functions.
 */

void TlclLog(char* format, ...) {
  va_list ap;
  if (tlcl_log_level > 0) {
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
  }
}

void TlclSetLogLevel(int level) {
  tlcl_log_level = level;
}

void TlclLibInit(void) {
#if USE_TPM_EMULATOR
  tpmemu_init();
#else
  TlclOpenDevice();
#endif  /* USE_TPM_EMULATOR */
}

void TlclCloseDevice(void) {
#if !FIRMWARE
  close(tpm_fd);
  tpm_fd = -1;
#endif  /* !FIRMWARE */
}

void TlclOpenDevice(void) {
#if !FIRMWARE
  tpm_fd = open(TPM_DEVICE_PATH, O_RDWR);
  if (tpm_fd < 0) {
    error("cannot open TPM device %s: %s\n", TPM_DEVICE_PATH, strerror(errno));
  }
#endif  /* !FIRMWARE */
}

uint32_t TlclStartup(void) {
  return Send(tpm_startup_cmd.buffer);
}

uint32_t TlclSelftestfull(void) {
  return Send(tpm_selftestfull_cmd.buffer);
}

uint32_t TlclContinueSelfTest(void) {
  return Send(tpm_continueselftest_cmd.buffer);
}

uint32_t TlclDefineSpace(uint32_t index, uint32_t perm, uint32_t size) {
  ToTpmUint32(tpm_nv_definespace_cmd.index, index);
  ToTpmUint32(tpm_nv_definespace_cmd.perm, perm);
  ToTpmUint32(tpm_nv_definespace_cmd.size, size);
  return Send(tpm_nv_definespace_cmd.buffer);
}

uint32_t TlclWrite(uint32_t index, uint8_t* data, uint32_t length) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  const int total_length =
    kTpmRequestHeaderLength + kWriteInfoLength + length;

  assert(total_length <= TPM_LARGE_ENOUGH_COMMAND_SIZE);
  SetTpmCommandSize(tpm_nv_write_cmd.buffer, total_length);

  ToTpmUint32(tpm_nv_write_cmd.index, index);
  ToTpmUint32(tpm_nv_write_cmd.length, length);
  memcpy(tpm_nv_write_cmd.data, data, length);

  SendReceive(tpm_nv_write_cmd.buffer, response, sizeof(response));
  CheckResult(tpm_nv_write_cmd.buffer, response, true);

  return TpmReturnCode(response);
}

uint32_t TlclRead(uint32_t index, uint8_t* data, uint32_t length) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  uint32_t result_length;
  uint32_t result;

  ToTpmUint32(tpm_nv_read_cmd.index, index);
  ToTpmUint32(tpm_nv_read_cmd.length, length);

  SendReceive(tpm_nv_read_cmd.buffer, response, sizeof(response));
  result = TpmReturnCode(response);
  if (result == TPM_SUCCESS && length > 0) {
    uint8_t* nv_read_cursor = response + kTpmResponseHeaderLength;
    FromTpmUint32(nv_read_cursor, &result_length);
    nv_read_cursor += sizeof(uint32_t);
    memcpy(data, nv_read_cursor, result_length);
  }

  return result;
}

uint32_t TlclWriteLock(uint32_t index) {
  return TlclWrite(index, NULL, 0);
}

uint32_t TlclReadLock(uint32_t index) {
  return TlclRead(index, NULL, 0);
}

uint32_t TlclAssertPhysicalPresence(void) {
  return Send(tpm_ppassert_cmd.buffer);
}

uint32_t TlclAssertPhysicalPresenceResult(void) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  SendReceive(tpm_ppassert_cmd.buffer, response, sizeof(response));
  return TpmReturnCode(response);
}

uint32_t TlclLockPhysicalPresence(void) {
  return Send(tpm_pplock_cmd.buffer);
}

uint32_t TlclSetNvLocked(void) {
  return TlclDefineSpace(TPM_NV_INDEX_LOCK, 0, 0);
}

int TlclIsOwned(void) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE + TPM_PUBEK_SIZE];
  uint32_t result;
  SendReceive(tpm_readpubek_cmd.buffer, response, sizeof(response));
  result = TpmReturnCode(response);
  return (result != TPM_SUCCESS);
}

uint32_t TlclForceClear(void) {
  return Send(tpm_forceclear_cmd.buffer);
}

uint32_t TlclSetEnable(void) {
  return Send(tpm_physicalenable_cmd.buffer);
}

uint32_t TlclClearEnable(void) {
  return Send(tpm_physicaldisable_cmd.buffer);
}

uint32_t TlclSetDeactivated(uint8_t flag) {
  *((uint8_t*)tpm_physicalsetdeactivated_cmd.deactivated) = flag;
  return Send(tpm_physicalsetdeactivated_cmd.buffer);
}

uint32_t TlclGetFlags(uint8_t* disable, uint8_t* deactivated) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  TPM_PERMANENT_FLAGS* pflags;
  uint32_t result;
  uint32_t size;

  SendReceive(tpm_getflags_cmd.buffer, response, sizeof(response));
  result = TpmReturnCode(response);
  if (result != TPM_SUCCESS) {
    return result;
  }
  FromTpmUint32(response + kTpmResponseHeaderLength, &size);
  assert(size == sizeof(TPM_PERMANENT_FLAGS));
  pflags =
    (TPM_PERMANENT_FLAGS*) (response + kTpmResponseHeaderLength + sizeof(size));
  *disable = pflags->disable;
  *deactivated = pflags->deactivated;
  return result;
}

uint32_t TlclSetGlobalLock(void) {
  uint32_t x;
  return TlclWrite(TPM_NV_INDEX0, (uint8_t*) &x, 0);
}

uint32_t TlclExtend(int pcr_num, uint8_t* in_digest, uint8_t* out_digest) {
  uint8_t response[kTpmResponseHeaderLength + kPcrDigestLength];
  ToTpmUint32(tpm_extend_cmd.pcrNum, pcr_num);
  memcpy(tpm_extend_cmd.inDigest, in_digest, kPcrDigestLength);
  SendReceive(tpm_extend_cmd.buffer, response, sizeof(response));
  memcpy(out_digest, response + kTpmResponseHeaderLength, kPcrDigestLength);
  return TpmReturnCode(response);
}

uint32_t TlclGetPermissions(uint32_t index, uint32_t* permissions) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  uint8_t* nvdata;
  uint32_t result;
  uint32_t size;

  ToTpmUint32(tpm_getpermissions_cmd.index, index);
  SendReceive(tpm_getpermissions_cmd.buffer, response, sizeof(response));
  result = TpmReturnCode(response);
  if (result != TPM_SUCCESS) {
    return result;
  }
  nvdata = response + kTpmResponseHeaderLength + sizeof(size);
  FromTpmUint32(nvdata + kNvDataPublicPermissionsOffset, permissions);
  return result;
}
