// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "solar/data/entities.h"
#include "settings/settings_common_session.h"

#include "boxes/premium_limits_box.h"
#include "base/unixtime.h"

class BoxContent;

namespace Window {
class Controller;
class SessionController;
} // namespace Window

namespace Settings {

object_ptr<Ui::GenericBox> RegexEditBox(RegexFilter *filter,
										const Fn<void(RegexFilter)> &onDone,
										std::optional<long long> dialogId = std::nullopt,
										bool showToast = false);
} // namespace Settings