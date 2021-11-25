// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/process/process_handle.h"

#include "base/files/file_util.h"

#define PARENT_ID 3
extern "C" pid_t _kern_process_info(pid_t, int);

namespace base {

ProcessId GetParentProcessId(ProcessHandle process) {
  return _kern_process_info(process, PARENT_ID);
}

FilePath GetProcessExecutablePath(ProcessHandle process) {
  NOTIMPLEMENTED();
  return FilePath();
}

}  // namespace base
