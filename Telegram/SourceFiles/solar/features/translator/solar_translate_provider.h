// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "solar/solar_settings.h"
#include "translate_provider.h"

#include <memory>

namespace Main {
class Session;
} // namespace Main

namespace Ui {

[[nodiscard]] std::unique_ptr<TranslateProvider> CreateSolarTranslateProvider(
	not_null<Main::Session*> session,
	TranslationProvider provider);

} // namespace Ui
