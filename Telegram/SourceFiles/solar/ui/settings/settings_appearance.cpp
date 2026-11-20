// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/ui/settings/settings_appearance.h"

#include "lang_auto.h"
#include "solar/solar_settings.h"
#include "solar/solar_ui_settings.h"
#include "solar/ui/boxes/font_selector.h"
#include "solar/ui/components/avatar_corners_preview.h"
#include "solar/ui/components/icon_picker.h"
#include "solar/ui/settings/solar_builder.h"
#include "solar/ui/settings/settings_solar_utils.h"
#include "solar/ui/settings/settings_main.h"
#include "inline_bots/bot_attach_web_view.h"
#include "main/main_session.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_solar_icons.h"
#include "styles/style_solar_styles.h"
#include "styles/style_dialogs.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/painter.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/padding_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

namespace Settings {

using namespace Builder;
using namespace SolarBuilder;

namespace {

bool HasDrawerBots(not_null<Window::SessionController*> controller) {
	// todo: maybe iterate through all accounts
	const auto bots = &controller->session().attachWebView();
	for (const auto &bot : bots->attachBots()) {
		if (!bot.inMainMenu || !bot.media) {
			continue;
		}
		return true;
	}
	return false;
}

void BuildAppIcon(SectionBuilder &builder, SolarSectionBuilder &solar) {
	builder.addSubsectionTitle({
		.id = u"solar/appIcon"_q,
		.title = tr::solar_AppIconHeader(),
	});

	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<IconPicker>(ctx.container),
			.margin = st::settingsButtonNoIcon.padding,
		};
	});

#if defined Q_OS_WIN || defined Q_OS_MAC
	builder.addDivider();
	builder.addSkip();
	solar.addSettingToggle({
		.id = u"solar/hideNotificationBadge"_q,
		.title = tr::solar_HideNotificationBadge(),
		.getter = &SolarSettings::hideNotificationBadge,
		.setter = &SolarSettings::setHideNotificationBadge,
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_HideNotificationBadgeDescription());
	builder.addSkip();
#else
    builder.addDivider();
    builder.addSkip();
#endif
}

void BuildAvatarCorners(SectionBuilder &builder, SolarSectionBuilder &solar) {
	auto *settings = &SolarSettings::getInstance();
	const auto controller = builder.controller();

	const auto mapRadius = [](int val)
	{
		if (val == 0) {
			return tr::solar_AvatarCornersSquare(tr::now).toUpper();
		} else if (val == SolarUiSettings::kMaxAvatarCorners) {
			return tr::solar_AvatarCornersCircle(tr::now).toUpper();
		}
		return QString::number(val);
	};

	builder.add([=](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		const auto container = ctx.container;
		auto title = object_ptr<Ui::FlatLabel>(
			container,
			tr::solar_AvatarCorners(),
			st::defaultSubsectionTitle);
		const auto titleRaw = title.data();

		const auto badge = Ui::CreateChild<Ui::PaddingWrap<Ui::FlatLabel>>(
			container,
			object_ptr<Ui::FlatLabel>(
				container,
				settings->avatarCornersValue() | rpl::map(mapRadius),
				st::settingsPremiumNewBadge),
			st::solarBetaBadgePadding);
		badge->show();
		badge->setAttribute(Qt::WA_TransparentForMouseEvents);
		badge->paintRequest() | rpl::on_next([=] {
			auto p = QPainter(badge);
			auto hq = PainterHighQualityEnabler(p);
			p.setPen(Qt::NoPen);
			p.setBrush(st::windowBgActive);
			const auto r = st::solarBetaBadgePadding.left();
			p.drawRoundedRect(badge->rect(), r, r);
		}, badge->lifetime());

		titleRaw->geometryValue() | rpl::on_next([=](QRect geometry) {
			badge->moveToLeft(
				geometry.x()
					+ titleRaw->textMaxWidth()
					+ st::settingsPremiumNewBadgePosition.x(),
				geometry.y()
					+ (geometry.height() - badge->height()) / 2);
		}, badge->lifetime());

		return {
			.widget = std::move(title),
			.margin = st::defaultSubsectionTitlePadding,
		};
	}, [] {
		return SearchEntry{
			.id = u"solar/avatarCorners"_q,
			.title = tr::solar_AvatarCorners(tr::now),
		};
	});

	auto *previewRaw = static_cast<AvatarCornersPreview*>(nullptr);
	builder.add([&](const Builder::WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		auto preview = object_ptr<AvatarCornersPreview>(
			ctx.container,
			controller);
		previewRaw = preview.data();
		const auto vMargin = st::settingsButtonNoIcon.padding
			- st::defaultDialogRow.padding;
		return {
			.widget = std::move(preview),
			.margin = QMargins(0, vMargin.top(), 0, vMargin.bottom()),
		};
	});

	solar.addSlider({
		.id = u"solar/avatarCornersSlider"_q,
		.title = rpl::single(QString()),
		.showTitle = false,
		.steps = SolarUiSettings::kMaxAvatarCorners + 1,
		.current = settings->avatarCorners(),
		.onChanged = [=](int val) {
			SolarSettings::getInstance().setAvatarCorners(val);
			if (previewRaw) {
				previewRaw->update();
			}
		},
		.onFinalChanged = [=](int val) {
			SolarSettings::getInstance().setAvatarCorners(val);
			ShowRestartPrompt(controller);
		},
	});

	solar.addSettingToggle({
		.id = u"solar/singleCornerRadius"_q,
		.title = tr::solar_SingleCornerRadius(),
		.getter = &SolarSettings::singleCornerRadius,
		.setter = &SolarSettings::setSingleCornerRadius,
	});

	builder.addSkip();
	builder.addDividerText(tr::solar_SingleCornerRadiusDescription());
	builder.addSkip();
}

void BuildAppearance(SectionBuilder &builder, SolarSectionBuilder &solar) {
	auto *settings = &SolarSettings::getInstance();

	builder.addSubsectionTitle(tr::solar_CategoryAppearance());

	solar.addSettingToggle({
		.id = u"solar/materialSwitches"_q,
		.altIds = { u"solar/newSwitchStyle"_q },
		.title = tr::solar_MaterialSwitches(),
		.getter = &SolarSettings::materialSwitches,
		.setter = &SolarSettings::setMaterialSwitches,
	});
	solar.addSettingToggle({
		.id = u"solar/disableCustomBackgrounds"_q,
		.altIds = { u"solar/customThemes"_q },
		.title = tr::solar_DisableCustomBackgrounds(),
		.getter = &SolarSettings::disableCustomBackgrounds,
		.setter = &SolarSettings::setDisableCustomBackgrounds,
	});
	solar.addSettingToggle({
		.id = u"solar/hidePremiumStatuses"_q,
		.title = tr::solar_HidePremiumStatuses(),
		.getter = &SolarSettings::hidePremiumStatuses,
		.setter = &SolarSettings::setHidePremiumStatuses,
	});

	const auto controller = builder.controller();
	builder.addButton({
		.id = u"solar/monoFont"_q,
		.title = tr::solar_MonospaceFont(),
		.st = &st::settingsButtonNoIcon,
		.label = rpl::single(
			settings->monoFont().isEmpty()
				? tr::solar_FontDefault(tr::now)
				: settings->monoFont()),
		.onClick = [weak = base::make_weak(controller)] {
			if (const auto c = weak.get()) {
				SolarUi::FontSelectorBox::Show(
					c,
					[](const QString &font) {
						SolarSettings::getInstance().setMonoFont(font);
					});
			}
		},
	});
	builder.addButton({
		.id = u"solar/mainFont"_q,
		.title = tr::solar_MainFont(),
		.st = &st::settingsButtonNoIcon,
		.label = settings->mainFontValue(
		) | rpl::map([=](const QString &font) {
			return font.isEmpty() ? tr::solar_FontDefault(tr::now) : font;
		}),
		.onClick = [weak = base::make_weak(controller)] {
			if (const auto c = weak.get()) {
				SolarUi::FontSelectorBox::Show(
					c,
					[](const QString &font) {
						SolarSettings::getInstance().setMainFont(font);
					},
					SolarSettings::getInstance().mainFont());
			}
		},
	});
	solar.addSettingToggle({
		.id = u"solar/showAlwaysOnTopButton"_q,
		.title = tr::solar_ShowAlwaysOnTopButton(),
		.getter = &SolarSettings::showAlwaysOnTopButton,
		.setter = &SolarSettings::setShowAlwaysOnTopButton,
	});
	solar.addSettingToggle({
		.id = u"solar/streamerMaskPrivacy"_q,
		.title = tr::solar_StreamerMaskPrivacy(),
		.getter = &SolarSettings::streamerMaskPrivacy,
		.setter = &SolarSettings::setStreamerMaskPrivacy,
	});
	solar.addSettingToggle({
		.id = u"solar/streamerMaskNotifications"_q,
		.title = tr::solar_StreamerMaskNotifications(),
		.getter = &SolarSettings::streamerMaskNotifications,
		.setter = &SolarSettings::setStreamerMaskNotifications,
	});

	solar.addSectionDivider();
}

void BuildChatFolders(SectionBuilder &builder, SolarSectionBuilder &solar) {
	builder.addSubsectionTitle(tr::solar_ChatFoldersHeader());

	solar.addSettingToggle({
		.id = u"solar/hideNotificationCounters"_q,
		.altIds = { u"solar/tabCounter"_q },
		.title = tr::solar_HideNotificationCounters(),
		.getter = &SolarSettings::hideNotificationCounters,
		.setter = &SolarSettings::setHideNotificationCounters,
	});
	solar.addSettingToggle({
		.id = u"solar/hideAllChatsFolder"_q,
		.altIds = { u"solar/hideAllChats"_q },
		.title = tr::solar_HideAllChats(),
		.getter = &SolarSettings::hideAllChatsFolder,
		.setter = &SolarSettings::setHideAllChatsFolder,
	});
	solar.addSettingToggle({
		.id = u"solar/localFolders"_q,
		.title = tr::solar_LocalFolders(),
		.getter = &SolarSettings::localFoldersEnabled,
		.setter = &SolarSettings::setLocalFoldersEnabled,
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_LocalFoldersAbout());
	builder.addSkip();

	solar.addSectionDivider();
}

void BuildTrayElements(SectionBuilder &builder, SolarSectionBuilder &solar) {
	builder.addSubsectionTitle(tr::solar_TrayElementsHeader());

	solar.addSettingToggle({
		.id = u"solar/showGhostToggleInTray"_q,
		.title = tr::solar_EnableGhostModeTray(),
		.getter = &SolarSettings::showGhostToggleInTray,
		.setter = &SolarSettings::setShowGhostToggleInTray,
	});

#if defined Q_OS_WIN || defined Q_OS_MAC
	solar.addSettingToggle({
		.id = u"solar/showStreamerToggleInTray"_q,
		.title = tr::solar_EnableStreamerModeTray(),
		.getter = &SolarSettings::showStreamerToggleInTray,
		.setter = &SolarSettings::setShowStreamerToggleInTray,
	});
#endif

	solar.addSectionDivider();
}

void BuildDrawerElements(SectionBuilder &builder, SolarSectionBuilder &solar) {
	builder.addSubsectionTitle(tr::solar_DrawerElementsHeader());

	solar.addSettingToggle({
		.id = u"solar/showMyProfileInDrawer"_q,
		.title = tr::lng_menu_my_profile(),
		.getter = &SolarSettings::showMyProfileInDrawer,
		.setter = &SolarSettings::setShowMyProfileInDrawer,
		.icon = { &st::menuIconProfile },
	});

	const auto controller = builder.controller();
	if (controller && HasDrawerBots(controller)) {
		solar.addSettingToggle({
			.id = u"solar/showBotsInDrawer"_q,
			.title = tr::lng_filters_type_bots(),
			.getter = &SolarSettings::showBotsInDrawer,
			.setter = &SolarSettings::setShowBotsInDrawer,
			.icon = { &st::menuIconBot },
		});
	}

	solar.addSettingToggle({
		.id = u"solar/showNewGroupInDrawer"_q,
		.title = tr::lng_create_group_title(),
		.getter = &SolarSettings::showNewGroupInDrawer,
		.setter = &SolarSettings::setShowNewGroupInDrawer,
		.icon = { &st::menuIconGroups },
	});
	solar.addSettingToggle({
		.id = u"solar/showNewChannelInDrawer"_q,
		.title = tr::lng_create_channel_title(),
		.getter = &SolarSettings::showNewChannelInDrawer,
		.setter = &SolarSettings::setShowNewChannelInDrawer,
		.icon = { &st::menuIconChannel },
	});
	solar.addSettingToggle({
		.id = u"solar/showContactsInDrawer"_q,
		.title = tr::lng_menu_contacts(),
		.getter = &SolarSettings::showContactsInDrawer,
		.setter = &SolarSettings::setShowContactsInDrawer,
		.icon = { &st::menuIconUserShow },
	});
	solar.addSettingToggle({
		.id = u"solar/showCallsInDrawer"_q,
		.title = tr::lng_menu_calls(),
		.getter = &SolarSettings::showCallsInDrawer,
		.setter = &SolarSettings::setShowCallsInDrawer,
		.icon = { &st::menuIconPhone },
	});
	solar.addSettingToggle({
		.id = u"solar/showSavedMessagesInDrawer"_q,
		.title = tr::lng_saved_messages(),
		.getter = &SolarSettings::showSavedMessagesInDrawer,
		.setter = &SolarSettings::setShowSavedMessagesInDrawer,
		.icon = { &st::menuIconSavedMessages },
	});
	solar.addSettingToggle({
		.id = u"solar/showLReadToggleInDrawer"_q,
		.title = tr::solar_LReadMessages(),
		.getter = &SolarSettings::showLReadToggleInDrawer,
		.setter = &SolarSettings::setShowLReadToggleInDrawer,
		.icon = { &st::solarLReadMenuIcon },
	});
	solar.addSettingToggle({
		.id = u"solar/showSReadToggleInDrawer"_q,
		.title = tr::solar_SReadMessages(),
		.getter = &SolarSettings::showSReadToggleInDrawer,
		.setter = &SolarSettings::setShowSReadToggleInDrawer,
		.icon = { &st::solarSReadMenuIcon },
	});
	solar.addSettingToggle({
		.id = u"solar/showNightModeToggleInDrawer"_q,
		.title = tr::lng_menu_night_mode(),
		.getter = &SolarSettings::showNightModeToggleInDrawer,
		.setter = &SolarSettings::setShowNightModeToggleInDrawer,
		.icon = { &st::menuIconNightMode },
	});
	solar.addSettingToggle({
		.id = u"solar/showGhostToggleInDrawer"_q,
		.title = tr::solar_GhostModeToggle(),
		.getter = &SolarSettings::showGhostToggleInDrawer,
		.setter = &SolarSettings::setShowGhostToggleInDrawer,
		.icon = { &st::solarGhostIcon },
	});

#if defined Q_OS_WIN || defined Q_OS_MAC
	solar.addSettingToggle({
		.id = u"solar/showStreamerToggleInDrawer"_q,
		.title = tr::solar_StreamerModeToggle(),
		.getter = &SolarSettings::showStreamerToggleInDrawer,
		.setter = &SolarSettings::setShowStreamerToggleInDrawer,
		.icon = { &st::solarStreamerModeMenuIcon },
	});
#endif

	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = SolarAppearance::Id(),
	.parentId = SolarMain::Id(),
	.title = &tr::solar_CategoryAppearance,
	.icon = &st::menuIconPalette,
}, [](SectionBuilder &builder) {
	auto solar = SolarSectionBuilder(builder);

	builder.addSkip();
	BuildAppIcon(builder, solar);
	BuildAvatarCorners(builder, solar);
	BuildAppearance(builder, solar);
	BuildChatFolders(builder, solar);
	BuildTrayElements(builder, solar);
	BuildDrawerElements(builder, solar);
	builder.addSkip();
});

} // namespace

rpl::producer<QString> SolarAppearance::title() {
	return tr::solar_CategoryAppearance();
}

SolarAppearance::SolarAppearance(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void SolarAppearance::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type SolarAppearanceId() {
	return SolarAppearance::Id();
}

} // namespace Settings
