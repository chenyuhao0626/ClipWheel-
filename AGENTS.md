# AGENTS.md — ClipWheel

## Build

```bat
build.bat
```

Requires Visual Studio 2022 Build Tools with C++ toolchain + Windows SDK.
`build.bat` auto-locates MSVC via `vswhere`, then runs `cl /O2 /W3 /utf-8`.

The resource compiler step tries `rc` first, falls back to `llvm-rc`.
Outputs `clipwheel.exe`, `history.obj`, `main.obj`, `clipwheel.res`.

## Test

```bat
clipwheel.exe --selftest
```

Runs the `cw_history_selftest()` routine (pin CRUD, save/load, reordering).
Accepts `--selftest-noui` for headless runs.
Smoke test: `scripts\smoke-test.bat`.

There is no lint or typecheck. CI via GitHub Actions (`.github/workflows/ci.yml`) runs `build.bat` + `--selftest-noui` + smoke-test on push/PR.

## Run

`clipwheel.exe` must be in the same directory as `clipwheel.ini` (path resolved via
`GetModuleFileNameW`). A compiled `clipwheel.exe` is checked into root.

## Architecture

- **Overlay window** (`ClipWheelOverlay`): hidden topmost popup, covers the virtual screen.
  Shown on hotkey to render the radial wheel. Uses layered window alpha (~95% opaque).
- **Main window** (`ClipWheelMain`): the control center, ~1180×760 min size.
- **Child windows**: two `ClipWheelCardList` controls (pin list, history list), and one
  `ClipWheelPreview` (visual wheel with drag-drop).
- **Keyboard hook**: `WH_KEYBOARD_LL` global hook — the entire app depends on this.
  Without it, hotkey detection and wheel hide-on-release fail.

### Sector slot mapping

Wheel has 8 sectors (`NSECT=8`). Sector 4 is always the cancel zone.
`sector_to_slot(i)` maps `0-3→0-3`, `5-7→4-6`. Unused sectors return -1.

### Data layer

| Constant | Value | Meaning |
|---|---|---|
| `CW_MAX_SLOT` | 7 | Wheel display slots |
| `CW_MAX_PIN` | 7 | Max pinned items |
| `CW_MAX_HIST` | 32 | Max history entries |
| `CW_MAX_CHARS` | 4096 | Max text length per entry |

Data is persisted to `%APPDATA%\ClipWheel\clips.bin` (binary, magic `0x43505748`,
version 1, pins first then history). Log to `%APPDATA%\ClipWheel\clipwheel.log`.

### Single instance

`Local\ClipWheelSingleInstance` named mutex. Second launch shows a message box.

## Configuration

`clipwheel.ini` — INI format:

| Section | Key | Type | Default | Notes |
|---|---|---|---|---|
| `[Hotkey]` | `VK` | decimal int | 192 | Virtual key code (192=~) |
| `[Hotkey]` | `Mod` | int bitmask | 0 | 1=Alt, 2=Ctrl, 4=Shift, additive |
| `[Behavior]` | `AutoPaste` | 0/1 | 1 | Auto Ctrl+V after selection |
| `[Behavior]` | `FloatingMode` | 0/1 | 0 | Wheel follows cursor, not fullscreen |

## Version

Defined as `CLIPWHEEL_VERSION L"2.1.0"` in `main.c`. Must be updated by hand when bumping.

## Conventions / gotchas

- Pure C + Win32 API, no external libraries. All drawing is GDI (`GradientFill`,
  `RoundRect`, `Pie`).
- `clipwheel.res`, `main.obj`, `history.obj` are **build artifacts** checked into VCS.
  Do not edit them — rebuild instead.
- The VK value in `.ini` is decimal, not hex. `VK=192` means `VK_OEM_3`.
- Hotkey recording captures raw keystrokes; ESC cancels, any non-modifier key commits.
- DPI awareness is `PerMonitorAwareV2` with fallback to `SetProcessDPIAware`.
- The `g_ignore_clip` flag must be set before programmatic clipboard writes to avoid
  re-recording your own copies.
- `CW_MAX_CHARS` is hard at 4096 — longer texts are truncated on copy handling.
- The `--pin=` and `--pin-many=` CLI flags trigger `cw_history_init()`, write, then exit
  immediately — they can be used from scripts.
- `msimg32.lib` (for `GradientFill`) is linked via `#pragma comment(lib)` in `app.h`,
  not listed in `build.bat`'s `cl` command. Other libs are listed in both places.
- `.gitignore` ignores `*.exe`, `*.obj`, `*.res`, but `clipwheel.exe`, `*.obj`, and
  `clipwheel.res` are intentionally tracked in VCS (so the repo is self-contained).
