#include "pch.h"
#include "AutoStartManager.h"
#include "Utils/PathUtils.h"
#include <filesystem>
#include <vector>
#include <taskschd.h>
#include <comdef.h>
#pragma comment(lib, "taskschd.lib")

_COM_SMARTPTR_TYPEDEF(ITaskService, IID_ITaskService);
_COM_SMARTPTR_TYPEDEF(ITaskFolder, IID_ITaskFolder);
_COM_SMARTPTR_TYPEDEF(ITaskDefinition, IID_ITaskDefinition);
_COM_SMARTPTR_TYPEDEF(IRegistrationInfo, IID_IRegistrationInfo);
_COM_SMARTPTR_TYPEDEF(IPrincipal, IID_IPrincipal);
_COM_SMARTPTR_TYPEDEF(ITaskSettings, IID_ITaskSettings);
_COM_SMARTPTR_TYPEDEF(ITriggerCollection, IID_ITriggerCollection);
_COM_SMARTPTR_TYPEDEF(ITrigger, IID_ITrigger);
_COM_SMARTPTR_TYPEDEF(ILogonTrigger, IID_ILogonTrigger);
_COM_SMARTPTR_TYPEDEF(IActionCollection, IID_IActionCollection);
_COM_SMARTPTR_TYPEDEF(IAction, IID_IAction);
_COM_SMARTPTR_TYPEDEF(IExecAction, IID_IExecAction);
_COM_SMARTPTR_TYPEDEF(IRegisteredTask, IID_IRegisteredTask);

namespace GoodByDpi_App::Services
{
    void AutoStartManager::ApplyTaskScheduler(bool enable, std::wstring const& targetExePath)
    {
        HRESULT hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        bool coInit = SUCCEEDED(hr);

        {
            ITaskServicePtr pService;
            hr = ::CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, reinterpret_cast<void**>(&pService));
            if (SUCCEEDED(hr) && pService)
            {
                hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
                if (SUCCEEDED(hr))
                {
                    ITaskFolderPtr pRootFolder;
                    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
                    if (SUCCEEDED(hr) && pRootFolder)
                    {
                        pRootFolder->DeleteTask(_bstr_t(L"GoodByDpi"), 0);

                        if (enable && !targetExePath.empty())
                        {
                            ITaskDefinitionPtr pTask;
                            hr = pService->NewTask(0, &pTask);
                            if (SUCCEEDED(hr) && pTask)
                            {
                                IRegistrationInfoPtr pRegInfo;
                                if (SUCCEEDED(pTask->get_RegistrationInfo(&pRegInfo)) && pRegInfo)
                                {
                                    pRegInfo->put_Author(_bstr_t(L"GoodByDpi"));
                                }

                                IPrincipalPtr pPrincipal;
                                if (SUCCEEDED(pTask->get_Principal(&pPrincipal)) && pPrincipal)
                                {
                                    pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
                                    pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
                                }

                                ITaskSettingsPtr pSettings;
                                if (SUCCEEDED(pTask->get_Settings(&pSettings)) && pSettings)
                                {
                                    pSettings->put_StartWhenAvailable(VARIANT_TRUE);
                                    pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
                                    pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
                                    pSettings->put_ExecutionTimeLimit(_bstr_t(L"PT0S"));
                                }

                                ITriggerCollectionPtr pTriggerCollection;
                                if (SUCCEEDED(pTask->get_Triggers(&pTriggerCollection)) && pTriggerCollection)
                                {
                                    ITriggerPtr pTrigger;
                                    if (SUCCEEDED(pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger)) && pTrigger)
                                    {
                                        ILogonTriggerPtr pLogonTrigger;
                                        if (SUCCEEDED(pTrigger->QueryInterface(IID_ILogonTrigger, reinterpret_cast<void**>(&pLogonTrigger))) && pLogonTrigger)
                                        {
                                            pLogonTrigger->put_Enabled(VARIANT_TRUE);
                                        }
                                    }
                                }

                                IActionCollectionPtr pActionCollection;
                                if (SUCCEEDED(pTask->get_Actions(&pActionCollection)) && pActionCollection)
                                {
                                    IActionPtr pAction;
                                    if (SUCCEEDED(pActionCollection->Create(TASK_ACTION_EXEC, &pAction)) && pAction)
                                    {
                                        IExecActionPtr pExecAction;
                                        if (SUCCEEDED(pAction->QueryInterface(IID_IExecAction, reinterpret_cast<void**>(&pExecAction))) && pExecAction)
                                        {
                                            pExecAction->put_Path(_bstr_t(targetExePath.c_str()));
                                            pExecAction->put_Arguments(_bstr_t(L"--autostart"));
                                        }
                                    }
                                }

                                IRegisteredTaskPtr pRegisteredTask;
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
                            }
                        }
                    }
                }
            }
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
                targetExePath = Utils::GetExecutablePath().wstring();
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
