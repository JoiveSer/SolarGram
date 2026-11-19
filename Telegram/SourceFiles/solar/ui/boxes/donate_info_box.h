// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

namespace Window {
class SessionController;
}

namespace Ui {

class GenericBox;

void FillDonateInfoBox(not_null<Ui::GenericBox*> box, not_null<Window::SessionController*> controller);

} // namespace Ui
