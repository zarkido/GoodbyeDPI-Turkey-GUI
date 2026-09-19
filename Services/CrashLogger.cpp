#include "pch.h"
#include "CrashLogger.h"
#include "Utils/PathUtils.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <shlobj.h>

namespace GoodByDpi_App::Services
{
    static std::wstring GetTimestampString()
    {
        auto now = std::chrono::system_clock::now();
        auto inTime = std::chrono::system_clock::to_time_t(now);
        struct tm tmBuf {};
        localtime_s(&tmBuf, &inTime);
        std::wstringstream wss;
        wss << std::setfill(L'0')
            << std::setw(4) << (tmBuf.tm_year + 1900) << L"-"
            << std::setw(2) << (tmBuf.tm_mon + 1) << L"-"
            << std::setw(2) << tmBuf.tm_mday << L" "
            << std::setw(2) << tmBuf.tm_hour << L":"
            << std::setw(2) << tmBuf.tm_min << L":"
            << std::setw(2) << tmBuf.tm_sec;
        return wss.str();
    }

    static std::wstring ExceptionCodeToString(DWORD code)
    {
        switch (code)
        {
        case EXCEPTION_ACCESS_VIOLATION: return L"EXCEPTION_ACCESS_VIOLATION (0xC0000005)";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return L"EXCEPTION_ARRAY_BOUNDS_EXCEEDED (0xC000008C)";
        case EXCEPTION_BREAKPOINT: return L"EXCEPTION_BREAKPOINT (0x80000003)";
        case EXCEPTION_DATATYPE_MISALIGNMENT: return L"EXCEPTION_DATATYPE_MISALIGNMENT (0x80000002)";
        case EXCEPTION_FLT_DENORMAL_OPERAND: return L"EXCEPTION_FLT_DENORMAL_OPERAND (0xC000008D)";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO: return L"EXCEPTION_FLT_DIVIDE_BY_ZERO (0xC000008E)";
        case EXCEPTION_FLT_INEXACT_RESULT: return L"EXCEPTION_FLT_INEXACT_RESULT (0xC000008F)";
        case EXCEPTION_FLT_INVALID_OPERATION: return L"EXCEPTION_FLT_INVALID_OPERATION (0xC0000090)";
        case EXCEPTION_FLT_OVERFLOW: return L"EXCEPTION_FLT_OVERFLOW (0xC0000091)";
        case EXCEPTION_FLT_STACK_CHECK: return L"EXCEPTION_FLT_STACK_CHECK (0xC0000092)";
        case EXCEPTION_FLT_UNDERFLOW: return L"EXCEPTION_FLT_UNDERFLOW (0xC0000093)";
        case EXCEPTION_ILLEGAL_INSTRUCTION: return L"EXCEPTION_ILLEGAL_INSTRUCTION (0xC000001D)";
        case EXCEPTION_IN_PAGE_ERROR: return L"EXCEPTION_IN_PAGE_ERROR (0xC0000006)";
        case EXCEPTION_INT_DIVIDE_BY_ZERO: return L"EXCEPTION_INT_DIVIDE_BY_ZERO (0xC0000094)";
        case EXCEPTION_INT_OVERFLOW: return L"EXCEPTION_INT_OVERFLOW (0xC0000095)";
        case EXCEPTION_INVALID_DISPOSITION: return L"EXCEPTION_INVALID_DISPOSITION (0xC0000026)";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: return L"EXCEPTION_NONCONTINUABLE_EXCEPTION (0xC0000025)";
        case EXCEPTION_PRIV_INSTRUCTION: return L"EXCEPTION_PRIV_INSTRUCTION (0xC0000096)";
        case EXCEPTION_SINGLE_STEP: return L"EXCEPTION_SINGLE_STEP (0x80000004)";
        case EXCEPTION_STACK_OVERFLOW: return L"EXCEPTION_STACK_OVERFLOW (0xC00000FD)";
        case 0xE06D7363: return L"MSVC_CPP_EXCEPTION (0xE06D7363)";
        default:
        {
            std::wstringstream ss;
            ss << L"0x" << std::hex << std::uppercase << code;
            return ss.str();
        }
        }
    }

    CrashLogger& CrashLogger::Instance()
    {
        static CrashLogger s_instance;
        return s_instance;
    }

    CrashLogger::CrashLogger()
    {
        m_logPath = Utils::GetCrashLogFilePath();
        std::error_code ec;
        std::filesystem::create_directories(m_logPath.parent_path(), ec);
    }

    void CrashLogger::Initialize()
    {
        if (m_initialized) return;
        m_initialized = true;

        ::SetUnhandledExceptionFilter(UnhandledFilter);
        std::set_terminate(TerminateHandler);
    }

    std::filesystem::path CrashLogger::GetCrashLogPath() const
    {
        return m_logPath;
    }

    void CrashLogger::LogMessage(std::wstring const& category, std::wstring const& message)
    {
        std::lock_guard lock(m_mutex);
        std::error_code ec;
        std::filesystem::create_directories(m_logPath.parent_path(), ec);

        std::wofstream file(m_logPath, std::ios::app);
        if (file.is_open())
        {
            file << L"[" << GetTimestampString() << L"] [" << category << L"] " << message << L"\n";
        }
    }

    void CrashLogger::LogXamlException(std::wstring const& message)
    {
        LogMessage(L"XAML_EXCEPTION", message);
    }

    LONG WINAPI CrashLogger::UnhandledFilter(PEXCEPTION_POINTERS pEx)
    {
        Instance().LogException(pEx);
        return EXCEPTION_CONTINUE_SEARCH;
    }

    void CrashLogger::TerminateHandler()
    {
        Instance().LogMessage(L"TERMINATE", L"std::terminate was invoked.");
        std::abort();
    }

    void CrashLogger::LogException(PEXCEPTION_POINTERS pEx)
    {
        std::lock_guard lock(m_mutex);
        if (!pEx || !pEx->ExceptionRecord) return;

        std::error_code ec;
        std::filesystem::create_directories(m_logPath.parent_path(), ec);

        std::wofstream file(m_logPath, std::ios::app);
        if (!file.is_open()) return;

        DWORD code = pEx->ExceptionRecord->ExceptionCode;
        PVOID address = pEx->ExceptionRecord->ExceptionAddress;

        wchar_t moduleName[MAX_PATH] = L"Unknown";
        HMODULE hMod = nullptr;
        if (::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(address), &hMod) && hMod)
        {
            ::GetModuleFileNameW(hMod, moduleName, MAX_PATH);
        }

        file << L"================================================================\n";
        file << L"[" << GetTimestampString() << L"] CRASH DETECTED\n";
        file << L"Exception Code: " << ExceptionCodeToString(code) << L"\n";
        file << L"Fault Address: 0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(address) << L"\n";
        file << L"Fault Module: " << moduleName << L"\n";
        file << L"Thread ID: " << std::dec << ::GetCurrentThreadId() << L"\n";
        file << L"Process ID: " << std::dec << ::GetCurrentProcessId() << L"\n";

        if (code == EXCEPTION_ACCESS_VIOLATION && pEx->ExceptionRecord->NumberParameters >= 2)
        {
            ULONG_PTR type = pEx->ExceptionRecord->ExceptionInformation[0];
            ULONG_PTR targetAddr = pEx->ExceptionRecord->ExceptionInformation[1];
            file << L"Access Type: " << (type == 0 ? L"Read" : (type == 1 ? L"Write" : L"Execute")) << L"\n";
            file << L"Target Address: 0x" << std::hex << std::uppercase << targetAddr << L"\n";
        }

        if (pEx->ContextRecord)
        {
            PCONTEXT ctx = pEx->ContextRecord;
            file << L"Registers:\n";
            file << L"  RAX: 0x" << std::hex << std::uppercase << ctx->Rax << L"  RBX: 0x" << ctx->Rbx << L"  RCX: 0x" << ctx->Rcx << L"  RDX: 0x" << ctx->Rdx << L"\n";
            file << L"  RSI: 0x" << std::hex << std::uppercase << ctx->Rsi << L"  RDI: 0x" << ctx->Rdi << L"  RBP: 0x" << ctx->Rbp << L"  RSP: 0x" << ctx->Rsp << L"\n";
            file << L"  RIP: 0x" << std::hex << std::uppercase << ctx->Rip << L"  EFL: 0x" << ctx->EFlags << L"\n";
        }

        file << L"================================================================\n\n";
        file.flush();
    }
}
