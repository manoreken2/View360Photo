// 日本語。
//*********************************************************
//    Copyright (c) Microsoft. All rights reserved.
//
//    Apache 2.0 License
//
//    You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
//
//*********************************************************

#include "pch.h"
#include "OpenXrProgram.h"
#include "GdiplusHousekeeping.h"
#include <windows.h>
#include <comdef.h>
#include "Config.h"

static bool AttachToConsole(void) {
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        if (GetLastError() != ERROR_ACCESS_DENIED) // already has a console
        {
            if (!AttachConsole(GetCurrentProcessId())) {
                DWORD dwLastError = GetLastError();
                if (dwLastError != ERROR_ACCESS_DENIED) // already has a console
                {
                    return false;
                }
            }
        }
    }

    return true;
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    int rv = S_OK;
    AttachToConsole();

    rv = GdiplusHousekeepingInit();
    if (FAILED(rv)) {
        return 1;
    }

    try {
        auto program = sample::CreateOpenXrProgram(PROGRAM_NAME);
        rv = program->Run();
    } catch (const std::exception& ex) {
        DEBUG_PRINT("Unhandled Exception: %s\n", ex.what());
        rv = E_FAIL;
    } catch (...) {
        DEBUG_PRINT("Unhandled Exception\n");
        rv = E_FAIL;
    }

    GdiplusHousekeepingTerm();

    if (FAILED(rv)) {
        _com_error err(rv);
        LPCTSTR errMsg = err.ErrorMessage();

        MessageBoxW(nullptr, errMsg, L"", MB_OK | MB_ICONERROR);
    }

    return rv;
}
