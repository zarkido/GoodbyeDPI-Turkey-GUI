#pragma once
#include <string>

namespace GoodByDpi_App::Services
{
    class AutoStartManager
    {
    public:
        static void SetAutoStart(bool enable);
        static void SyncPath();

    private:
        static void ApplyTaskScheduler(bool enable, std::wstring const& targetExePath);
    };
}
