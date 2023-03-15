/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace AittWebRTCNamespace {
template <typename T>
class SyncQueue {
  public:
    bool IsEmpty(void)
    {
        std::lock_guard<std::mutex> queue_lock(mutex_);
        return queue_.empty();
    };

    void Push(T data)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(data);
        condition_.notify_one();
    };

    void Pop(void)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.pop();
    };

    T WaitedFront(void)
    {
        std::unique_lock<std::mutex> scanned(mutex_);
        condition_.wait(scanned, [&] { return !queue_.empty(); });

        T data = queue_.front();
        return data;
    };

    T WaitedPop(void)
    {
        std::unique_lock<std::mutex> scanned(mutex_);
        condition_.wait(scanned, [&] { return !queue_.empty(); });

        T data = queue_.front();
        queue_.pop();
        scanned.unlock();

        return data;
    };

    void WakeUpAll(void) { condition_.notify_all(); };

  private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable condition_;
};
}  // namespace AittWebRTCNamespace
