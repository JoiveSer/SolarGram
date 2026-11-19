# SolarGram Combine Architecture & Implementation Map

> Engineering blueprint mapping how **SolarGram Desktop** combines verified open-source features inside `Telegram/SourceFiles/solar/` without telemetry or redundant abstractions.

---

## 1. Repository Layout & Submodule Bridge

- **`Telegram/SourceFiles/solar/`** — self-contained SolarGram feature namespace (settings, SQLite persistence, filters, translator, Ghost Mode, OutgoingGuard, SolarBrain AI, OBS Sentinel).
- **`Telegram/Resources/art/solar/` & `Telegram/Resources/qrc/solar/solar.qrc`** — SolarGram logos, tray/application icons, SVG badge masks, and 4 built-in icon themes (`default`, `emblem`, `obsidian`, `nova`).
- **`Telegram/build/solar_patch_submodules.py`** — idempotent pre-configure bridge invoked automatically by root `CMakeLists.txt` and `Telegram/configure.py` to align `lib_ui`, `lib_tl`, `lib_icu`, and `codegen` with `SolarUiSettings` / `solargram::lib_icu` / `is_solarNoforwards()`.

---

## 2. Module Implementation Status Matrix

| Subsystem | Module / Hook Files | Donor / Origin | Status |
|---|---|---|---|
| **Ghost Mode & Read Until Here** | `solar/solar_settings.*`, `api/api_updates.cpp`, `solar/ui/context_menu/context_menu.cpp` | Base + GhostgramPC | Implemented (`v1.0`) |
| **Eternal Online** | `api/api_updates.cpp`, `solar/ui/settings/settings_solar.cpp` | GhostgramPC / LuxuryGram | Implemented (`v1.0`) |
| **Force Relay Calls (No P2P IP Leak)** | `calls/calls_call.cpp` (`f_udp_p2p` + `enableP2P` guard) | LuxuryGram / tgcalls | Implemented (`v1.0`) |
| **Outgoing Call Confirmation** | `calls/calls_instance.cpp` | LuxuryGram / SereinGram | Implemented (`v1.0`) |
| **Auto-Strip Tracking URLs on Send & Copy** | `solar/features/security/outgoing_guard.{h,cpp}`, `history/history_widget.cpp`, `history/view/history_view_context_menu.cpp` | SereinGram (`b0966a1`) + `serein.rs` | Implemented (`v1.0`) |
| **5-Digit Login Code Leak Guard (777000)** | `solar/features/security/outgoing_guard.{h,cpp}`, `history/history.cpp`, `history/history_widget.cpp` | decoder-dev / Privacy-Security | Implemented (`v1.0`) |
| **Anti-Caps Mode** | `solar/features/security/outgoing_guard.cpp`, `history/history_widget.cpp` | GhostgramPC / Serein | Implemented (`v1.0`) |
| **Win32 OBS / Capture Sentinel** | `solar/features/streamer_mode/platform/win/streamer_mode_win.cpp`, `streamer_mode.cpp` | `serein.rs` -> native WinAPI | Implemented (`v1.0`) |
| **Always on Top Window Lock** | `window/window_controller.{h,cpp}`, `window/window_main_menu.cpp` | Native Win32 `HWND_TOPMOST` / Qt flag | Implemented (`v1.0`) |
| **Exact Unrounded Counters** | `lang/lang_tag.cpp` (`FormatCountToShort`) | 64Gram / NFGram | Implemented (`v1.0`) |
| **Forward Without Author + Clean Copy** | `history/view/history_view_context_menu.cpp` | GhostgramPC | Implemented (`v1.0`) |
| **Offline Account Creation Date** | `solar/utils/telegram_helpers.cpp` | ID-anchor interpolation (zero bot leak) | Implemented (`v1.0`) |
| **SolarBrain Local AI Copilot (Ollama)** | `solar/features/ai/solar_brain.{h,cpp}` | `telebrain.rs` -> Qt QNetworkAccessManager | Implemented (`v1.0`) |
| **Telemetry & External Endpoint Purge** | `rc_manager.*`, `solar_lang.cpp`, `crash_report_window.cpp`, `localstorage.cpp`, `donate_info_box.cpp`, `settings_other.cpp` | SolarGram Security Audit | Implemented (`v1.0`) |
| **Similar Channels Hide Guard** | `dialogs/ui/dialogs_suggestions.cpp` | Fixed regression | Implemented (`v1.0`) |

---

## 3. Phased Combine Roadmap (`v1.1` – `v1.3`)

- **`v1.1` — Sandbox Scripting Engine**: Port `QuickJS-ng` in-process JS runtime with explicit capability permission manifests and zero unrestricted shell bridges.
- **`v1.2` — Network & Secret Security**: `Novagram` DPAPI local session hardening + configurable local TLS/relay bypass & proxy chains; optional duress passcode vault (`partisan.rs` hidden peers + duress wipe trigger).
- **`v1.3` — Offline Media Intelligence**: Optional downloadable models for `whisper.cpp` voice/video-note transcription, `CTranslate2` offline translation, and `Tesseract` image OCR.
