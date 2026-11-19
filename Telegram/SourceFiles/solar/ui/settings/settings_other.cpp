// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/ui/settings/settings_other.h"

#include "lang_auto.h"
#include "solar/solar_settings.h"
#include "solar/ui/settings/solar_builder.h"
#include "solar/ui/settings/settings_solar_utils.h"
#include "solar/ui/settings/settings_main.h"
#include "boxes/abstract_box.h"
#include "core/application.h"
#include "lang/lang_text_entity.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/boxes/confirm_box.h"
#include "ui/text/text_utilities.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

#include <QDesktopServices>
#include <QUrl>

namespace Settings {

using namespace Builder;
using namespace SolarBuilder;

namespace {

void BuildProjectLinks(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle(rpl::single(QString("SolarGram Project")));

	builder.addButton({
		.id = u"solar/githubRepo"_q,
		.title = rpl::single(QString("Source Code")),
		.icon = { &st::menuIconIpAddress },
		.label = rpl::single(QString("JoiveSer/SolarGram")),
		.onClick = [] {
			QDesktopServices::openUrl(QUrl(u"https://github.com/JoiveSer/SolarGram"_q));
		},
	});
	builder.addButton({
		.id = u"solar/githubReleases"_q,
		.title = rpl::single(QString("Releases & Builds")),
		.icon = { &st::menuIconChannel },
		.label = rpl::single(QString("GitHub Releases")),
		.onClick = [] {
			QDesktopServices::openUrl(QUrl(u"https://github.com/JoiveSer/SolarGram/releases"_q));
		},
	});
	builder.addSkip();
	builder.addDividerText(rpl::single(
		u"SolarGram runs strictly without third-party telemetry, external language CDNs, or hidden endpoints."_q));
}

void BuildCrashReporting(SectionBuilder &builder, SolarSectionBuilder &solar) {
#ifndef TDESKTOP_DISABLE_AUTOUPDATE
	builder.addSkip();
	builder.addSubsectionTitle(tr::solar_CategoryOther());

	solar.addSettingToggle({
		.id = u"solar/crashReporting"_q,
		.altIds = { u"solar/crashlytics"_q },
		.title = tr::solar_CrashReporting(),
		.getter = &SolarSettings::crashReporting,
		.setter = &SolarSettings::setCrashReporting,
		.icon = { &st::menuIconReport },
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_CrashReportingDescription());
#endif
}

void BuildOtherThings(SectionBuilder &builder) {
	const auto controller = builder.controller();

	builder.addSkip();
	builder.addButton({
		.id = u"solar/registerUrlScheme"_q,
		.title = tr::solar_RegisterURLScheme(),
		.icon = { &st::menuIconLink },
		.onClick = [=] {
			Core::Application::RegisterUrlScheme();
			controller->showToast(tr::lng_box_done(tr::now));
		},
	});
	builder.addButton({
		.id = u"solar/resetSettings"_q,
		.title = tr::solar_ResetSettings(),
		.icon = { &st::menuIconRestore },
		.onClick = [=] {
			controller->show(Ui::MakeConfirmBox({
				.text = tr::solar_ResetSettingsConfirmation(tr::rich),
				.confirmed = [=](Fn<void()> &&close) {
					SolarSettings::reset();
					controller->showToast(tr::lng_box_done(tr::now));
					close();
				},
				.confirmText = tr::lng_box_yes(),
			}));
		},
	});
	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = SolarOther::Id(),
	.parentId = SolarMain::Id(),
	.title = &tr::solar_CategoryOther,
	.icon = &st::menuIconFave,
}, [](SectionBuilder &builder) {
	auto solar = SolarSectionBuilder(builder);

	builder.addSkip();
	BuildProjectLinks(builder);
	BuildCrashReporting(builder, solar);
	BuildOtherThings(builder);
});

} // namespace

rpl::producer<QString> SolarOther::title() {
	return tr::solar_CategoryOther();
}

SolarOther::SolarOther(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void SolarOther::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type SolarOtherId() {
	return SolarOther::Id();
}

} // namespace Settings
