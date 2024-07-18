#pragma once
// clang-format off
#include <string>
#if defined(WIN32)
#include <windows.h>
#include <processthreadsapi.h>

#include "coding_helper.h"
#elif defined(__LINUX__)
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h> 
#elif defined(__ANDROID__)
#include <pthread.h>
#include <sys/prctl.h>
#include <unistd.h>
#if defined(__MACH__) || defined(__APPLE__)
#include <pthread.h>
#include <pthread_spis.h>
#include <unistd.h>
#endif
#endif

namespace helper {

#if defined(WIN32)
  typedef DWORD PlatformThreadId;
  typedef DWORD PlatformThreadRef;
#elif defined(__LINUX__) || defined(__ANDROID__) || defined(__MACH__) || defined(__APPLE__)
  typedef pid_t PlatformThreadId;
  typedef pthread_t PlatformThreadRef;
#endif

  static inline PlatformThreadId CurrentThreadId() {
#if defined(WIN32)
    return GetCurrentThreadId();
#elif defined(__MACH__) || defined(__APPLE__)
    return pthread_mach_thread_np(pthread_self());
#elif defined(__ANDROID__)
    return gettid();
#elif defined(__LINUX__)
   return syscall(__NR_gettid);
#else
    // Default implementation for nacl and solaris.
    return reinterpret_cast<PlatformThreadId>(pthread_self());
#endif
  }

  static inline void SetCurrentThreadName(const std::string& name) {
#if defined(WIN32)
    typedef HRESULT(WINAPI* RTC_SetThreadDescription)(HANDLE hThread, PCWSTR lpThreadDescription);
    static auto set_thread_description_func = reinterpret_cast<RTC_SetThreadDescription>
        (::GetProcAddress(::GetModuleHandleA("Kernel32.dll"), "SetThreadDescription"));
    if (set_thread_description_func) {
        // The SetThreadDescription API works even if no debugger is attached.
        // The names set with this API also show up in ETW traces. Very handy.
        std::wstring wide_thread_name = helper::Utf82Unicode(name);
        set_thread_description_func(::GetCurrentThread(), wide_thread_name.c_str());
  }
#elif defined(__LINUX__) || defined(__ANDROID__)
    prctl(PR_SET_NAME, reinterpret_cast<unsigned long>(name.c_str()));  // NOLINT
#elif defined(__MACH__) || defined(__APPLE__)
    pthread_setname_np(name.c_str());
#endif
  }
}  // end namespace helper
// clang-format on
