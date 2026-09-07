# wilx

Header-only, WIL-style extensions for Virtual Desktop. One theme per header,
organized like wil: `details` trampolines, thin public APIs.

## Files

| File | Theme |
|---|---|
| `desktops.h` | Desktop enumeration (`for_each_desktop`) — earned by its trampoline machinery |
| `win32_helpers.h` | Default drawer for single-pattern Win32 helpers (user-object name queries) |

## Naming grammar

Every new name must answer these questions; deviations are declared in the
header that owns the name.

| Question | Encoding |
|---|---|
| Knowledge source | `PascalCase` = Win32 vocabulary (contract follows the API's docs); `lowerCamel` = invented abstraction (contract lives in this header) |
| Failure contract | `Get*` throws on failure; `TryGet*` fail-soft, failure returned as data; `*NoThrow` / `*_nothrow` = exceptions banned, failure via return value |
| Ownership | `unique_*` / `shared_*` = RAII; the handle type encodes the deleter |
| Shape | `for_each_*` = callback-driven algorithm; callback returns void (continue), bool (false stops), or HRESULT (S_OK continues) |
| Placement | A theme header is earned by machinery or mass; single-pattern helpers go to `win32_helpers.h` |

## Deliberate deviations from wil

- `std::` instead of wil's internal `wistd::`
- `__cpp_exceptions` instead of `WIL_ENABLE_EXCEPTIONS`
- C++20 concepts instead of `always_false` + `static_assert`
- No `W` suffix on invented composite names (reserved for API mirrors with A/W duality)
