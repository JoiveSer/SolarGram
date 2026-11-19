// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.

#include "solar/features/streamer_mode/platform/linux/streamer_mode_linux.h"

namespace SolarFeatures::StreamerMode::Platform {

void SetWindowCaptureExcluded(not_null<QWidget*>, bool) {
}

bool IsCaptureSoftwareRunning() {
	return false;
}

} // namespace SolarFeatures::StreamerMode::Platform
