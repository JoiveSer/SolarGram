<div align="center">
  <img src=".github/SolarGram.png" alt="SolarGram Logo" width="144" />
  <h1>SolarGram Desktop</h1>
  <p><strong>Privacy-hardened, modular Telegram Desktop combine for Windows (x64)</strong></p>
  <p>
    <a href="docs/SOLARGRAM_FEATURES_CATALOG.md">Features Catalog (65+ Open-Source Donors)</a> ·
    <a href="docs/COMBINE_ARCHITECTURE.md">Combine Architecture & Roadmap</a> ·
    <a href="README-RU.md">Русская версия</a>
  </p>
</div>

---

## Core Capabilities (Implemented in v1.0)

### 1. Privacy, Stealth & Telemetry Hardening
- **Zero Third-Party Telemetry**: Removed crash-dump uploaders (`Sentry`), remote developer/badge manifests, and external CDN language auto-fetchers.
- **Account Age Without External Bots**: Deterministic offline Telegram registration-date estimation directly from User ID ranges (no querying third-party inline bots).
- **Ghost Mode (Per-Account & Global)**: Fine-grained suppression of read receipts (`SendReadMessages`), story views (`SendReadStories`), online status (`SendOnlinePackets`), typing indicators (`SendUploadProgress`), scheduled ghost sending, and quick **Read Until Here** actions.
- **Eternal Online Toggle**: Optional continuous online presence flag in `Api::Updates`.
- **Force Relay Calls (IP Privacy)**: Strips `f_udp_p2p` flags and disables direct P2P WebRTC routes in 1-on-1 calls so your IP address never leaks to peers.
- **Call Confirmation Guard**: Forces pre-call confirmation before initiating direct outgoing calls.

### 2. Outgoing Guard & Anti-Leak Pipeline
- **Auto-Strip Tracking URLs on Send & Copy**: Removes `utm_*`, `fbclid`, `gclid`, `dclid`, `msclkid`, `twclid`, `yclid`, `ysclid`, `mc_cid`, `mc_eid`, `igshid`, `ref_src`, `si`, and `mkt_tok` from HTTP/HTTPS URLs while preserving signed URLs (`signature`, `sig`, `x-amz-*`, `x-goog-*`) and text formatting offsets.
- **Login Code Leak Guard (`777000`)**: Tracks 5-digit authentication codes received from Telegram service notifications strictly in RAM (15-minute TTL, never persisted to disk) and blocks accidental forwarding to third-party chats behind an explicit warning dialog.
- **Anti-Caps Mode**: Normalizes accidental ALL-CAPS outgoing messages while preserving `/commands`, URLs, and inline `` `code` `` spans.
- **Zalgo Filter & Send Confirmations**: Optional confirmations for stickers, GIFs, voice messages, and video rounds.

### 3. Screen Capture & Window Control
- **Streamer Mode & Win32 OBS Sentinel**: Applies `WDA_EXCLUDEFROMCAPTURE` across application windows and monitors common screen-capture processes (`obs64.exe`, `streamlabs obs.exe`, `xsplit.core.exe`, `discord.exe`, `sharex.exe`, `bdcam.exe`) to auto-protect windows while active.
- **Always on Top**: Instant `HWND_TOPMOST` / window-stay-on-top toggle accessible from Settings and the main drawer menu.

### 4. Local History, Media & UI Ergonomics
- **Anti-Recall & Edit History**: Local SQLite persistence (`tdata/solardata.db`) preserving deleted messages and edit revisions.
- **Clean Forwarding & Context Power Tools**: One-click **Forward Without Author**, unrestricted media forwarding/saving, Message Shot rendering, **Copy Callback Data** on bot inline buttons, **Exact Unrounded Counters** (`12,487` instead of `12.4K`), and **Hide Similar Channels**.
- **SolarGram Icon Variants**: Custom Solar vector & emblem themes (`default`, `emblem`, `obsidian`, `nova`).

### 5. SolarBrain Local AI Copilot
- Explicit user-triggered AI assistant panel in message context menus supporting local **Ollama** (`http://127.0.0.1:11434/api/chat`) and OpenAI-compatible endpoints: **Summarize**, **3 Reply Drafts**, **Proofread & Soften**, and **Explain & Extract Action Items**.

### 6. v1.0.1 Combine Upgrades (Anti-Recall, Partisan, Privacy & QoL)
- **3-Mode Anti-Recall (`Off` / `Until Restart (RAM)` / `Always (Database)`) + Full Media**: Captures and reconstructs photos, documents, stickers, voice notes, round videos, standalone media, and One-Time (`OneViewTTL`) media; RAM vault survives minimizing to tray and clears strictly on full app exit.
- **Per-Chat Exclusions & Spy History**: Per-dialog Saving/Read/Typing overrides, `Delete (Keep Locally)`, exact Read & Listened timestamps (`ContextActionWithSubText`), `Peek Last Seen`, `Presence History` + `Remembered Last Seen`, and JSON `Export / Import Backup` (`Merge` / `Replace`).
- **Partisan Security & Hotkeys**: Salted SHA-256 `Duress PIN` (`DecoyEmptyView` / `SilentPanicWipe`), hidden chats behind `#PIN` search, `Panic Wipe` (`Ctrl+Shift+F12`), global `Ctrl+Shift+F10` (Always on Top + header pin button), `Ctrl+Shift+F11` (Streamer Mode), `DisplayGhostStatus` + configurable Ghost hotkey, `Alt+F` / `Alt+C`.
- **Combine Additions**: Client-side **Local Folders** (`Unread`, `Unmuted`, `Admin`, `Groups`, `Channels`, `Bots`), **Per-Chat Ghost Mode (5/5)** + **Online Blink (~1.5s)**, **Block External Lookups** (blocks iTunes cover leaks), **Anti-Phishing Guards** (unverified phone/Mini-App/fake-Support/external link confirmation), **DeepLX / TranSmart / SolarBrain** translators with Google auto-fallback, **Summarize Chat (Last 50)**, **Pangu CJK-Latin Spacing**, statutory disclaimer stripper, **Quoted Reply on Deleted Messages**, **To Saved (No Author)**, **Unread Chats First**, **Recent Chats (50)** (`Alt+R`), **Watcher Bio Log & Online Alerts**, Streamer phone/`@username`/notification masking, **Device Spoofing** (Model/OS/AppVersion/LangCode), **Strip EXIF/XMP/IPTC**, **Forced Copy & Save**, **Bulk Delete by Period**, **Seekbar on Round Videos**, **Delete for All by Default**, **Text Replacements**, **Upload & Download Speed Boost**, **Random 16-Hex Download Names**, **Prefer IPv6**, **Hide Birthdays/Gifts UI**, **Mutual Contact** label, **Main Font**, and **SQLite WAL Mode**.

---

## Credits & Open-Source Donor Attribution
SolarGram stands on the shoulders of the open-source Telegram ecosystem (see [docs/COMBINE_ARCHITECTURE.md](docs/COMBINE_ARCHITECTURE.md) and [docs/SOLARGRAM_FEATURES_CATALOG.md](docs/SOLARGRAM_FEATURES_CATALOG.md)):
- **[Telegram Desktop (`tdesktop`)](https://github.com/telegramdesktop/tdesktop)** — Official GPLv3 desktop client foundation.
- **[AyuGramDesktop](https://github.com/AyuGram/AyuGramDesktop)** — Ghost Mode engine, Anti-Recall & Edit History, Spy read/online timestamps, OneView media capture, WAL/Font/Upload Boost ergonomics, Streamer window flag, Regex filters, and customization settings.
- **[Partisan Telegram](https://github.com/wrwrabbit/tdesktop)** & **[Vartagram](https://github.com/wrwrabbit/Vartagram)** — Duress PIN (`Decoy` / `Panic Wipe`) and `#PIN`-hidden chats architecture.
- **[SereinGram](https://github.com/eltavine/SereinGram)** — Tracking URL parameter stripper, OBS/capture process Sentinel, Streamer masking, DeepLX & TranSmart translators, Pangu spacing, Chat Summary, Recent Chats (50), and To Saved (No Author).
- **[GhostgramPC](https://github.com/ichmagmaus111/GhostgramPC)** — Scheduled ghost send, Read Until Here, Eternal Online, Forward Without Author, Anti-Caps, and Quoted Reply on Deleted Messages.
- **[Regram](https://github.com/RegramDev/Regram)** — ICU regex ReDoS time and stack limits.
- **[LuxuryGram](https://github.com/GofMan5/LuxuryGram)** — Force Relay Calls (`f_udp_p2p`), Outgoing Call Confirmation, rich-preview Repeat, and presence logging patterns.
- **[Kotatogram](https://github.com/kotatogram/kotatogram-desktop)** — Local client-side folders (`Unread` / `Unmuted` / `Admin`) and text shorthand replacements (`:shrug:`).
- **[Materialgram](https://github.com/kukuruzka165/materialgram)** — Period/batch 100-message deletion (`FLOOD_WAIT` aware), round video seekbar, and MD3 switch styling.
- **[Forkgram](https://github.com/crazypeace/forkgram-tdesktop)** — `Delete for Everyone` default toggle and title-bar Always-on-Top pin control.
- **[entinyGram](https://github.com/entaytion/entinyGram)** & **[Donutgram](https://github.com/kittenello/Donutgram)** — Presence logger, remembered last-seen caching, and `initConnection` device spoofing.
- **[Novagram](https://github.com/confeden/Novagram)** & **[Nagram](https://github.com/NextAlone/Nagram)** — Outgoing image EXIF/XMP/IPTC stripping and Forced Copy/Save in restricted chats.
- **[64Gram](https://github.com/TDesktop-x64/tdesktop)** / **[NFGram](https://github.com/frknkrc44/NFGram)** — Exact unrounded counters, Upload & Download Speed Boost, Prefer IPv6 option, and Birthday/Gifts/Channel debloat toggles.
- **[decoder-dev/Telegram-iOS](https://github.com/decoder-dev/Telegram-iOS)** — 5-digit `777000` login code leak guard.
- **[Telebrain](https://github.com/fedorn/telebrain)** — Local Ollama / OpenAI-compatible AI copilot concept (`SolarBrain`).

---

## Documentation Structure
- [docs/SOLARGRAM_FEATURES_CATALOG.md](docs/SOLARGRAM_FEATURES_CATALOG.md) — Exhaustive deduplicated catalog built from `tdesktop`, `telegram-ios`, `Telegram-Android`, their `>= 3★` forks, and standalone clients.
- [docs/COMBINE_ARCHITECTURE.md](docs/COMBINE_ARCHITECTURE.md) — Module layout (`Telegram/SourceFiles/solar/`), donor provenance, security invariants, and phased roadmap (`v1.1` QuickJS-ng plugin engine, `v1.2` DPI/TLS modules, `v1.3` offline Whisper/OCR).

---

## Building via GitHub Actions (Recommended, Zero Local 60 GB Cache)
1. Fork or push to `JoiveSer/SolarGram`.
2. Add repository secrets in **Settings -> Secrets and variables -> Actions**:
   - `TDESKTOP_API_ID`
   - `TDESKTOP_API_HASH`
3. Trigger **SolarGram Windows (x64)** in the **Actions** tab (`workflow_dispatch`) to compile with MSVC `14.44`, Visual Studio MSBuild, Qt `6.11.2`, stripped PDB debug symbols, and split dependency caching, producing `SolarGram-Windows-x64-Portable.zip`.

---

## License
Built on [Telegram Desktop](https://github.com/telegramdesktop/tdesktop) under the terms of **GNU GPLv3**. See [LICENSE](LICENSE).
