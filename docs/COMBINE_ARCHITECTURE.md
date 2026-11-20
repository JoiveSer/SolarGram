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
| **3-Mode Anti-Recall (`Off` / `SessionOnly` RAM / `Persistent` DB) + Full Media** | `solar/data/messages_storage.*`, `solar/ui/message_history/history_item.cpp`, `core/application.cpp`, `history/history_item.cpp` | AyuGramDesktop + SolarSpec | Implemented (`v1.0.1`) |
| **Per-Chat Exclusions & Spy Read/Online + OneViewTTL + Backup** | `solar/solar_settings.*`, `api/api_updates.cpp`, `apiwrap.cpp`, `solar/data/solar_database.*`, `solar/ui/context_menu/context_menu.cpp` | AyuGramDesktop | Implemented (`v1.0.1`) |
| **Partisan Security (Duress PIN Decoy/Wipe, `#PIN` Hidden Chats, Panic Wipe)** | `window/window_lock_widgets.cpp`, `dialogs/dialogs_widget.cpp`, `history/history.cpp`, `window/notifications_manager.cpp`, `solar/features/hotkeys/solar_hotkeys.*` | Partisan Telegram + `partisan.rs` | Implemented (`v1.0.1`) |
| **Global Hotkeys (`Ctrl+Shift+F10/F11/F12`) + Title Pin Button & Ghost Chip** | `solar/features/hotkeys/solar_hotkeys.*`, `window/main_window.*` | `hotkey.rs` + Forkgram | Implemented (`v1.0.1`) |
| **Streamer Privacy & Notification Masking** | `info/profile/info_profile_values.cpp`, `settings/sections/settings_main.cpp`, `window/notifications_manager.cpp` | SereinGram + `serein.rs` | Implemented (`v1.0.1`) |
| **Bulk Delete by Period + Round Video Seekbar + Delete-for-All Default** | `solar/ui/context_menu/context_menu.cpp`, `history/view/media/history_view_gif.cpp`, `boxes/delete_messages_box.cpp` | Materialgram + Forkgram + `material.rs` | Implemented (`v1.0.1`) |
| **Local Folders (`Unread` / `Unmuted` / `Admin`) + Text Replacements** | `data/data_chat_filters.*`, `window/window_filters_menu.cpp`, `ui/widgets/chat_filters_tabs_strip.cpp`, `solar/features/security/outgoing_guard.cpp` | Kotatogram + `kotato.rs` | Implemented (`v1.0.1`) |
| **Presence Logger + Remembered Last Seen + Peek Last Seen** | `api/api_updates.cpp`, `data/data_peer_values.cpp`, `solar/data/solar_database.*`, `solar/ui/context_menu/context_menu.cpp` | entinyGram + LuxuryGram + Donutgram | Implemented (`v1.0.1`) |
| **Device Spoofing + EXIF/XMP/IPTC Strip + Forced Copy (`noForwards` Bypass) + Upload Boost + MainFont + WAL** | `mtproto/mtp_instance.cpp`, `mtproto/session_private.cpp`, `storage/localimageloader.cpp`, `data/data_document.cpp`, `media/view/media_view_overlay_widget.cpp`, `storage/file_upload.cpp`, `core/application.cpp`, `solar/data/solar_database.cpp` | entinyGram + Novagram + Nagram + 64Gram + AyuGramDesktop | Implemented (`v1.0.1`) |
| **Per-Chat Ghost (5/5) & Online Blink (~1.5s) + Block External Lookups** | `solar/solar_settings.*`, `solar/solar_worker.cpp`, `data/data_stories.cpp`, `solar/ui/utils/itunes_search.cpp`, `solar/ui/context_menu/context_menu.cpp` | SereinGram + GhostgramPC | Implemented (`v1.0.1`) |
| **Anti-Phishing Barriers (#2–#5) + Regex ReDoS Limits** | `api/api_bot.cpp`, `history/view/history_view_contact_status.cpp`, `inline_bots/bot_attach_web_view.cpp`, `history/history_widget.cpp`, `core/click_handler_types.*`, `core/ui_integration.cpp`, `solar/features/filters/filters_controller.cpp` | SereinGram + Regram + SolarSpec | Implemented (`v1.0.1`) |
| **Extra Translators (`DeepLX`, `TranSmart`, `SolarBrain AI`) + Auto-Fallback + Chat Summary (Last 50)** | `solar/features/translator/implementations/extra.*`, `solar_translator.cpp`, `lang/translate_provider.cpp`, `solar/ui/context_menu/context_menu.cpp` | SereinGram + SolarBrain | Implemented (`v1.0.1`) |
| **Pangu Spacing + Disclaimer Strip + Quoted Reply on Deleted + To Saved (No Author)** | `solar/features/security/outgoing_guard.*`, `history/history_item.cpp`, `history/history_widget.cpp`, `history/history_inner_widget.cpp`, `history/view/history_view_context_menu.cpp` | SereinGram + GhostgramPC | Implemented (`v1.0.1`) |
| **Local Folders (6x) + Unread Chats First + Recent Chats (50) + Watcher Bio/Alerts** | `data/data_chat_filters.*`, `dialogs/dialogs_entry.cpp`, `data/data_session.cpp`, `data/data_user.cpp`, `solar/ui/settings/settings_solar_utils.*`, `window/window_main_menu.cpp`, `solar/features/hotkeys/solar_hotkeys.cpp` | SereinGram + Kotatogram + LuxuryGram | Implemented (`v1.0.1`) |
| **Download Speed Boost + 16-Hex Filenames + Prefer IPv6 + Birthday/Gifts Debloat + Mutual Label** | `storage/download_manager_mtproto.cpp`, `data/data_document.cpp`, `solar/ui/settings/settings_general.cpp`, `info/profile/*`, `data/components/promo_suggestions.cpp` | 64Gram + SereinGram + NFGram | Implemented (`v1.0.1`) |

---

## 3. Phased Combine Roadmap (`v1.1` – `v1.3`)

- **`v1.1` — Extended Privacy, Composer & Sandbox Scripting**: Per-chat Ghost Mode with brief online blink, 5-barrier anti-phishing & `.exe`/archive auto-download block, `Message Stash` / inline calculator / domain rewrites (`fixupx`/`tnktok`) / dot-commands / Channel Feed / Notification Centre, plus `QuickJS-ng` in-process JS runtime with explicit capability manifests.
- **`v1.2` — Host Forensics Hardening, DPI Bypass & Desktop E2EE**: `Novagram` DPAPI `tdata` sealing + `.data` pointer, built-in `WebSocket` (`kws*.web.telegram.org`) + `FakeTLS` + `XRay` core + `Pinned DoH` + `Call Proxy Bridge`, and native Desktop E2EE Secret Chats + `Force E2EE`.
- **`v1.3` — Offline Media Intelligence & Audio/Media Combine**: Optional downloadable models for `whisper.cpp` voice/video-note transcription, `CTranslate2` offline translation, and `Tesseract` image OCR; 5-band EQ + Slowed/Reverb player, pre-encoder Voice Changer, `yt-dlp` / format converter, Desktop Stories, and `Ctrl+Alt+M` MTProto Inspector.
