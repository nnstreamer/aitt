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
#include "MainLoopHandler.h"

#ifdef USE_GLIB
#include "GlibMainLoop.h"
#else
#include "PosixMainLoop.h"
#endif
#include "aitt_internal.h"

namespace aitt {

MainLoopHandler::MainLoopHandler()
      :
#ifdef USE_GLIB
        loop(new GlibMainLoop())
#else
        loop(new PosixMainLoop())
#endif
{
}

void MainLoopHandler::AddIdle(MainLoopHandler *handle, const mainLoopCB &cb,
      MainLoopData *user_data)
{
    RET_IF(handle == nullptr);

    handle->loop->AddIdle(cb, user_data);
}

void MainLoopHandler::Run()
{
    return loop->Run();
}

bool MainLoopHandler::Quit()
{
    return loop->Quit();
}

void MainLoopHandler::AddWatch(int fd, const mainLoopCB &cb, MainLoopData *user_data)
{
    return loop->AddWatch(fd, cb, user_data);
}

void MainLoopHandler::AddIdle(const mainLoopCB &cb, MainLoopData *user_data)
{
    return loop->AddIdle(cb, user_data);
}

MainLoopIface::MainLoopData *MainLoopHandler::RemoveWatch(int fd)
{
    return loop->RemoveWatch(fd);
}

unsigned int MainLoopHandler::AddTimeout(int interval, const mainLoopCB &cb,
      MainLoopData *user_data)
{
    return loop->AddTimeout(interval, cb, user_data);
}

void MainLoopHandler::RemoveTimeout(unsigned int id)
{
    return loop->RemoveTimeout(id);
}

}  // namespace aitt
