#include "PlatformUtil/ProcessRuntimeUtility.h"

#include <vector>

#include <windows.h>
#include <psapi.h>

#define LINE_MAX 2048

// ================================================================
// GetProcessMemoryLayout

static bool memory_region_comparator(MemRegion a, MemRegion b) {
  return (a.start > b.start);
}

// https://gist.github.com/jedwardsol/9d4fe1fd806043a5767affbd200088ca

static std::vector<MemRegion> ProcessMemoryLayout;
const std::vector<MemRegion> &ProcessRuntimeUtility::GetProcessMemoryLayout() {
  if (!ProcessMemoryLayout.empty()) {
    ProcessMemoryLayout.clear();
  }

  char *address{nullptr};
  MEMORY_BASIC_INFORMATION region;

  while (VirtualQuery(address, &region, sizeof(region))) {
    address += region.RegionSize;
    if (!(region.State & (MEM_COMMIT | MEM_RESERVE))) {
      continue;
    }

    MemoryPermission permission = MemoryPermission::kNoAccess;
    auto mask = PAGE_GUARD | PAGE_NOCACHE | PAGE_WRITECOMBINE;
    switch (region.Protect & ~mask) {
    case PAGE_NOACCESS:
    case PAGE_READONLY:
      permission = MemoryPermission::kRead;
      break;

    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
      permission = MemoryPermission::kReadExecute;
      break;

    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
      permission = MemoryPermission::kReadWrite;
      break;

    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
      permission = MemoryPermission::kReadWriteExecute;
      break;
    }

    ProcessMemoryLayout.push_back(MemRegion{(addr_t)region.BaseAddress, region.RegionSize, permission});
  }
  return ProcessMemoryLayout;
}

// ================================================================
// GetProcessModuleMap

static std::vector<RuntimeModule> ProcessModuleMap;

const std::vector<RuntimeModule> &ProcessRuntimeUtility::GetProcessModuleMap() {
  if (!ProcessModuleMap.empty()) {
    ProcessModuleMap.clear();
  }

  HMODULE hMods[1024];
  DWORD cbNeeded;
  HANDLE hProcess = GetCurrentProcess();

  if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
    for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
      RuntimeModule module = {0};
      GetModuleFileNameExA(hProcess, hMods[i], module.path, sizeof(module.path));
      module.load_address = (void *)hMods[i];
      ProcessModuleMap.push_back(module);
    }
  }

  return ProcessModuleMap;
}

RuntimeModule ProcessRuntimeUtility::GetProcessModule(const char *name) {
  std::vector<RuntimeModule> ProcessModuleMap = GetProcessModuleMap();
  for (auto module : ProcessModuleMap) {
    if (strstr(module.path, name) != 0) {
      return module;
    }
  }
  return RuntimeModule{0};
}