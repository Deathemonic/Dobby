#include <stdio.h>
#include <cassert>

#include <windows.h>


#include "logging/logging.h"
#include "logging/check_logging.h"
#include "UnifiedInterface/platform.h"

static int GetProtectionFromMemoryPermission(MemoryPermission access) {
  switch (access) {
    case kNoAccess:
      return PAGE_NOACCESS;
    case kRead:
      return PAGE_READONLY;
    case kReadWrite:
      return PAGE_READWRITE;
    case kReadWriteExecute:
      return PAGE_EXECUTE_READWRITE;
    case kReadExecute:
      return PAGE_EXECUTE_READ;
    default:
      return PAGE_NOACCESS;
  }
}

int OSMemory::PageSize() {
  static int lastRet = -1;
  if (lastRet == -1) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    lastRet = si.dwPageSize;
  }
  return lastRet;
}

void *OSMemory::Allocate(size_t size, MemoryPermission access) {
  int prot = GetProtectionFromMemoryPermission(access);
  void *result = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, prot);
  return result;
}

void *OSMemory::Allocate(size_t size, MemoryPermission access, void *fixed_address) {
  int prot = GetProtectionFromMemoryPermission(access);
  void *result = VirtualAlloc(fixed_address, size, MEM_COMMIT | MEM_RESERVE, prot);
  return result;
}

bool OSMemory::Free(void *address, size_t size) {
  (void)size;
  return VirtualFree(address, 0, MEM_RELEASE) != 0;
}

bool OSMemory::Release(void *address, size_t size) {
  return OSMemory::Free(address, size);
}

bool OSMemory::SetPermission(void *address, size_t size, MemoryPermission access) {
  int prot = GetProtectionFromMemoryPermission(access);
  DWORD oldProtect;
  return VirtualProtect(address, size, prot, &oldProtect) != 0;
}

// =====

void OSPrint::Print(const char *format, ...) {
  va_list args;
  va_start(args, format);
  VPrint(format, args);
  va_end(args);
}

void OSPrint::VPrint(const char *format, va_list args) {
  vprintf(format, args);
}
