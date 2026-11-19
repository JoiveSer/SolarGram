// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "settings/settings_common.h"
#include "settings/settings_common_session.h"

namespace Window {
class SessionController;
} // namespace Window

namespace Settings {

class SolarAppearance : public Section<SolarAppearance> {
public:
	SolarAppearance(QWidget *parent, not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();
};

[[nodiscard]] Type SolarAppearanceId();

} // namespace Settings
