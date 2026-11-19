/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "base/const_string.h"

#define TDESKTOP_REQUESTED_ALPHA_VERSION (0ULL)

#ifdef TDESKTOP_ALLOW_CLOSED_ALPHA
#define TDESKTOP_ALPHA_VERSION TDESKTOP_REQUESTED_ALPHA_VERSION
#else // TDESKTOP_ALLOW_CLOSED_ALPHA
#define TDESKTOP_ALPHA_VERSION (0ULL)
#endif // TDESKTOP_ALLOW_CLOSED_ALPHA

// used in Updater.cpp and Setup.iss for Windows
constexpr auto AppId = "{8D9B4C2A-31E5-4A79-9F60-7C2E1B4D9011}"_cs;
constexpr auto AppNameOld = "SolarGram for Windows"_cs;
#if defined(Q_OS_WIN) && !defined(OS_WIN_STORE)
constexpr auto AppName = "SolarGram Desktop"_cs;
#else
constexpr auto AppName = "SolarGram Desktop"_cs;
#endif
constexpr auto AppFile = "SolarGram"_cs;
constexpr auto AppVersion = 7002009;
constexpr auto AppVersionStr = "7.2.9";
constexpr auto AppBetaVersion = false;
constexpr auto AppAlphaVersion = TDESKTOP_ALPHA_VERSION;
