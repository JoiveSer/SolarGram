// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/ui/settings/settings_main.h"

#include "settings/sections/settings_main.h"
#include "lang_auto.h"
#include "solar/solar_settings.h"
#include "solar/ui/solar_logo.h"
#include "solar/ui/settings/settings_appearance.h"
#include "solar/ui/settings/settings_solar.h"
#include "solar/ui/settings/settings_chats.h"
#include "solar/ui/settings/settings_filters.h"
#include "solar/ui/settings/settings_general.h"
#include "solar/ui/settings/settings_other.h"
#include "core/version.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_solar_settings.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/painter.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "window/window_session_controller_link_info.h"

#include <QDesktopServices>

namespace Settings {

using namespace Builder;

namespace {

void BuildLogo(SectionBuilder &builder) {
	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		auto logo = object_ptr<Ui::RpWidget>(ctx.container);
		const auto logoRaw = logo.data();
		logoRaw->resize(
			QSize(st::settingsCloudPasswordIconSize,
				st::settingsCloudPasswordIconSize));
		logoRaw->setNaturalWidth(st::settingsCloudPasswordIconSize);
		logoRaw->paintRequest(
		) | rpl::on_next([=] {
			auto p = QPainter(logoRaw);
			const auto image = SolarAssets::currentAppLogoPad();
			if (!image.isNull()) {
				const auto size = st::settingsCloudPasswordIconSize;
				const auto scaled = image.scaled(
					size * style::DevicePixelRatio(),
					size * style::DevicePixelRatio(),
					Qt::KeepAspectRatio,
					Qt::SmoothTransformation);
				p.drawImage(QRect(0, 0, size, size), scaled);
			}
		}, logoRaw->lifetime());
		return { .widget = std::move(logo), .align = style::al_top };
	});
}

void BuildVersionInfo(SectionBuilder &builder) {
	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<Ui::FlatLabel>(
				ctx.container,
				rpl::single(
					QString("SolarGram Desktop v")
					+ QString::fromLatin1(AppVersionStr)),
				st::boxTitle),
			.align = style::al_top,
		};
	});

	builder.addSkip();

	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<Ui::FlatLabel>(
				ctx.container,
				tr::solar_SettingsDescription(),
				st::centeredBoxLabel),
			.align = style::al_top,
		};
	});
}

void BuildCategories(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSkip();
	builder.addSkip();
	builder.addSkip();
	builder.addDivider();
	builder.addSkip();

	builder.addSubsectionTitle(tr::solar_CategoriesHeader());

	builder.addSectionButton({
		.title = rpl::single(QString("SolarGram")),
		.targetSection = SolarGhost::Id(),
		.icon = { &st::menuIconGroupReactions },
	});
	builder.addSectionButton({
		.title = tr::solar_CategoryFilters(),
		.targetSection = SolarFilters::Id(),
		.icon = { &st::menuIconTagFilter },
	});
	builder.addSectionButton({
		.title = tr::solar_CategoryGeneral(),
		.targetSection = SolarGeneral::Id(),
		.icon = { &st::menuIconShowAll },
	});
	builder.addSectionButton({
		.title = tr::solar_CategoryAppearance(),
		.targetSection = SolarAppearance::Id(),
		.icon = { &st::menuIconPalette },
	});
	builder.addSectionButton({
		.title = tr::solar_CategoryChats(),
		.targetSection = SolarChats::Id(),
		.icon = { &st::menuIconChatBubble },
	});
	builder.addSectionButton({
		.title = tr::solar_CategoryOther(),
		.targetSection = SolarOther::Id(),
		.icon = { &st::menuIconFave },
	});
}

void BuildLinks(SectionBuilder &builder) {
	builder.addSkip();
	builder.addDivider();
	builder.addSkip();

	builder.addSubsectionTitle(tr::solar_LinksHeader());

	builder.addButton({
		.id = u"solar/channel"_q,
		.title = rpl::single(QString("GitHub Repository")),
		.icon = { &st::menuIconChannel },
		.label = rpl::single(QString("JoiveSer/SolarGram")),
		.onClick = [=] {
			QDesktopServices::openUrl(
				QString("https://github.com/JoiveSer/SolarGram"));
		},
	});
	builder.addButton({
		.id = u"solar/chat"_q,
		.title = rpl::single(QString("Releases")),
		.icon = { &st::menuIconChats },
		.label = rpl::single(QString("GitHub Releases")),
		.onClick = [=] {
			QDesktopServices::openUrl(
				QString("https://github.com/JoiveSer/SolarGram/releases"));
		},
	});
	builder.addButton({
		.id = u"solar/crowdin"_q,
		.title = rpl::single(QString("Features Catalog")),
		.icon = { &st::menuIconTranslate },
		.label = rpl::single(QString("COMBINE CATALOG")),
		.onClick = [=] {
			QDesktopServices::openUrl(
				QString("https://github.com/JoiveSer/SolarGram/blob/main/docs/SOLARGRAM_FEATURES_CATALOG.md"));
		},
	});
	builder.addButton({
		.id = u"solar/website"_q,
		.title = tr::solar_LinksDocumentation(),
		.icon = { &st::menuIconIpAddress },
		.label = rpl::single(QString("README")),
		.onClick = [=] {
			QDesktopServices::openUrl(
				QString("https://github.com/JoiveSer/SolarGram#readme"));
		},
	});

	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = SolarMain::Id(),
	.parentId = MainId(),
	.title = &tr::solar_Preferences,
	.icon = &st::menuIconPremium,
}, [](SectionBuilder &builder) {
	BuildLogo(builder);
	builder.addSkip();
	BuildVersionInfo(builder);
	BuildCategories(builder);
	BuildLinks(builder);
});

} // namespace

rpl::producer<QString> SolarMain::title() {
	return rpl::single(QString(""));
}

SolarMain::SolarMain(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void SolarMain::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type SolarMainId() {
	return SolarMain::Id();
}

} // namespace Settings
