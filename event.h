//
//  event.h
//  helper
//
//  Created by boss on 2020/10/15.
//  Copyright © 2020 boss. All rights reserved.
//

#pragma once

#include   <mutex>
#include   <condition_variable>

namespace helper {
  class Event {
  public:
    Event() {}
    ~Event() {}

    void Set() {
      std::unique_lock<std::mutex> locker(mtx_);
      cv_.notify_one();
    }

    void Wait() {
      std::unique_lock<std::mutex> locker(mtx_);
      cv_.wait(locker);
    }

    // 非超时返回true,超时返回false
    bool Wait(int milliseconeds) {
      std::unique_lock<std::mutex> locker(mtx_);
      auto status = cv_.wait_for(locker,
        std::chrono::milliseconds(milliseconeds));

      return (status != std::cv_status::timeout);
    }

  private:
    std::mutex mtx_;
    std::condition_variable cv_;
  };
}//end namespace helper

