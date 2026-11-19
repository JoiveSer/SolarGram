// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/ui/boxes/donate_info_box.h"

#include "lang/lang_keys.h"
#include "core/ui_integration.h"
#include "core/version.h"
#include "info/profile/info_profile_icon.h"
#include "styles/style_solar_styles.h"
#include "styles/style_boxes.h"
#include "styles/style_giveaway.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_premium.h"
#include "styles/style_settings.h"
#include "styles/style_widgets.h"
#include "ui/painter.h"
#include "ui/rect.h"
#include "ui/rp_widget.h"
#include "ui/vertical_list.h"
#include "ui/layers/generic_box.h"
#include "ui/text/text_utilities.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"

#include <QDesktopServices>
#include <QSvgRenderer>

namespace Ui {
namespace {

QImage MakeSolarLogo() {
	const auto s = Size(st::supportLogoSize);
	auto svg = QSvgRenderer(QString(":/gui/icons/solar/about_logo.svg"));
	auto image = QImage(
		s * style::DevicePixelRatio(),
		QImage::Format_ARGB32_Premultiplied);
	image.setDevicePixelRatio(style::DevicePixelRatio());
	image.fill(Qt::transparent);
	{
		auto p = QPainter(&image);
		svg.render(&p, Rect(s));
	}
	return image;
}

object_ptr<Ui::RpWidget> CreateTopLogoWidget(
		not_null<Ui::RpWidget*> parent) {
	auto w = object_ptr<Ui::RpWidget>(parent);
	const auto raw = w.data();
	const auto logo = MakeSolarLogo();

	raw->paintRequest(
	) | rpl::on_next([=](QRect) {
		QPainter p(raw);
		PainterHighQualityEnabler hq(p);
		const auto original = logo.size() / style::DevicePixelRatio();
		const auto maxWidth = raw->width() - rect::m::sum::h(st::boxRowPadding);
		const auto maxHeight = raw->height();
		if (original.isEmpty() || maxWidth <= 0 || maxHeight <= 0) {
			return;
		}
		const auto scale = std::min(
			double(maxWidth) / double(original.width()),
			double(maxHeight) / double(original.height()));
		const auto target = QSize(
			int(original.width() * scale),
			int(original.height() * scale));
		const auto x = (raw->width() - target.width()) / 2;
		const auto y = (raw->height() - target.height()) / 2;
		p.drawImage(QRect(QPoint(x, y), target), logo);
	}, raw->lifetime());

	return w;
}

object_ptr<Ui::RpWidget> InfoRow(
		not_null<Ui::RpWidget*> parent,
		const QString &title,
		const TextWithEntities &text,
		not_null<const style::icon*> icon,
		const Text::MarkedContext &context) {
	auto row = object_ptr<Ui::VerticalLayout>(parent);
	const auto raw = row.data();

	raw->add(
		object_ptr<Ui::FlatLabel>(
			raw,
			rpl::single(tr::bold(title)),
			st::defaultFlatLabel),
		st::settingsPremiumRowTitlePadding);

	const auto label = raw->add(
		object_ptr<Ui::FlatLabel>(
			raw,
			st::boxDividerLabel),
		st::settingsPremiumRowAboutPadding);
	label->setLinksTrusted();
	label->setMarkedText(text, std::move(context));

	object_ptr<Info::Profile::FloatingIcon>(
		raw,
		*icon,
		st::starrefInfoIconPosition);

	return row;
}

} // namespace

void FillDonateInfoBox(
		not_null<Ui::GenericBox*> box,
		not_null<Window::SessionController*> controller) {
	box->setStyle(st::giveawayGiftCodeBox);
	box->setNoContentMargin(true);
	box->setWidth(int(st::aboutWidth * 1.1));
	box->verticalLayout()->resizeToWidth(box->width());

	box->addTopButton(st::boxTitleClose, [=] { box->closeBox(); });

	Ui::AddSkip(box->verticalLayout());
	Ui::AddSkip(box->verticalLayout());

	const auto logoWidget = box->verticalLayout()->add(
		CreateTopLogoWidget(box->verticalLayout()));
	logoWidget->resize(st::supportLogoSize, st::supportLogoSize);

	Ui::AddSkip(box->verticalLayout());

	box->verticalLayout()->add(
		object_ptr<Ui::FlatLabel>(
			box->verticalLayout(),
			rpl::single(tr::bold(u"SolarGram Desktop"_q)),
			st::boxTitle),
		st::boxRowPadding,
		style::al_top);

	box->verticalLayout()->add(
		object_ptr<Ui::FlatLabel>(
			box->verticalLayout(),
			rpl::single(u"Privacy-hardened Telegram Desktop combine with zero third-party telemetry."_q),
			st::starrefCenteredText),
		st::boxRowPadding);

	Ui::AddSkip(box->verticalLayout());
	Ui::AddSkip(box->verticalLayout());

	box->verticalLayout()->add(InfoRow(
		box->verticalLayout(),
		u"Open Source & Transparent"_q,
		tr::marked(Ui::Text::Link(
			u"github.com/JoiveSer/SolarGram"_q,
			u"https://github.com/JoiveSer/SolarGram"_q)),
		&st::menuIconIpAddress,
		Core::TextContext({ .session = &controller->session() })));

	Ui::AddSkip(box->verticalLayout());

	box->verticalLayout()->add(InfoRow(
		box->verticalLayout(),
		u"Zero Telemetry Architecture"_q,
		TextWithEntities{
			u"Remote telemetry, crash-dump uploads, and third-party analytics endpoints are permanently disabled."_q
		},
		&st::menuIconLock,
		Core::TextContext({ .session = &controller->session() })));

	const auto closeButton = box->addButton(tr::lng_close(), [=] { box->closeBox(); });
	const auto buttonWidth = box->width()
		- rect::m::sum::h(st::starrefFooterBox.buttonPadding);
	closeButton->widthValue() | rpl::filter([=] {
		return (closeButton->widthNoMargins() != buttonWidth);
	}) | rpl::on_next([=] {
		closeButton->resizeToWidth(buttonWidth);
	}, closeButton->lifetime());
}

} // namespace Ui
