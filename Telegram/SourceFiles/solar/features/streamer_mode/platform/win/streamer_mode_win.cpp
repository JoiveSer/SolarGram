// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.

#include "solar/features/streamer_mode/platform/win/streamer_mode_win.h"

#include <QtWidgets/QWidget>

#include <windows.h>
#include <tlhelp32.h>

#include <array>

namespace SolarFeatures::StreamerMode::Platform {

void SetWindowCaptureExcluded(
		not_null<QWidget*> widget,
		bool excluded) {
	const auto handle = reinterpret_cast<HWND>(widget->winId());
	SetWindowDisplayAffinity(
		handle,
		excluded ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
}

bool IsCaptureSoftwareRunning() {
	static constexpr std::array<const wchar_t*, 9> kCaptureProcesses = {{
		L"obs64.exe",
		L"obs32.exe",
		L"streamlabs obs.exe",
		L"xsplit.core.exe",
		L"bdcam.exe",
		L"action_x64.exe",
		L"sharex.exe",
		L"discord.exe",
		L"twitchstudio.exe",
	}};

	const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) {
		return false;
	}

	PROCESSENTRY32W entry{};
	entry.dwSize = sizeof(entry);
	auto found = false;
	if (Process32FirstW(snapshot, &entry)) {
		do {
			for (const auto *target : kCaptureProcesses) {
				if (_wcsicmp(entry.szExeFile, target) == 0) {
					found = true;
					break;
				}
			}
			if (found) {
				break;
			}
		} while (Process32NextW(snapshot, &entry));
	}
	CloseHandle(snapshot);
	return found;
}

} // namespace SolarFeatures::StreamerMode::Platform
