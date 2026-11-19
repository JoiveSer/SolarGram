// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#define ICON(name, value) const auto name##_ICON = QStringLiteral(value)

namespace SolarAssets {

ICON(DEFAULT, "default");
ICON(EMBLEM, "emblem");
ICON(OBSIDIAN, "obsidian");
ICON(NOVA, "nova");

void loadAppIco();
QString appIcoPath();

QImage loadPreview(const QString& name);

QString currentAppLogoName();
QImage currentAppLogo();
QImage currentAppLogoPad();

}
