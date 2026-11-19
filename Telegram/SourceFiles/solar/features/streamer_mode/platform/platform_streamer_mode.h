// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "base/basic_types.h"

class QWidget;

namespace SolarFeatures::StreamerMode::Platform {

void SetWindowCaptureExcluded(
	not_null<QWidget*> widget,
	bool excluded);

[[nodiscard]] bool IsCaptureSoftwareRunning();

} // namespace SolarFeatures::StreamerMode::Platform
