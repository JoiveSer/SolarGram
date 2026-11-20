// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/solar_infra.h"

#include "solar/solar_lang.h"
#include "solar/solar_settings.h"
#include "solar/solar_ui_settings.h"
#include "solar/solar_worker.h"
#include "solar/data/solar_database.h"
#include "solar/features/streamer_mode/streamer_mode.h"
#include "solar/ui/solar_logo.h"
#include "features/translator/solar_translator.h"
#include "lang/lang_instance.h"
#include "ui/chat/chat_style_radius.h"
#include "utils/rc_manager.h"

#ifdef Q_OS_WIN
#include "solar/utils/windows_utils.h"
#endif

namespace SolarInfra {

void initLang() {
	QString id = Lang::GetInstance().id();
	QString baseId = Lang::GetInstance().baseId();
	if (id.isEmpty()) {
		LOG(("Language is not loaded"));
		return;
	}
	SolarLanguage::init();
	SolarLanguage::currentInstance()->fetchLanguage(id, baseId);
}

void initUiSettings() {
	const auto &settings = SolarSettings::getInstance();

	SolarUiSettings::setMonoFont(settings.monoFont());
	SolarUiSettings::setWideMultiplier(settings.wideMultiplier());
	SolarUiSettings::setMaterialSwitches(settings.materialSwitches());
	SolarUiSettings::setAvatarCorners(settings.avatarCorners());
	Ui::SetAppliedBubbleRadius(settings.messageBubbleRadius());
}

void initDatabase() {
	SolarDatabase::initialize();
	SolarDatabase::setWalMode(SolarSettings::getInstance().walMode());
}

void initWorker() {
	SolarWorker::initialize();
}

void initRCManager() {
	RCManager::getInstance().start();
}

void initTranslator() {
	Solar::Translator::TranslateManager::init();
}

void initIcon() {
#ifdef Q_OS_WIN
	SolarAssets::loadAppIco();
	reloadAppIconFromTaskBar();
#endif
}

void init() {
	initLang();
	initDatabase();
	initUiSettings();
	initIcon();
	initWorker();
	initRCManager();
	initTranslator();
	SolarFeatures::StreamerMode::initAutoDetector();
}

}
