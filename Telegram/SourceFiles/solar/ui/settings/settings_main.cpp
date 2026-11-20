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
#include "styles/style_boxes.h"
#include "ui/layers/generic_box.h"
#include "ui/painter.h"
#include "ui/text/text_utilities.h"
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
	builder.addButton({
		.id = u"solar/credits"_q,
		.title = rpl::single(QString("Credits & Donors")),
		.icon = { &st::menuIconInfo },
		.label = rpl::single(QString("Open-Source Attribution")),
		.onClick = [controller = builder.controller()] {
			if (controller) {
				controller->show(Box(FillSolarCreditsBox));
			}
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

void FillSolarCreditsBox(not_null<Ui::GenericBox*> box) {
	box->setTitle(rpl::single(QString("SolarGram Credits & Ported Modules")));
	const auto addDonor = [&](const QString &name, const QString &url, const QString &modules) {
		auto rich = Ui::Text::Link(name, url);
		rich.append(u" — "_q + modules);
		const auto label = box->addRow(
			object_ptr<Ui::FlatLabel>(box, rpl::single(std::move(rich)), st::boxLabel),
			st::boxRowPadding);
		label->setLinksTrusted();
	};
	addDonor(
		u"Telegram Desktop (tdesktop)"_q,
		u"https://github.com/telegramdesktop/tdesktop"_q,
		u"Base GPLv3 client engine."_q);
	addDonor(
		u"AyuGramDesktop"_q,
		u"https://github.com/AyuGram/AyuGramDesktop"_q,
		u"Ghost Mode 0/5, Anti-Recall (Off/RAM/DB + media), OneView TTL, per-chat Spy exclusions, WAL Mode, MainFont, Upload Boost, Regex Filters, Message Shot, Streamer Mode."_q);
	addDonor(
		u"Partisan Telegram / Vartagram"_q,
		u"https://github.com/wrwrabbit/tdesktop"_q,
		u"Duress PIN (Decoy & Silent Wipe), #PIN hidden chats, Panic Wipe (Ctrl+Shift+F12)."_q);
	addDonor(
		u"SereinGram"_q,
		u"https://github.com/eltavine/SereinGram"_q,
		u"Tracking URL stripper, Win32 OBS Sentinel, Streamer masking, DeepLX & TranSmart translators, Pangu CJK-Latin spacing, Chat Summary, Recent Chats, To Saved (No Author)."_q);
	addDonor(
		u"GhostgramPC"_q,
		u"https://github.com/ichmagmaus111/GhostgramPC"_q,
		u"Scheduled Ghost send (+12s), Read Until Here, Eternal Online, Forward Without Author, Anti-Caps, Quoted Reply on Deleted Messages."_q);
	addDonor(
		u"Regram"_q,
		u"https://github.com/RegramDev/Regram"_q,
		u"ICU regex ReDoS time and stack safety limits."_q);
	addDonor(
		u"LuxuryGram"_q,
		u"https://github.com/GofMan5/LuxuryGram"_q,
		u"Force Relay Calls (no P2P leak), Call Confirmation, unquoted Repeat with rich preview."_q);
	addDonor(
		u"Kotatogram"_q,
		u"https://github.com/kotatogram/kotatogram-desktop"_q,
		u"Local Folders (Unread / Unmuted / Admin / Types) and text replacements (:shrug:)."_q);
	addDonor(
		u"Materialgram"_q,
		u"https://github.com/kukuruzka165/materialgram"_q,
		u"Period bulk delete (100-batch RPC chunker), round video seekbar, MD3 switch styling."_q);
	addDonor(
		u"Forkgram"_q,
		u"https://github.com/crazypeace/forkgram-tdesktop"_q,
		u"Delete for Everyone checked by default, title-bar Always-on-Top pin control."_q);
	addDonor(
		u"entinyGram / Donutgram"_q,
		u"https://github.com/entaytion/entinyGram"_q,
		u"Presence History log, Remembered Last Seen, Peek Last Seen, initConnection Device Spoofing."_q);
	addDonor(
		u"Novagram / Nagram"_q,
		u"https://github.com/confeden/Novagram"_q,
		u"EXIF/XMP/IPTC stripping on outgoing photos, Forced Copy/Save in restricted chats."_q);
	addDonor(
		u"64Gram / NFGram"_q,
		u"https://github.com/TDesktop-x64/tdesktop"_q,
		u"Exact unrounded counters, Upload & Download Speed Boost, Similar Channels hide guard, Prefer IPv6 option."_q);
	addDonor(
		u"decoder-dev/Telegram-iOS"_q,
		u"https://github.com/decoder-dev/Telegram-iOS"_q,
		u"5-digit 777000 login code leak guard (RAM-only)."_q);
	addDonor(
		u"Telebrain"_q,
		u"https://github.com/fedorn/telebrain"_q,
		u"SolarBrain Local AI Copilot (Ollama / OpenAI-compatible manual prompt pipeline)."_q);
	box->addButton(tr::lng_close(), [=] { box->closeBox(); });
	box->setWidth(st::boxWideWidth);
}

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
