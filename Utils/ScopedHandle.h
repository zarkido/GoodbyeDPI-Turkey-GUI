#pragma once
#include <windows.h>
#include <winsvc.h>
#include <utility>

namespace GoodByDpi_App::Utils
{
    class ScopedHandle
    {
    public:
        ScopedHandle() noexcept : m_handle(nullptr) {}
        explicit ScopedHandle(HANDLE h) noexcept : m_handle(h) {}

        ~ScopedHandle() noexcept
        {
            Reset();
        }

        ScopedHandle(ScopedHandle const&) = delete;
        ScopedHandle& operator=(ScopedHandle const&) = delete;

        ScopedHandle(ScopedHandle&& other) noexcept : m_handle(other.m_handle)
        {
            other.m_handle = nullptr;
        }

        ScopedHandle& operator=(ScopedHandle&& other) noexcept
        {
            if (this != &other)
            {
                Reset(other.m_handle);
                other.m_handle = nullptr;
            }
            return *this;
        }

        bool IsValid() const noexcept
        {
            return m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE;
        }

        explicit operator bool() const noexcept
        {
            return IsValid();
        }

        HANDLE Get() const noexcept
        {
            return m_handle;
        }

        HANDLE* Put() noexcept
        {
            Reset();
            return &m_handle;
        }

        HANDLE Release() noexcept
        {
            HANDLE temp = m_handle;
            m_handle = nullptr;
            return temp;
        }

        void Reset(HANDLE h = nullptr) noexcept
        {
            if (m_handle && m_handle != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(m_handle);
            }
            m_handle = h;
        }

    private:
        HANDLE m_handle;
    };

    class ScopedServiceHandle
    {
    public:
        ScopedServiceHandle() noexcept : m_handle(nullptr) {}
        explicit ScopedServiceHandle(SC_HANDLE h) noexcept : m_handle(h) {}

        ~ScopedServiceHandle() noexcept
        {
            Reset();
        }

        ScopedServiceHandle(ScopedServiceHandle const&) = delete;
        ScopedServiceHandle& operator=(ScopedServiceHandle const&) = delete;

        ScopedServiceHandle(ScopedServiceHandle&& other) noexcept : m_handle(other.m_handle)
        {
            other.m_handle = nullptr;
        }

        ScopedServiceHandle& operator=(ScopedServiceHandle&& other) noexcept
        {
            if (this != &other)
            {
                Reset(other.m_handle);
                other.m_handle = nullptr;
            }
            return *this;
        }

        bool IsValid() const noexcept
        {
            return m_handle != nullptr;
        }

        explicit operator bool() const noexcept
        {
            return IsValid();
        }

        SC_HANDLE Get() const noexcept
        {
            return m_handle;
        }

        SC_HANDLE Release() noexcept
        {
            SC_HANDLE temp = m_handle;
            m_handle = nullptr;
            return temp;
        }

        void Reset(SC_HANDLE h = nullptr) noexcept
        {
            if (m_handle)
            {
                ::CloseServiceHandle(m_handle);
            }
            m_handle = h;
        }

    private:
        SC_HANDLE m_handle;
    };
}
