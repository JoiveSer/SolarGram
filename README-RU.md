<div align="center">
  <img src=".github/SolarGram.png" alt="SolarGram Logo" width="144" />
  <h1>SolarGram Desktop</h1>
  <p><strong>Защищённый модульный Telegram-комбайн для Windows (x64)</strong></p>
  <p>
    <a href="docs/SOLARGRAM_FEATURES_CATALOG.md">Каталог всех фич (65+ опенсорс-доноров)</a> ·
    <a href="docs/COMBINE_ARCHITECTURE.md">Архитектура комбайна и Roadmap</a> ·
    <a href="README.md">English Version</a>
  </p>
</div>

---

## Реализованные модули (v1.0)

### 1. Приватность, стелс и отсутствие сторонней телеметрии
- **Нулевая внешняя телеметрия**: полностью вырезаны сторонние Sentry minidump-отправки, удалённые бейджи/манифесты разработчиков и скрытые подгрузки переводов со сторонних CDN.
- **Дата регистрации без ботов**: детерминированный оффлайн-расчёт примерной даты создания аккаунта по диапазонам Telegram ID (без отправки чужих ID сторонним ботам).
- **Режим Призрака (по аккаунтам и глобальный)**: отключение отчётов о прочтении, просмотра историй, онлайна и набора текста; отложенная отправка, **Прочитать до этого места** и режим **Вечный онлайн**.
- **Принудительные Relay-звонки (`Force Relay Calls`)**: отключает P2P-маршруты WebRTC, скрывая ваш реальный IP-адрес от собеседника во время звонков 1-на-1.
- **Подтверждение исходящих звонков**: защита от случайного нажатия кнопки вызова.

### 2. Исходящий фильтр и защита аккаунта (`OutgoingGuard`)
- **Автоочистка трекинг-параметров (`utm_*`, `fbclid`, `gclid`, `si`, `ysclid`, `mc_*`)**: удаляет трекинг как в контекстном меню копирования ссылок, так и автоматически при отправке сообщений (сохраняя подписанные URL `x-amz-*`, `sig` и сдвигая разметку/теги).
- **Защита от слива 5-значного кода входа (`777000`)**: отслеживает свежие коды авторизации исключительно в оперативной памяти (TTL 15 минут, никогда не пишется на диск) и блокирует случайную отправку кода в сторонние чаты предупреждающим окном.
- **Анти-Капс**: нормализация случайного CAPS LOCK с сохранением `/команд`, ссылок и кода в `` `...` ``.

### 3. Защита экрана и управление окном
- **Режим стримера + Win32 OBS Sentinel**: исключение всех окон клиента из захвата (`WDA_EXCLUDEFROMCAPTURE`) и автоматическое обнаружение процессов захвата экрана (`obs64.exe`, `streamlabs obs.exe`, `xsplit.core.exe`, `discord.exe`, `sharex.exe`, `bdcam.exe`).
- **Поверх всех окон (`Always on Top`)**: мгновенное закрепление окна (`HWND_TOPMOST`) через боковое меню и настройки.

### 4. Локальная история, интерфейс и медиа-инструменты
- **Анти-удаление и история правок** в локальной SQLite БД (`tdata/solardata.db`), рендер Message Shot и сохранение защищённого медиа/TTL-контента.
- **Пересылка без автора (`Forward Without Author`)**, копирование `callback_data` инлайн-кнопок, **Точные несокращённые счётчики** (`12,487` вместо `12.4K`), скрытие похожих каналов и 4 фирменных стиля иконки SolarGram (`default`, `emblem`, `obsidian`, `nova`).

### 5. Локальный ИИ-помощник `SolarBrain`
- Вызывается строго вручную из контекстного меню сообщения; работает с локальным **Ollama** (`http://127.0.0.1:11434/api/chat`) или настраиваемым OpenAI-совместимым API: **Резюме**, **3 варианта ответа**, **Улучшение тона**, **Объяснение и чеклист задач**.

### 6. Обновление v1.0.1 (полный добор комбайна Вариант 2)
- **3-режимное сохранение удалёнок (`Off` / `До перезахода (RAM)` / `Навсегда (БД)`) + медиа**: захват и восстановление фото, документов, стикеров, ГС, кружков, сообщений без текста и одноразовых (`OneViewTTL`) медиа; в режиме RAM переживает сворачивание в трей («домик») и очищается только при полном закрытии приложения.
- **Исключения и Spy-инструменты**: per-chat исключения сохранения/чтения/набора текста, `Delete (Keep Locally)`, точное время прочтения/прослушивания (`ContextActionWithSubText`), `Peek Last Seen`, журнал присутствия (`Presence History`) + запоминание последнего онлайна (`Remembered Last Seen`), экспорт/импорт бэкапа (`Merge` / `Replace`).
- **Partisan Security и хоткеи**: `Duress PIN` (`Decoy` / `Silent Panic Wipe`), скрытые чаты за `#PIN` в поиске, `Panic Wipe` (`Ctrl+Shift+F12`), глобальные хоткеи `Ctrl+Shift+F10` (`Always on Top` + кнопка-булавка в шапке окна) и `Ctrl+Shift+F11` (`Streamer Mode`), статус `[Ghost]`/`[Streamer]` в заголовке + настраиваемый хоткей, `Alt+F` и `Alt+C`.
- **Дополнения комбайна**: **Локальные папки** (6 фильтров: `Unread`, `Unmuted`, `Admin`, `Groups`, `Channels`, `Bots`), **Per-Chat Ghost Mode (5/5)** + **Online Blink (~1.5с)**, блокировка внешних запросов метаданных (`BlockExternalLookups`), **Анти-фишинг барьеры** (кнопка номера, Mini Apps, фейк-Support, подтверждение внешних ссылок), переводчики **DeepLX / TranSmart / SolarBrain** с авто-fallback на Google и саммари чата (`Summarize Chat`), **Pangu**-пробелы CJK–Latin, скрытие плашек дисклеймеров, **ответ/цитата на удалённое сообщение**, **в Избранное без автора**, **Unread Chats First**, **Recent Chats (50)** (`Alt+R`), лог смены био и онлайн-алерты (`Watch Online`), маскировка номера/`@username`/уведомлений в `Streamer Mode`, **Device Spoofing** (`Model`/`System`/`AppVersion`/`LangCode`), **очистка EXIF/XMP/IPTC**, **Forced Copy & Save**, **массовое удаление за период**, **перемотка в кружках**, **«Удалить для всех» по умолчанию**, **автозамены**, **Upload & Download Speed Boost**, случайные **16-hex** имена файлов, **Prefer IPv6**, скрытие дней рождений и подарков, индикатор **Mutual Contact**, **Main Font** и **SQLite WAL Mode**.

---

## Credits и благодарности (перенесённые open-source модули)
SolarGram объединяет наработки проверенных open-source форков Telegram (подробнее — [docs/COMBINE_ARCHITECTURE.md](docs/COMBINE_ARCHITECTURE.md) и [docs/SOLARGRAM_FEATURES_CATALOG.md](docs/SOLARGRAM_FEATURES_CATALOG.md)):
- **[Telegram Desktop (`tdesktop`)](https://github.com/telegramdesktop/tdesktop)** — официальная база под GNU GPLv3.
- **[AyuGramDesktop](https://github.com/AyuGram/AyuGramDesktop)** — Ghost Mode (`0/5`), Anti-Recall и история правок, Spy read/online, OneView-медиа, `WAL Mode`, `MainFont`, `Upload Speed Boost`, `Regex`-фильтры, `Message Shot`, кастомизация меню и `Streamer Mode` (`SetWindowDisplayAffinity`).
- **[Partisan Telegram](https://github.com/wrwrabbit/tdesktop)** и **[Vartagram](https://github.com/wrwrabbit/Vartagram)** — `Duress PIN` (режим приманки и экстренный вайп) и скрытие чатов за `#PIN`.
- **[SereinGram](https://github.com/eltavine/SereinGram)** — зачистка трекинг-хвостов URL (`OutgoingGuard`), OBS/capture-детектор, маскировка на стриме, переводчики DeepLX и TranSmart, Pangu-пробелы, саммари чата, `Recent Chats` и `To Saved (No Author)`.
- **[GhostgramPC](https://github.com/ichmagmaus111/GhostgramPC)** — тихая отложка (+12с), `Read Until Here`, `Eternal Online`, пересылка без автора, `Anti-Caps` и цитирование/ответ на удалённые сообщения.
- **[Regram](https://github.com/RegramDev/Regram)** — защита ICU Regex-фильтров от ReDoS (лимит времени и стека).
- **[LuxuryGram](https://github.com/GofMan5/LuxuryGram)** — `Force Relay Calls` (без P2P утечки IP), подтверждение звонка, `Repeat` с rich-превью и логирование присутствия.
- **[Kotatogram](https://github.com/kotatogram/kotatogram-desktop)** — локальные папки клиента и автозамены текста (`:shrug:` и др.).
- **[Materialgram](https://github.com/kukuruzka165/materialgram)** — массовое удаление чанками по 100 (`FLOOD_WAIT`), ползунок перемотки кружков и MD3-переключатели.
- **[Forkgram](https://github.com/crazypeace/forkgram-tdesktop)** — авточекбокс «Удалить для всех» и кнопка закрепления окна поверх других.
- **[entinyGram](https://github.com/entaytion/entinyGram)** и **[Donutgram](https://github.com/kittenello/Donutgram)** — журнал онлайна, запоминание скрытого `last seen` и `Device Spoofing` в `initConnection`.
- **[Novagram](https://github.com/confeden/Novagram)** и **[Nagram](https://github.com/NextAlone/Nagram)** — удаление метаданных EXIF/XMP/IPTC при отправке фото и принудительное копирование (`Forced Copy`).
- **[64Gram](https://github.com/TDesktop-x64/tdesktop)** / **[NFGram](https://github.com/frknkrc44/NFGram)** — точные счётчики, ускорение выгрузки и скачивания, переключатель `Prefer IPv6` и деблоат дней рождений/подарков/каналов.
- **[decoder-dev/Telegram-iOS](https://github.com/decoder-dev/Telegram-iOS)** — защита от утечки 5-значных кодов входа `777000`.
- **[Telebrain](https://github.com/fedorn/telebrain)** — концепция локального ИИ-помощника через Ollama (`SolarBrain`).

---

## Структура документации
- [docs/SOLARGRAM_FEATURES_CATALOG.md](docs/SOLARGRAM_FEATURES_CATALOG.md) — полный дедуплицированный каталог функций по итогам аудита официальных репозиториев (`tdesktop`, `telegram-ios`, `Telegram-Android`), форков `>= 3★` и автономных клиентов.
- [docs/COMBINE_ARCHITECTURE.md](docs/COMBINE_ARCHITECTURE.md) — архитектура `Telegram/SourceFiles/solar/`, карта реализованных модулей и план следующих релизов (`v1.1–v1.3`).

---

## Облачная сборка в GitHub Actions (без 60 ГБ кэша на локальном диске)
1. Запушьте репозиторий в `https://github.com/JoiveSer/SolarGram`.
2. Добавьте в **Settings -> Secrets and variables -> Actions** ключи `TDESKTOP_API_ID` и `TDESKTOP_API_HASH`.
3. Запустите workflow **SolarGram Windows (x64)** во вкладке **Actions** — на раннере соберётся готовый архив `SolarGram-Windows-x64-Portable.zip`.
