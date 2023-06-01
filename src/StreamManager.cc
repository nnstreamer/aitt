/*
 * Copyright 2023 Samsung Electronics Co., Ltd. All Rights Reserved
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
#include "StreamManager.h"

#include <algorithm>

#include "AittException.h"
#include "aitt_internal.h"

namespace aitt {
#ifndef TIZEN_RT

AittStream *StreamManager::CreateStream(AittStreamProtocol type, const std::string &topic,
      AittStreamRole role)
{
    AittStreamModule *stream = nullptr;
    try {
        stream = module_manager.NewStreamModule(type, topic, role);
        streams_in_use.push_back(stream);
    } catch (std::exception &e) {
        ERR("StreamHandler() Fail(%s)", e.what());
    }

    return stream;
}

void StreamManager::DestroyStream(AittStream *aitt_stream)
{
    auto it = std::find(streams_in_use.begin(), streams_in_use.end(), aitt_stream);
    if (it == streams_in_use.end()) {
        ERR("Unknown Stream(%p)", aitt_stream);
        return;
    }
    streams_in_use.erase(it);
    delete aitt_stream;
}

void StreamManager::DestroyStreamAll(void)
{
    for (auto stream : streams_in_use)
        delete stream;
    streams_in_use.clear();
}
#else
AittStream *StreamManager::CreateStream(AittStreamProtocol type, const std::string &topic,
      AittStreamRole role)
{
    throw AittException(AittException::NOT_SUPPORTED);
}
void StreamManager::DestroyStream(AittStream *aitt_stream)
{
}
void StreamManager::DestroyStreamAll(void)
{
}
#endif
}  // namespace aitt
