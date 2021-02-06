// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Platform-specific code for FreeBSD goes here. For the POSIX-compatible
// parts, the implementation is in platform-posix.cc.

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>

#include <fcntl.h>  // open
#include <sys/mman.h>   // mmap & munmap
#include <sys/stat.h>   // open
#include <unistd.h>     // getpagesize
// If you don't have execinfo.h then you need devel/libexecinfo from ports.
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <strings.h>    // index

#include <cmath>

#include <OS.h>

#undef MAP_TYPE

#include "src/base/macros.h"
#include "src/base/platform/platform-posix-time.h"
#include "src/base/platform/platform-posix.h"
#include "src/base/platform/platform.h"

namespace v8 {
namespace base {

TimezoneCache* OS::CreateTimezoneCache() {
  return new PosixDefaultTimezoneCache();
}

static unsigned StringToLong(char* buffer) {
  return static_cast<unsigned>(strtol(buffer, nullptr, 16));  // NOLINT
}

std::vector<OS::SharedLibraryAddress> OS::GetSharedLibraryAddresses() {
  return std::vector<SharedLibraryAddress>();
}

void OS::SignalCodeMovingGC() {}

void OS::AdjustSchedulingParams() {}

// static
void* Stack::GetStackStart() {
  thread_info threadInfo;
  get_thread_info(find_thread(NULL), &threadInfo);
  return threadInfo.stack_base;
}

}  // namespace base
}  // namespace v8
