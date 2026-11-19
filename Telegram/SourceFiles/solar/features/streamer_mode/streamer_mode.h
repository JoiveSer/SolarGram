// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

class QWidget;

namespace SolarFeatures::StreamerMode {

void initAutoDetector();
[[nodiscard]] bool isAutoDetected();

void apply(bool enabled);
void hideWidgetWindow(QWidget *widget);
void showWidgetWindow(QWidget *widget);

} // namespace SolarFeatures::StreamerMode
