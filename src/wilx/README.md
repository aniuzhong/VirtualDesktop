# wilx

Header-only, WIL-style extensions for Virtual Desktop, organized like wil:
machinery earns a header, single-pattern helpers live in the drawer.

## House rules

- Build on wil's **public API only**; `wil::details` is reference material,
  never a dependency — copy and own its evolution if ever truly needed.
- C++23 and up: no historical back-compat layers.
- A helper enters wilx only if it passes all three checks: no business
  vocabulary in names or signatures, no touching app globals, and usable
  as-is by any Win32 program. wilx provides primitives and failure semantics;
  the app provides policy (caching, confirmation dialogs, relaunch logic).
- Grow on demand: one failure regime until a caller needs another; no
  speculative overloads.

## Files

| File | Theme |
|---|---|
| `desktops.h` | Desktop enumeration (`for_each_desktop`) — earned by its trampoline machinery |
| `win32_helpers.h` | The drawer: user-object name queries, error-message formatting, UTF-8 conversion, window text, tray icon RAII |

`unique_notify_icon_data`: fill the fields (including `cbSize`), `NIM_ADD` it,
and keep the object alive for as long as the icon should exist; deleting a
never-added icon fails harmlessly (`wil::unique_prop_variant` semantics).

## Naming grammar

Every new name must answer these questions; deviations are declared in the
header that owns the name.

| Question | Encoding |
|---|---|
| Knowledge source | `PascalCase` = Win32 vocabulary (contract follows the API's docs); `lowerCamel` = invented abstraction (contract lives in this header) |
| Failure contract | `Get*` throws on failure; `TryGet*` fail-soft, failure returned as data; `*NoThrow` / `*_nothrow` = exceptions banned, failure via return value |
| Ownership | `unique_*` / `shared_*` = RAII; the handle type encodes the deleter |
| Shape | `for_each_*` = callback-driven algorithm; callback returns void (continue), bool (false stops), or HRESULT (S_OK continues) |
| API mirroring | The `W` suffix is kept iff the wrapped API has W/A duality and the function is (a narrowing of) that API (`TrySearchPathW`); narrowings and multi-API composites are descriptive without `W` (`TryGetUserObjectName`, `TryGetWindowText`) |
| Placement | A theme header is earned by machinery or mass; single-pattern helpers go to `win32_helpers.h` |

## Failure regime

`TryGet*` is total fail-soft: empty result == failure (invalid handle,
insufficient rights, OOM), never an exception. Deliberately broader than wil's
TryGet family, which still surfaces unexpected failures via HRESULT or throw;
add an HRESULT nothrow core or `Get*` overloads only when a caller must
distinguish or propagate failures.

## Deliberate deviations from wil

- `std::` instead of wil's internal `wistd::`
- C++23 concepts instead of `always_false` + `static_assert`
- No `W` suffix on invented composite names (reserved for API mirrors with A/W duality)
- Total fail-soft `TryGet*` (wil's still surfaces unexpected failures)
