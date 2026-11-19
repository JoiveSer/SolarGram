// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "base/basic_types.h"

#include <QString>
#include <functional>

namespace Window {
class SessionController;
} // namespace Window

namespace SolarFeatures::AI {

enum class PromptPreset {
	Summarize,
	ReplyDrafts,
	Proofread,
	Explain,
};

void ShowSolarBrainBox(
	not_null<Window::SessionController*> controller,
	const QString &sourceText,
	Fn<void(QString)> onInsert = nullptr);

void ShowSolarBrainConfigBox(
	not_null<Window::SessionController*> controller);

} // namespace SolarFeatures::AI
