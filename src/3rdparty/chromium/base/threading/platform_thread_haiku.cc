// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/platform_thread.h"

#include <pthread.h>
#include <sched.h>

#include "base/threading/platform_thread_internal_posix.h"
#include "base/threading/thread_id_name_manager.h"

namespace base {

namespace internal {

const ThreadPriorityToNiceValuePair kThreadPriorityToNiceValueMap[4] = {
    {ThreadPriority::BACKGROUND, 10},
    {ThreadPriority::NORMAL, 0},
    {ThreadPriority::DISPLAY, -8},
    {ThreadPriority::REALTIME_AUDIO, -10},
};

Optional<bool> CanIncreaseCurrentThreadPriorityForPlatform(
    ThreadPriority priority) {
  return base::nullopt;
}

bool SetCurrentThreadPriorityForPlatform(ThreadPriority priority) {
  sched_param prio = {0};
  prio.sched_priority = ThreadPriorityToNiceValue(priority);
  return pthread_setschedparam(pthread_self(), SCHED_OTHER, &prio) == 0;
}

Optional<ThreadPriority> GetCurrentThreadPriorityForPlatform() {
  int maybe_sched_rr = 0;
  struct sched_param maybe_realtime_prio = {0};
  if (pthread_getschedparam(pthread_self(), &maybe_sched_rr,
                            &maybe_realtime_prio) == 0 &&
      maybe_sched_rr == SCHED_RR &&
      maybe_realtime_prio.sched_priority >= 100) {
    return base::make_optional(ThreadPriority::REALTIME_AUDIO);
  }
  return base::nullopt;
}

}  // namespace internal


// static
void PlatformThread::SetThreadPriority(PlatformThreadId thread_id,
                                       ThreadPriority priority) {
  // Changing current main threads' priority is not permitted in favor of
  // security, this interface is restricted to change only non-main thread
  // priority.
  CHECK_NE(thread_id, getpid());

  const int nice_setting = internal::ThreadPriorityToNiceValue(priority);
  if (setpriority(PRIO_PROCESS, thread_id, nice_setting)) {
    DVPLOG(1) << "Failed to set nice value of thread (" << thread_id << ") to "
              << nice_setting;
  }
}

void InitThreading() {}

void TerminateOnThread() {}

size_t GetDefaultThreadStackSize(const pthread_attr_t& attributes) {
  return 0;
}

// static
void PlatformThread::SetName(const std::string& name) {
  
}

}  // namespace base
