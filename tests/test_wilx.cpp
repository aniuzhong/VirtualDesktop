// wilx behavior tests: run via ctest, or the test_wilx executable directly.
#include <windows.h>
#include <wilx/desktops.h>
#include <wilx/win32_helpers.h>

#include <string>
#include <type_traits>
#include <vector>

#define CHECK(x) do { if (!(x)) { return 1; } } while (0)

int main()
{
    // TryGet* total fail-soft: empty == failure, never an exception.
    CHECK(!wilx::TryGetWin32ErrorMessage(ERROR_FILE_NOT_FOUND).empty());
    CHECK(wilx::TryGetUtf8String(L"").empty());
    CHECK(wilx::TryGetUtf8String(L"abc") == "abc");
    CHECK(wilx::TryGetUtf8String(L"\x4f60\x597d") == "\xe4\xbd\xa0\xe5\xa5\xbd");
    CHECK(wilx::TryGetWindowText(nullptr).empty());

    CHECK(!wilx::TryGetUserObjectName(GetProcessWindowStation()).empty());
    CHECK(!wilx::TryGetThreadDesktopName().empty());

    // Window text round-trip on a real (system-class) window.
    HWND window = CreateWindowExW(0, L"STATIC", L"hello", 0, 0, 0, 0, 0,
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    CHECK(window != nullptr);
    CHECK(wilx::TryGetWindowText(window) == L"hello");
    DestroyWindow(window);

    // Enumeration: non-empty, and a false-returning callback stops after one.
    std::vector<std::wstring> names;
    wilx::for_each_desktop_nothrow([&](PCWSTR name) { names.emplace_back(name); });
    CHECK(!names.empty());

    size_t visited = 0;
    wilx::for_each_desktop_nothrow([&](PCWSTR) -> bool { ++visited; return false; });
    CHECK(visited == 1);

    static_assert(std::is_base_of_v<NOTIFYICONDATAW, wilx::unique_notify_icon_data>);
    wilx::unique_notify_icon_data icon;
    icon.cbSize = sizeof(icon);
    CHECK(icon.szTip[0] == L'\0');

    return 0;
}
