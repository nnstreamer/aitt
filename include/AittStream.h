/*
 * Copyright 2022-2023 Samsung Electronics Co., Ltd. All Rights Reserved
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

#include <AittTypes.h>

#include <functional>
#include <memory>
#include <string>

namespace aitt {

class AittStream {
  public:
    using StateCallback =
          std::function<void(AittStream *stream, AittStreamState state, void *user_data)>;
    using ReceiveCallback = std::function<void(AittStream *stream, void *data, void *user_data)>;

    AittStream() = default;
    virtual ~AittStream() = default;

    virtual AittStream *SetConfig(const std::string &key, const std::string &value) = 0;
    virtual AittStream *SetConfig(const std::string &key, void *obj) = 0;
    virtual std::string GetFormat(void) = 0;
    virtual int GetWidth(void) = 0;
    virtual int GetHeight(void) = 0;
    virtual void Start(void) = 0;
    virtual void Stop(void) = 0;
    virtual int Push(void *obj) = 0;
    virtual void SetStateCallback(StateCallback cb, void *user_data) = 0;

    // Subscriber ONLY
    virtual void SetReceiveCallback(ReceiveCallback cb, void *user_data) = 0;
    // virtual void SetMediaInfoCallback(???);
};
}  // namespace aitt
