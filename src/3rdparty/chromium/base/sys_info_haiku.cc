// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/sys_info.h"

#include "base/logging.h"

#include <kernel/OS.h>

namespace base {

// static
int64_t SysInfo::AmountOfPhysicalMemoryImpl() {
    system_info systemInfo;
    get_system_info(&systemInfo);
    return static_cast<int64_t>(systemInfo.max_pages * (B_PAGE_SIZE / 1048576.0f) + 0.5f);
}

// static
int64_t SysInfo::AmountOfAvailablePhysicalMemoryImpl() {
  // TODO(fuchsia): https://crbug.com/706592 This is not exposed.
  NOTREACHED();
  return 0;
}

// static
int SysInfo::NumberOfProcessors() {
    system_info systemInfo;
    get_system_info(&systemInfo);
    return static_cast<int>(systemInfo.cpu_count);
}

// static
int64_t SysInfo::AmountOfVirtualMemory() {
  return 0;
}

}  // namespace base
