// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/system/sys_info.h"

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
    system_info systemInfo;
    get_system_info(&systemInfo);
    return static_cast<int64_t>((systemInfo.max_pages - systemInfo.used_pages)
    	* (B_PAGE_SIZE / 1048576.0f) + 0.5f);
}


}  // namespace base
