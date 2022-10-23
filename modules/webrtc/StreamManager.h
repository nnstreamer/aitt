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

#include <WebRtcStream.h>

#include <functional>
#include <string>
#include <thread>

namespace AittWebRTCNamespace {

class StreamManager {
  public:
    using StreamReadyCallback = std::function<void(WebRtcStream &stream)>;
    explicit StreamManager(const std::string &topic, const std::string &aitt_id,
          const std::string &thread_id)
          : topic_(topic), aitt_id_(aitt_id), thread_id_(thread_id){};
    virtual ~StreamManager() = default;
    std::string GetTopic(void) const { return topic_; };
    std::string GetClientId(void) const { return aitt_id_; };
    virtual void HandleRemovedClient(const std::string &id) = 0;
    virtual void HandleDiscoveredStream(const std::string &id,
          const std::vector<uint8_t> &message) = 0;
    virtual void Start(void) = 0;
    virtual void Stop(void) = 0;
    virtual void SetStreamReadyCallback(StreamReadyCallback cb) = 0;

  protected:
    std::string topic_;
    // TODO: why dont' we remove below
    std::string aitt_id_;
    std::string thread_id_;

  private:
    virtual void SetWebRtcStreamCallbacks(WebRtcStream &stream) = 0;
};
}  // namespace AittWebRTCNamespace
