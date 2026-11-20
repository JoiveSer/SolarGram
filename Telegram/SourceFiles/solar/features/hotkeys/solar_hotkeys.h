// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

namespace Window {
class Controller;
} // namespace Window

namespace SolarHotkeys {

void TriggerPanicWipe();
void AttachWindowShortcuts(not_null<Window::Controller*> controller);

} // namespace SolarHotkeys
