#include "pch.h"
#include "AutoStartManager.h"
#include "Utils/PathUtils.h"
#include <filesystem>
#include <vector>
#include <taskschd.h>
#include <comdef.h>
#pragma comment(lib, "taskschd.lib")

namespace GoodByDpi_App::Services
{
    void AutoStartManager::ApplyTaskScheduler(bool enable, std::wstring const& targetExePath)
    {
        HRESULT hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        bool coInit = SUCCEEDED(hr);

        ITaskService* pService = nullptr;
        hr = ::CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, reinterpret_cast<void**>(&pService));
        if (SUCCEEDED(hr) && pService)
        {
            hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
            if (SUCCEEDED(hr))
            {
                ITaskFolder* pRootFolder = nullptr;
                hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
                if (SUCCEEDED(hr) && pRootFolder)
                {
                    pRootFolder->DeleteTask(_bstr_t(L"GoodByDpi"), 0);

                    if (enable && !targetExePath.empty())
                    {
                        ITaskDefinition* pTask = nullptr;
                        hr = pService->NewTask(0, &pTask);
                        if (SUCCEEDED(hr) && pTask)
                        {
                            IRegistrationInfo* pRegInfo = nullptr;
                            if (SUCCEEDED(pTask->get_RegistrationInfo(&pRegInfo)) && pRegInfo)
                            {
                                pRegInfo->put_Author(_bstr_t(L"GoodByDpi"));
                                pRegInfo->Release();
                            }

                            IPrincipal* pPrincipal = nullptr;
                            if (SUCCEEDED(pTask->get_Principal(&pPrincipal)) && pPrincipal)
                            {
                                pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
                                pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
                                pPrincipal->Release();
                            }

                            ITaskSettings* pSettings = nullptr;
                            if (SUCCEEDED(pTask->get_Settings(&pSettings)) && pSettings)
                            {
                                pSettings->put_StartWhenAvailable(VARIANT_TRUE);
                                pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
                                pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
                                pSettings->put_ExecutionTimeLimit(_bstr_t(L"PT0S"));
                                pSettings->Release();
                            }

                            ITriggerCollection* pTriggerCollection = nullptr;
                            if (SUCCEEDED(pTask->get_Triggers(&pTriggerCollection)) && pTriggerCollection)
                            {
                                ITrigger* pTrigger = nullptr;
                                if (SUCCEEDED(pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger)) && pTrigger)
                                {
                                    ILogonTrigger* pLogonTrigger = nullptr;
                                    if (SUCCEEDED(pTrigger->QueryInterface(IID_ILogonTrigger, reinterpret_cast<void**>(&pLogonTrigger))) && pLogonTrigger)
                                    {
                                        pLogonTrigger->put_Enabled(VARIANT_TRUE);
                                        pLogonTrigger->Release();
                                    }
                                    pTrigger->Release();
                                }
                                pTriggerCollection->Release();
                            }

                            IActionCollection* pActionCollection = nullptr;
                            if (SUCCEEDED(pTask->get_Actions(&pActionCollection)) && pActionCollection)
                            {
                                IAction* pAction = nullptr;
                                if (SUCCEEDED(pActionCollection->Create(TASK_ACTION_EXEC, &pAction)) && pAction)
                                {
                                    IExecAction* pExecAction = nullptr;
                                    if (SUCCEEDED(pAction->QueryInterface(IID_IExecAction, reinterpret_cast<void**>(&pExecAction))) && pExecAction)
                                    {
                                        pExecAction->put_Path(_bstr_t(targetExePath.c_str()));
                                        pExecAction->put_Arguments(_bstr_t(L"--autostart"));
                                        pExecAction->Release();
                                    }
                                    pAction->Release();
                                }
                                pActionCollection->Release();
                            }

                            IRegisteredTask* pRegisteredTask = nullptr;
                            pRootFolder->RegisterTaskDefinition(
                                _bstr_t(L"GoodByDpi"),
                                pTask,
                                TASK_CREATE_OR_UPDATE,
                                _variant_t(),
                                _variant_t(),
                                TASK_LOGON_INTERACTIVE_TOKEN,
                                _variant_t(L""),
                                &pRegisteredTask
                            );
                            if (pRegisteredTask) pRegisteredTask->Release();
                            pTask->Release();
                        }
                    }
                    pRootFolder->Release();
                }
            }
            pService->Release();
        }

        if (coInit)
        {
            ::CoUninitialize();
        }
    }

    void AutoStartManager::SetAutoStart(bool enable)
    {
        std::filesystem::path permanentExe(Utils::GetPermanentLauncherPath());
        std::error_code ec;

        if (enable)
        {
            wchar_t* launcherEnv = _wgetenv(L"GOODBYDPI_LAUNCHER_EXE");
            if (launcherEnv && wcslen(launcherEnv) > 0 && std::filesystem::exists(launcherEnv, ec))
            {
                std::filesystem::path currentLauncher(launcherEnv);
                std::filesystem::create_directories(permanentExe.parent_path(), ec);
                if (!std::filesystem::equivalent(currentLauncher, permanentExe, ec))
                {
                    std::filesystem::copy_file(currentLauncher, permanentExe, std::filesystem::copy_options::overwrite_existing, ec);
                }
            }

            std::wstring targetExePath;
            if (std::filesystem::exists(permanentExe, ec))
            {
                targetExePath = permanentExe.wstring();
            }
            else if (launcherEnv && wcslen(launcherEnv) > 0)
            {
                targetExePath = launcherEnv;
            }
            else
            {
                wchar_t exeBuffer[MAX_PATH];
                ::GetModuleFileNameW(nullptr, exeBuffer, MAX_PATH);
                targetExePath = exeBuffer;
            }

            std::wstring quoted = L"\"" + targetExePath + L"\" --autostart";

            HKEY hKey;
            if (::RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
            {
                ::RegSetValueExW(hKey, L"GoodByDpi", 0, REG_SZ, reinterpret_cast<BYTE const*>(quoted.c_str()), static_cast<DWORD>((quoted.length() + 1) * sizeof(wchar_t)));
                ::RegCloseKey(hKey);
            }

            ApplyTaskScheduler(true, targetExePath);
        }
        else
        {
            HKEY hKey;
            if (::RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
            {
                ::RegDeleteValueW(hKey, L"GoodByDpi");
                ::RegCloseKey(hKey);
            }

            ApplyTaskScheduler(false, L"");
        }
    }

    void AutoStartManager::SyncPath()
    {
        SetAutoStart(true);
    }
}
