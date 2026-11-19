// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.

#include "solar/features/streamer_mode/streamer_mode.h"

#include "solar/solar_settings.h"
#include "solar/features/streamer_mode/platform/platform_streamer_mode.h"
#include "core/application.h"
#include "window/window_controller.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

namespace SolarFeatures::StreamerMode {

namespace {

constexpr auto kHiddenProperty = "SolarStreamerModeHidden";
constexpr auto kScanIntervalMs = 3500;

bool AutoDetected = false;
QTimer *ScanTimer = nullptr;

[[nodiscard]] bool IsWindowCaptureExcluded(not_null<QWidget*> widget) {
	return widget->property(kHiddenProperty).toBool();
}

void SetWindowCaptureExcluded(QWidget *widget, bool excluded) {
	if (!widget) {
		return;
	}
	const auto window = widget->window();
	Platform::SetWindowCaptureExcluded(window, excluded);
	window->setProperty(kHiddenProperty, excluded);
}

[[nodiscard]] bool EffectiveEnabled(bool requested) {
	const auto &settings = SolarSettings::getInstance();
	return requested || (settings.autoStreamerMode() && AutoDetected);
}

void PollCaptureSoftware() {
	const auto &settings = SolarSettings::getInstance();
	if (!settings.autoStreamerMode()) {
		if (AutoDetected) {
			AutoDetected = false;
			apply(settings.streamerMode());
		}
		return;
	}
	const auto detectedNow = Platform::IsCaptureSoftwareRunning();
	if (detectedNow != AutoDetected) {
		AutoDetected = detectedNow;
		apply(settings.streamerMode());
	}
}

} // namespace

void initAutoDetector() {
	if (ScanTimer) {
		return;
	}
	ScanTimer = new QTimer(QCoreApplication::instance());
	QObject::connect(ScanTimer, &QTimer::timeout, [] {
		PollCaptureSoftware();
	});
	ScanTimer->start(kScanIntervalMs);
	PollCaptureSoftware();
}

bool isAutoDetected() {
	return AutoDetected;
}

void apply(bool enabled) {
	const auto effective = EffectiveEnabled(enabled);
	Core::App().enumerateWindows([=](not_null<Window::Controller*> window) {
		SetWindowCaptureExcluded(window->widget(), effective);
	});
	for (const auto widget : QApplication::topLevelWidgets()) {
		if (!widget->windowHandle()) {
			continue;
		}
		if (effective) {
			if (widget->isVisible()
				&& !IsWindowCaptureExcluded(widget)) {
				SetWindowCaptureExcluded(widget, true);
			}
		} else if (IsWindowCaptureExcluded(widget)) {
			SetWindowCaptureExcluded(widget, false);
		}
	}
}

void hideWidgetWindow(QWidget *widget) {
	SetWindowCaptureExcluded(widget, true);
}

void showWidgetWindow(QWidget *widget) {
	SetWindowCaptureExcluded(widget, EffectiveEnabled(SolarSettings::getInstance().streamerMode()));
}

} // namespace SolarFeatures::StreamerMode
