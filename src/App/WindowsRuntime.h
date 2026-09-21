#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <roapi.h>

namespace aidfame {
// Keep WinRT initialized until after Qt releases its cached activation factories.
class WindowsRuntime final {
public:
    WindowsRuntime() : result_(RoInitialize(RO_INIT_SINGLETHREADED)), mtaResult_(CoIncrementMTAUsage(&mtaCookie_)) {}
    ~WindowsRuntime() {
        if (SUCCEEDED(result_)) RoUninitialize();
        if (SUCCEEDED(mtaResult_)) CoDecrementMTAUsage(mtaCookie_);
    }
    bool valid() const { return SUCCEEDED(result_) && SUCCEEDED(mtaResult_); }
    WindowsRuntime(const WindowsRuntime&) = delete;
    WindowsRuntime& operator=(const WindowsRuntime&) = delete;
private:
    HRESULT result_;
    CO_MTA_USAGE_COOKIE mtaCookie_ = nullptr;
    HRESULT mtaResult_;
};
}
