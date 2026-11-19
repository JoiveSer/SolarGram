// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.

#include "solar/features/streamer_mode/platform/mac/streamer_mode_mac.h"

#include <QtWidgets/QWidget>

#include <Cocoa/Cocoa.h>

namespace SolarFeatures::StreamerMode::Platform {

void SetWindowCaptureExcluded(
		not_null<QWidget*> widget,
		bool excluded) {
	const auto view = reinterpret_cast<NSView*>(widget->winId());
	view.window.sharingType = excluded
		? NSWindowSharingNone
		: NSWindowSharingReadOnly;
}

bool IsCaptureSoftwareRunning() {
	return false;
}

} // namespace SolarFeatures::StreamerMode::Platform
