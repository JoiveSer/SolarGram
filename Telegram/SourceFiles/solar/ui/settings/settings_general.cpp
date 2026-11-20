// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/ui/settings/settings_general.h"

#include "lang_auto.h"
#include "solar/solar_settings.h"
#include "solar/features/ai/solar_brain.h"
#include "solar/ui/settings/solar_builder.h"
#include "solar/ui/settings/settings_solar_utils.h"
#include "solar/ui/settings/settings_main.h"
#include "base/options.h"
#include "base/platform/base_platform_info.h"
#include "core/application.h"
#include "lang/lang_text_entity.h"
#include "mtproto/session_private.h"
#include "platform/platform_translate_provider.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/boxes/single_choice_box.h"
#include "ui/toast/toast.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_controller.h"
#include "window/window_session_controller.h"

namespace Settings {

using namespace Builder;
using namespace SolarBuilder;

namespace {

void BuildTranslator(SectionBuilder &builder, SolarSectionBuilder &solar) {
	builder.addSubsectionTitle(tr::lng_translate_settings_subtitle());

	auto *settings = &SolarSettings::getInstance();

	const auto options = std::vector{
		std::pair(TranslationProvider::Telegram, QString("Telegram")),
		std::pair(TranslationProvider::Google, QString("Google")),
		std::pair(TranslationProvider::Yandex, QString("Yandex")),
		std::pair(TranslationProvider::DeepLX, QString("DeepLX")),
		std::pair(TranslationProvider::TranSmart, QString("TranSmart")),
		std::pair(TranslationProvider::SolarBrainLLM, QString("SolarBrain AI")),
	};
	const auto nativeAvailable = Platform::IsTranslateProviderAvailable();
	auto availableOptions = options;
	if (nativeAvailable) {
		availableOptions.push_back(std::pair(
			TranslationProvider::Native,
			[] {
				if constexpr (Platform::IsMac()) {
					return QString("macOS");
				} else if constexpr (Platform::IsWindows()) {
					return QString("Windows");
				} else {
					return QString("Linux");
				}
			}()));
	}
	auto optionLabels = std::vector<QString>();
	optionLabels.reserve(availableOptions.size());
	for (const auto &option : availableOptions) {
		optionLabels.push_back(option.second);
	}

	const auto getIndex = [=](TranslationProvider val) {
		const auto i = ranges::find(
			availableOptions,
			val,
			&std::pair<TranslationProvider, QString>::first);
		return (i != end(availableOptions))
			? int(i - begin(availableOptions))
			: 0;
	};

	auto currentVal = SolarSettings::getInstance().translationProviderValue()
		| rpl::map(getIndex)
		| rpl::map([=](int val) { return availableOptions[val].second; });

	const auto button = builder.addButton({
		.id = u"solar/translationProvider"_q,
		.title = tr::solar_TranslationProvider(),
		.st = &st::settingsButtonNoIcon,
		.label = std::move(currentVal),
		.onClick = [=] {
			if (const auto controller = Core::App().activeWindow()->sessionController()) {
				controller->show(Box(
						[=](not_null<Ui::GenericBox*> box) {
							const auto save = [=](int index) {
								const auto option = availableOptions[index].first;
								SolarSettings::getInstance().setTranslationProvider(option);

								if constexpr (Platform::IsMac()) {
									if (option == TranslationProvider::Native) {
										controller->showToast(Ui::Toast::Config{
											.text = tr::lng_translate_settings_use_platform_mac_about(tr::now, tr::rich),
											.duration = 6 * crl::time(1000)
										});
									}
								}
							};
							SingleChoiceBox(box, {
								.title = tr::solar_TranslationProvider(),
								.options = optionLabels,
								.initialSelection = getIndex(settings->translationProvider()),
								.callback = save,
							});
						}));
			}
		},
	});
	if (button) {
		solar.addBetaBadge(button);
	}
}

void BuildShowPeerId(SectionBuilder &builder) {
	auto *settings = &SolarSettings::getInstance();

	const auto options = std::vector{
		QString(tr::solar_SettingsShowID_Hide(tr::now)),
		QString("Telegram API"),
		QString("Bot API")
	};

	auto currentVal = SolarSettings::getInstance().showPeerIdValue()
		| rpl::map([=](PeerIdDisplay val) {
			return options[static_cast<int>(val)];
		});

	const auto controller = builder.controller();
	builder.addButton({
		.id = u"solar/showPeerId"_q,
		.altIds = { u"solar/showIdAndDc"_q },
		.title = tr::solar_SettingsShowID(),
		.st = &st::settingsButtonNoIcon,
		.label = std::move(currentVal),
		.onClick = [=] {
			controller->show(Box(
				[=](not_null<Ui::GenericBox*> box) {
					const auto save = [=](int index) {
						SolarSettings::getInstance().setShowPeerId(
							static_cast<PeerIdDisplay>(index));
					};
					SingleChoiceBox(box, {
						.title = tr::solar_SettingsShowID(),
						.options = options,
						.initialSelection = static_cast<int>(settings->showPeerId()),
						.callback = save,
					});
				}));
		},
	});
}

void BuildQoLToggles(SectionBuilder &builder, SolarSectionBuilder &solar) {
	auto *settings = &SolarSettings::getInstance();

	BuildTranslator(builder, solar);
	solar.addSectionDivider();

	builder.addSubsectionTitle(tr::solar_CategoryGeneral());

	const auto controller = builder.controller();
	solar.addToggle({
		.id = u"solar/disableStories"_q,
		.altIds = { u"solar/hideStories"_q },
		.title = tr::solar_DisableStories(),
		.getter = [=] { return settings->disableStories(); },
		.setter = [=](bool enabled) {
			SolarSettings::getInstance().setDisableStories(enabled);
			ShowRestartPrompt(controller);
		},
	});

	solar.addSettingToggle({
		.id = u"solar/disableOpenLinkWarning"_q,
		.title = tr::solar_DisableOpenLinkWarning(),
		.getter = &SolarSettings::disableOpenLinkWarning,
		.setter = &SolarSettings::setDisableOpenLinkWarning,
	});

	solar.addCollapsibleToggle({
		.id = u"solar/similarChannels"_q,
		.title = tr::solar_DisableSimilarChannels(),
		.checkboxes = {
			NestedEntry{
				tr::solar_CollapseSimilarChannels(tr::now),
				[] { return SolarSettings::getInstance().collapseSimilarChannels(); },
				[](bool v) { SolarSettings::getInstance().setCollapseSimilarChannels(v); }
			},
			NestedEntry{
				tr::solar_HideSimilarChannelsTab(tr::now),
				[] { return SolarSettings::getInstance().hideSimilarChannels(); },
				[](bool v) { SolarSettings::getInstance().setHideSimilarChannels(v); }
			}
		},
		.toggledWhenAll = true,
	});

	solar.addSettingToggle({
		.id = u"solar/disableNotificationsDelay"_q,
		.title = tr::solar_DisableNotificationsDelay(),
		.getter = &SolarSettings::disableNotificationsDelay,
		.setter = &SolarSettings::setDisableNotificationsDelay,
	});

	solar.addSectionDivider();

	const auto zalgoButton = builder.addButton({
		.id = u"solar/filterZalgo"_q,
		.title = tr::solar_FilterZalgo(),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings->filterZalgo()),
	});
	if (zalgoButton) {
		zalgoButton->toggledValue(
		) | rpl::filter(
			[=](bool enabled) {
				return (enabled != settings->filterZalgo());
			}
		) | on_next(
			[=](bool enabled) {
				SolarSettings::getInstance().setFilterZalgo(enabled);
				ShowRestartPrompt(controller);
			},
			zalgoButton->lifetime());
		solar.addBetaBadge(zalgoButton);
	}

	solar.addSettingToggle({
		.id = u"solar/improveLinkPreviews"_q,
		.title = tr::solar_ImproveLinkPreviews(),
		.getter = &SolarSettings::improveLinkPreviews,
		.setter = &SolarSettings::setImproveLinkPreviews,
	});
	solar.addCollapsibleToggle({
		.id = u"solar/confirmations"_q,
		.title = tr::solar_ConfirmationsTitle(),
		.checkboxes = {
			NestedEntry{
				tr::solar_StickerConfirmation(tr::now),
				[] { return SolarSettings::getInstance().stickerConfirmation(); },
				[](bool v) { SolarSettings::getInstance().setStickerConfirmation(v); }
			},
			NestedEntry{
				tr::solar_GIFConfirmation(tr::now),
				[] { return SolarSettings::getInstance().gifConfirmation(); },
				[](bool v) { SolarSettings::getInstance().setGifConfirmation(v); }
			},
			NestedEntry{
				tr::solar_VoiceConfirmation(tr::now),
				[] { return SolarSettings::getInstance().voiceConfirmation(); },
				[](bool v) { SolarSettings::getInstance().setVoiceConfirmation(v); }
			},
			NestedEntry{
				tr::solar_RoundConfirmation(tr::now),
				[] { return SolarSettings::getInstance().roundConfirmation(); },
				[](bool v) { SolarSettings::getInstance().setRoundConfirmation(v); }
			}
		},
		.toggledWhenAll = false,
	});
	solar.addSettingToggle({
		.id = u"solar/showMessageSeconds"_q,
		.altIds = { u"solar/formatTimeWithSeconds"_q },
		.title = tr::solar_SettingsShowMessageSeconds(),
		.getter = &SolarSettings::showMessageSeconds,
		.setter = &SolarSettings::setShowMessageSeconds,
	});

	BuildShowPeerId(builder);

	solar.addSectionDivider();

	builder.addSubsectionTitle(tr::solar_PrivacySecurityHeader());

	solar.addSettingToggle({
		.id = u"solar/autoStripTrackingOnSend"_q,
		.title = tr::solar_AutoStripTrackingOnSend(),
		.getter = &SolarSettings::autoStripTrackingOnSend,
		.setter = &SolarSettings::setAutoStripTrackingOnSend,
	});
	solar.addSettingToggle({
		.id = u"solar/loginCodeLeakGuard"_q,
		.title = tr::solar_LoginCodeLeakGuard(),
		.getter = &SolarSettings::loginCodeLeakGuard,
		.setter = &SolarSettings::setLoginCodeLeakGuard,
	});
	solar.addSettingToggle({
		.id = u"solar/antiCaps"_q,
		.title = tr::solar_AntiCaps(),
		.getter = &SolarSettings::antiCaps,
		.setter = &SolarSettings::setAntiCaps,
	});
	solar.addSettingToggle({
		.id = u"solar/forceRelayCalls"_q,
		.title = tr::solar_ForceRelayCalls(),
		.getter = &SolarSettings::forceRelayCalls,
		.setter = &SolarSettings::setForceRelayCalls,
	});
	solar.addSettingToggle({
		.id = u"solar/callConfirmation"_q,
		.title = tr::solar_CallConfirmation(),
		.getter = &SolarSettings::callConfirmation,
		.setter = &SolarSettings::setCallConfirmation,
	});
	solar.addSettingToggle({
		.id = u"solar/exactCounters"_q,
		.title = tr::solar_ExactCounters(),
		.getter = &SolarSettings::exactCounters,
		.setter = &SolarSettings::setExactCounters,
	});
	solar.addSettingToggle({
		.id = u"solar/alwaysOnTop"_q,
		.title = tr::solar_AlwaysOnTop(),
		.getter = &SolarSettings::alwaysOnTop,
		.setter = &SolarSettings::setAlwaysOnTop,
	});
	solar.addSettingToggle({
		.id = u"solar/stripImageExif"_q,
		.title = tr::solar_StripImageExif(),
		.getter = &SolarSettings::stripImageExif,
		.setter = &SolarSettings::setStripImageExif,
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_StripImageExifAbout());
	builder.addSkip();
	solar.addSettingToggle({
		.id = u"solar/forcedCopy"_q,
		.title = tr::solar_ForcedCopy(),
		.getter = &SolarSettings::forcedCopy,
		.setter = &SolarSettings::setForcedCopy,
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_ForcedCopyAbout());
	builder.addSkip();
	solar.addSettingToggle({
		.id = u"solar/uploadSpeedBoost"_q,
		.title = tr::solar_UploadSpeedBoost(),
		.getter = &SolarSettings::uploadSpeedBoost,
		.setter = &SolarSettings::setUploadSpeedBoost,
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_UploadSpeedBoostAbout());
	builder.addSkip();
	solar.addSettingToggle({
		.id = u"solar/downloadSpeedBoost"_q,
		.title = tr::solar_DownloadSpeedBoost(),
		.getter = &SolarSettings::downloadSpeedBoost,
		.setter = &SolarSettings::setDownloadSpeedBoost,
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_DownloadSpeedBoostAbout());
	builder.addSkip();
	solar.addSettingToggle({
		.id = u"solar/randomDownloadNames"_q,
		.title = tr::solar_RandomDownloadNames(),
		.getter = &SolarSettings::randomDownloadNames,
		.setter = &SolarSettings::setRandomDownloadNames,
	});
	solar.addToggle({
		.id = u"solar/preferIPv6"_q,
		.title = tr::solar_PreferIPv6(),
		.getter = [] {
			return base::options::lookup<bool>(MTP::details::kOptionPreferIPv6).value();
		},
		.setter = [](bool enabled) {
			base::options::lookup<bool>(MTP::details::kOptionPreferIPv6).set(enabled);
		},
	});

	solar.addSectionDivider();

	builder.addSubsectionTitle(tr::solar_BrainTitle());

	solar.addSettingToggle({
		.id = u"solar/solarBrainEnabled"_q,
		.title = tr::solar_BrainEnable(),
		.getter = &SolarSettings::solarBrainEnabled,
		.setter = &SolarSettings::setSolarBrainEnabled,
	});
	builder.addButton({
		.id = u"solar/solarBrainConfig"_q,
		.title = tr::solar_BrainConfigButton(),
		.st = &st::settingsButtonNoIcon,
		.label = SolarSettings::getInstance().solarBrainModelValue(),
		.onClick = [=] {
			SolarFeatures::AI::ShowSolarBrainConfigBox(controller);
		},
	});

	solar.addSectionDivider();

	builder.addSubsectionTitle(rpl::single(QString("Webview")));

	solar.addSettingToggle({
		.id = u"solar/spoofWebviewAsAndroid"_q,
		.title = tr::solar_SettingsSpoofWebviewAsAndroid(),
		.getter = &SolarSettings::spoofWebviewAsAndroid,
		.setter = &SolarSettings::setSpoofWebviewAsAndroid,
	});

	solar.addCollapsibleToggle({
		.id = u"solar/biggerWindow"_q,
		.title = tr::solar_SettingsBiggerWindow(),
		.checkboxes = {
			NestedEntry{
				tr::solar_SettingsIncreaseWebviewHeight(tr::now),
				[] { return SolarSettings::getInstance().increaseWebviewHeight(); },
				[](bool v) { SolarSettings::getInstance().setIncreaseWebviewHeight(v); }
			},
			NestedEntry{
				tr::solar_SettingsIncreaseWebviewWidth(tr::now),
				[] { return SolarSettings::getInstance().increaseWebviewWidth(); },
				[](bool v) { SolarSettings::getInstance().setIncreaseWebviewWidth(v); }
			}
		},
		.toggledWhenAll = false,
	});
}

const auto kMeta = BuildHelper({
	.id = SolarGeneral::Id(),
	.parentId = SolarMain::Id(),
	.title = &tr::solar_CategoryGeneral,
	.icon = &st::menuIconShowAll,
}, [](SectionBuilder &builder) {
	auto solar = SolarSectionBuilder(builder);

	builder.addSkip();
	BuildQoLToggles(builder, solar);
	builder.addSkip();
});

} // namespace

rpl::producer<QString> SolarGeneral::title() {
	return tr::solar_CategoryGeneral();
}

SolarGeneral::SolarGeneral(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void SolarGeneral::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type SolarGeneralId() {
	return SolarGeneral::Id();
}

} // namespace Settings
