// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "settings/settings_common.h"
#include "settings/settings_common_session.h"

namespace Window {
class Controller;
class SessionController;
} // namespace Window

namespace Settings {

class SolarMain : public Section<SolarMain> {
public:
	SolarMain(QWidget *parent, not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();
};

[[nodiscard]] Type SolarMainId();

} // namespace Settings
