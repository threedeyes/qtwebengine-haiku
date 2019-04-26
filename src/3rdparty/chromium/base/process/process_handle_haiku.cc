// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/process/process_handle.h"

#include "base/files/file_util.h"

namespace base {

ProcessId GetParentProcessId(ProcessHandle process) {
  NOTIMPLEMENTED();
  return -1;
}

FilePath GetProcessExecutablePath(ProcessHandle process) {
  NOTIMPLEMENTED();
  return FilePath();
}

}  // namespace base
