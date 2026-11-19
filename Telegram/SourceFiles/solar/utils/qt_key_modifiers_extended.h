// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "base/qt/qt_key_modifiers.h"

namespace base {

[[nodiscard]] inline bool IsExtendedContextMenuModifierPressed() {
	return IsShiftPressed() || IsCtrlPressed();
}

} // namespace base
