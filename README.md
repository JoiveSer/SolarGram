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
