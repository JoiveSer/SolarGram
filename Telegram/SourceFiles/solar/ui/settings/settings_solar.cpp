// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/ui/settings/settings_solar.h"

#include "lang_auto.h"
#include "core/file_utilities.h"
#include "solar/solar_settings.h"
#include "solar/data/messages_storage.h"
#include "solar/data/solar_database.h"
#include "solar/features/hotkeys/solar_hotkeys.h"
#include "solar/features/privacy/solar_guards.h"
#include "solar/ui/solar_userpic.h"
#include "solar/ui/boxes/edit_mark_box.h"
#include "solar/ui/settings/solar_builder.h"
#include "solar/ui/settings/settings_solar_utils.h"
#include "solar/ui/settings/settings_main.h"
#include "boxes/peer_list_box.h"
#include "core/application.h"
#include "data/data_user.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_solar_icons.h"
#include "styles/style_solar_styles.h"
#include "styles/style_boxes.h"
#include "styles/style_chat.h"
#include "styles/style_chat_helpers.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "styles/style_widgets.h"
#include "styles/style_window.h"
#include "ui/painter.h"
#include "ui/vertical_list.h"
#include "ui/boxes/confirm_box.h"
#include "ui/boxes/single_choice_box.h"
#include "ui/layers/generic_box.h"
#include "ui/text/text.h"
#include "ui/toast/toast.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/popup_menu.h"
#include "ui/widgets/menu/menu_item_base.h"
#include "ui/wrap/vertical_layout.h"
#include "window/main_window.h"
#include "window/window_controller.h"
#include "window/window_session_controller.h"

#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>

namespace Settings {

using namespace Builder;
using namespace SolarBuilder;

namespace {

struct GhostPickerState {
	rpl::variable<uint64> selectedUserId;
	base::unique_qptr<Ui::PopupMenu> menu;
	Ui::LinkButton *pickerButton = nullptr;
	Fn<void()> refreshCheckboxes;
};

struct AccountUserpicGeometry {
	QRect outer;
	QRect inner;
};

[[nodiscard]] AccountUserpicGeometry AccountUserpic(int height) {
	const auto line = st::mainMenuAccountLine;
	const auto skip = 2 * line + st::lineWidth;
	const auto full = st::mainMenuAccountSize + 2 * skip;
	const auto outer = QRect(
		st::defaultWhoRead.photoLeft
			+ (st::defaultWhoRead.photoSize - full) / 2,
		(height - full) / 2,
		full,
		full);
	return {
		.outer = outer,
		.inner = QRect(
			outer.x() + skip,
			outer.y() + skip,
			st::mainMenuAccountSize,
			st::mainMenuAccountSize),
	};
}

void PaintAccountOutline(Painter &p, QRect outer) {
	const auto line = st::mainMenuAccountLine;
	const auto shift = st::lineWidth + (line * 0.5);
	const auto rect = QRectF(outer).marginsRemoved(QMarginsF(
		shift,
		shift,
		shift,
		shift));
	auto hq = PainterHighQualityEnabler(p);
	auto pen = st::windowBgActive->p;
	pen.setWidthF(line);
	p.setPen(pen);
	p.setBrush(Qt::NoBrush);
	SolarUserpic::PaintShape(p, rect);
}

class AccountAction final : public Ui::Menu::ItemBase {
public:
	AccountAction(
		not_null<Ui::Menu::Menu*> parent,
		const style::Menu &st,
		UserData *user,
		bool active,
		Fn<void()> callback)
	: ItemBase(parent, st)
	, _dummyAction(Ui::CreateChild<QAction>(parent.get()))
	, _user(user)
	, _active(active)
	, _st(st)
	, _height(st::defaultWhoRead.photoSkip * 2 + st::defaultWhoRead.photoSize) {
		setAcceptBoth(true);
		fitToMenuWidth();

		_text.setText(_st.itemStyle, user->name());
		const auto goodWidth = st::defaultWhoRead.nameLeft
			+ _text.maxWidth()
			+ _st.itemPadding.right();
		setMinWidth(std::clamp(goodWidth, _st.widthMin, _st.widthMax));

		setActionTriggered(std::move(callback));

		paintRequest(
		) | rpl::on_next([=] {
			paint(Painter(this));
		}, lifetime());

		enableMouseSelecting();
	}

	not_null<QAction*> action() const override { return _dummyAction; }
	bool isEnabled() const override { return true; }

protected:
	int contentHeight() const override { return _height; }

private:
	void paint(Painter &&p) {
		const auto selected = isSelected();
		if (selected && _st.itemBgOver->c.alpha() < 255) {
			p.fillRect(0, 0, width(), _height, _st.itemBg);
		}
		const auto bg = selected ? _st.itemBgOver : _st.itemBg;
		p.fillRect(0, 0, width(), _height, bg);
		if (isEnabled()) {
			paintRipple(p, 0, 0);
		}

		const auto userpic = AccountUserpic(_height);
		_user->paintUserpicLeft(
			p,
			_userpicView,
			userpic.inner.x(),
			userpic.inner.y(),
			width(),
			userpic.inner.width());
		if (_active) {
			PaintAccountOutline(p, userpic.outer);
		}

		p.setPen(selected ? _st.itemFgOver : _st.itemFg);
		_text.drawLeftElided(
			p,
			st::defaultWhoRead.nameLeft,
			(_height - _st.itemStyle.font->height) / 2,
			width() - st::defaultWhoRead.nameLeft - _st.itemPadding.right(),
			width());
	}

	const not_null<QAction*> _dummyAction;
	UserData *_user = nullptr;
	mutable Ui::PeerUserpicView _userpicView;
	const bool _active = false;
	const style::Menu &_st;
	Ui::Text::String _text;
	const int _height;
};

class GlobalAction final : public Ui::Menu::ItemBase {
public:
	GlobalAction(
		not_null<Ui::Menu::Menu*> parent,
		const style::Menu &st,
		const QString &text,
		bool active,
		Fn<void()> callback)
	: ItemBase(parent, st)
	, _dummyAction(Ui::CreateChild<QAction>(parent.get()))
	, _active(active)
	, _st(st)
	, _height(st::defaultWhoRead.photoSkip * 2 + st::defaultWhoRead.photoSize) {
		setAcceptBoth(true);
		fitToMenuWidth();

		_text.setText(_st.itemStyle, text);
		const auto goodWidth = st::defaultWhoRead.nameLeft
			+ _text.maxWidth()
			+ _st.itemPadding.right();
		setMinWidth(std::clamp(goodWidth, _st.widthMin, _st.widthMax));

		setActionTriggered(std::move(callback));

		paintRequest(
		) | rpl::on_next([=] {
			paint(Painter(this));
		}, lifetime());

		enableMouseSelecting();
	}

	not_null<QAction*> action() const override { return _dummyAction; }
	bool isEnabled() const override { return true; }

protected:
	int contentHeight() const override { return _height; }

private:
	void paint(Painter &&p) {
		const auto selected = isSelected();
		if (selected && _st.itemBgOver->c.alpha() < 255) {
			p.fillRect(0, 0, width(), _height, _st.itemBg);
		}
		const auto bg = selected ? _st.itemBgOver : _st.itemBg;
		p.fillRect(0, 0, width(), _height, bg);
		if (isEnabled()) {
			paintRipple(p, 0, 0);
		}

		const auto userpic = AccountUserpic(_height);
		{
			auto hq = PainterHighQualityEnabler(p);
			auto rect = QRectF(userpic.inner);
			auto gradient = QLinearGradient(rect.topLeft(), rect.bottomLeft());
			gradient.setStops({
				{ 0., st::historyPeer5UserpicBg->c },
				{ 1., st::historyPeer5UserpicBg2->c },
			});
			p.setPen(Qt::NoPen);
			p.setBrush(gradient);
			SolarUserpic::PaintShape(p, rect);
		}
		{
			auto hq = PainterHighQualityEnabler(p);
			p.drawImage(
				userpic.inner,
				st::solarGhostModeGlobalIcon.instance(st::historyPeerUserpicFg->c));
		}
		if (_active) {
			PaintAccountOutline(p, userpic.outer);
		}

		p.setPen(selected ? _st.itemFgOver : _st.itemFg);
		_text.drawLeftElided(
			p,
			st::defaultWhoRead.nameLeft,
			(_height - _st.itemStyle.font->height) / 2,
			width() - st::defaultWhoRead.nameLeft - _st.itemPadding.right(),
			width());
	}

	const not_null<QAction*> _dummyAction;
	const bool _active = false;
	const style::Menu &_st;
	Ui::Text::String _text;
	const int _height;
};

QString GetAccountName(uint64 userId) {
	for (const auto &account : Core::App().domain().orderedAccounts()) {
		if (account->sessionExists()
			&& account->session().userId().bare == userId) {
			return account->session().user()->name();
		}
	}
	return QString("Unknown");
}

QString PickerLabel(uint64 userId) {
	return (userId == 0)
		? tr::solar_GhostModeGlobalSettings(tr::now)
		: GetAccountName(userId);
}

void selectGhostProfile(GhostPickerState *state, uint64 userId) {
	if (state->selectedUserId.current() == userId) {
		return;
	}

	auto wasGlobal = (state->selectedUserId.current() == 0);
	auto nowGlobal = (userId == 0);

	SolarSettings::getInstance().setUseGlobalGhostMode(nowGlobal);

	state->selectedUserId = userId;

	state->pickerButton->setText(PickerLabel(userId));

	state->refreshCheckboxes();

	if (wasGlobal != nowGlobal) {
		Ui::Toast::Show(nowGlobal
			? tr::solar_GhostModeSwitchedToGlobalSettings(tr::now)
			: tr::solar_GhostModeSwitchedToIndividualSettings(tr::now));
	}
}

void BuildGhostEssentials(SectionBuilder &builder) {
	builder.add([](const BuildContext &ctx) {
		v::match(ctx, [&](const WidgetContext &wctx) {
			const auto container = wctx.container;
			const auto controller = wctx.controller;

			auto activeCount = 0;
			for (const auto &account : Core::App().domain().orderedAccounts()) {
				if (account->sessionExists()) {
					++activeCount;
				}
			}

			if (activeCount <= 1 && !SolarSettings::getInstance().useGlobalGhostMode()) {
				auto userId = controller->session().userId().bare;
				auto &src = SolarSettings::ghost(userId);
				auto &dst = SolarSettings::ghost(0);
				dst.setSendReadMessages(src.sendReadMessages());
				dst.setSendReadStories(src.sendReadStories());
				dst.setSendOnlinePackets(src.sendOnlinePackets());
				dst.setSendUploadProgress(src.sendUploadProgress());
				dst.setSendOfflinePacketAfterOnline(src.sendOfflinePacketAfterOnline());
				dst.setMarkReadAfterAction(src.markReadAfterAction());
				dst.setUseScheduledMessages(src.useScheduledMessages());
				dst.setSendWithoutSound(src.sendWithoutSound());
				dst.setSuggestGhostModeBeforeViewingStory(src.suggestGhostModeBeforeViewingStory());
				dst.setSendReadMessagesLocked(src.sendReadMessagesLocked());
				dst.setSendReadStoriesLocked(src.sendReadStoriesLocked());
				dst.setSendOnlinePacketsLocked(src.sendOnlinePacketsLocked());
				dst.setSendUploadProgressLocked(src.sendUploadProgressLocked());
				dst.setSendOfflinePacketAfterOnlineLocked(src.sendOfflinePacketAfterOnlineLocked());
				SolarSettings::getInstance().setUseGlobalGhostMode(true);
			}

			const auto isGlobal = SolarSettings::getInstance().useGlobalGhostMode();
			auto initialUserId = isGlobal
				? uint64(0)
				: controller->session().userId().bare;

			const auto state = container->lifetime().make_state<GhostPickerState>();
			state->selectedUserId = initialUserId;

			const auto title = AddSubsectionTitle(container, tr::solar_GhostEssentialsHeader());

			const auto pickerButton = Ui::CreateChild<Ui::LinkButton>(
				container.get(),
				PickerLabel(initialUserId),
				st::ghostPickerButton);
			state->pickerButton = pickerButton;

			const auto arrow = Ui::CreateChild<Ui::AbstractButton>(container.get());
			{
				const auto &icon = st::ghostPickerArrow;
				arrow->resize(icon.size());
				arrow->paintRequest(
				) | rpl::on_next([=, &icon] {
					auto p = QPainter(arrow);
					icon.paint(p, 0, 0, arrow->width());
				}, arrow->lifetime());
			}
			arrow->setCursor(style::cur_pointer);

			const auto showPicker = activeCount > 1;
			pickerButton->setVisible(showPicker);
			arrow->setVisible(showPicker);

			rpl::combine(
				title->geometryValue(),
				container->widthValue(),
				pickerButton->naturalWidthValue()
			) | rpl::on_next([=](QRect r, int width, int natural) {
				pickerButton->resizeToNaturalWidth(width / 2);
				pickerButton->moveToRight(
					st::defaultSubsectionTitlePadding.right() + arrow->width() + st::normalFont->spacew / 2,
					r.y() + (r.height() - pickerButton->height()) / 2,
					width);
				arrow->moveToLeft(
					pickerButton->x() + pickerButton->width() + st::normalFont->spacew / 2,
					r.y() + (r.height() - arrow->height()) / 2);
			}, pickerButton->lifetime());

			std::vector checkboxes{
				NestedEntry{
					tr::solar_DontReadMessages(tr::now),
					[state] { return !SolarSettings::ghost(state->selectedUserId.current()).sendReadMessages(); },
					[state](bool v) { SolarSettings::ghost(state->selectedUserId.current()).setSendReadMessages(!v); },
					[state] { return SolarSettings::ghost(state->selectedUserId.current()).sendReadMessagesLocked(); },
					[state](bool v) { SolarSettings::ghost(state->selectedUserId.current()).setSendReadMessagesLocked(v); }
				},
				NestedEntry{
					tr::solar_DontReadStories(tr::now),
					[state] { return !SolarSettings::ghost(state->selectedUserId.current()).sendReadStories(); },
					[state](bool v) { SolarSettings::ghost(state->selectedUserId.current()).setSendReadStories(!v); },
					[state] { return SolarSettings::ghost(state->selectedUserId.current()).sendReadStoriesLocked(); },
					[state](bool v) { SolarSettings::ghost(state->selectedUserId.current()).setSendReadStoriesLocked(v); }
				},
				NestedEntry{
					tr::solar_DontSendOnlinePackets(tr::now),
					[state] { return !SolarSettings::ghost(state->selectedUserId.current()).sendOnlinePackets(); },
					[state](bool v) { SolarSettings::ghost(state->selectedUserId.current()).setSendOnlinePackets(!v); },
					[state] { return SolarSettings::ghost(state->selectedUserId.current()).sendOnlinePacketsLocked(); },
					[state](bool v) { SolarSettings::ghost(state->selectedUserId.current()).setSendOnlinePacketsLocked(v); }
				},
				NestedEntry{
					tr::solar_DontSendUploadProgress(tr::now),
					[state] { return !SolarSettings::ghost(state->selectedUserId.current()).sendUploadProgress(); },
					[state](bool v) { SolarSettings::ghost(state->selectedUserId.current()).setSendUploadProgress(!v); },
					[state] { return SolarSettings::ghost(state->selectedUserId.current()).sendUploadProgressLocked(); },
					[state](bool v) { SolarSettings::ghost(state->selectedUserId.current()).setSendUploadProgressLocked(v); }
				},
				NestedEntry{
					tr::solar_SendOfflinePacketAfterOnline(tr::now),
					[state] { return SolarSettings::ghost(state->selectedUserId.current()).sendOfflinePacketAfterOnline(); },
					[state](bool v) { SolarSettings::ghost(state->selectedUserId.current()).setSendOfflinePacketAfterOnline(v); },
					[state] { return SolarSettings::ghost(state->selectedUserId.current()).sendOfflinePacketAfterOnlineLocked(); },
					[state](bool v) { SolarSettings::ghost(state->selectedUserId.current()).setSendOfflinePacketAfterOnlineLocked(v); }
				},
			};

			auto collapsible = AddCollapsibleToggle(
				container,
				tr::solar_GhostModeToggle(),
				std::move(checkboxes),
				true,
				tr::solar_GhostModeOptionShiftDescription(tr::now));
			state->refreshCheckboxes = std::move(collapsible.refresh);
			if (wctx.highlights && collapsible.widget) {
				wctx.highlights->push_back(std::make_pair(
					u"solar/ghostModeToggle"_q,
					HighlightEntry{ collapsible.widget, {} }));
			}

			const auto markReadButton = AddButtonWithIcon(
				container,
				tr::solar_MarkReadAfterAction(),
				st::settingsButtonNoIcon
			);
			if (wctx.highlights) {
				wctx.highlights->push_back(std::make_pair(
					u"solar/markReadAfterAction"_q,
					HighlightEntry{ markReadButton.get(), {} }));
			}
			markReadButton->toggleOn(
				state->selectedUserId.value()
				| rpl::map([](uint64 id) {
					return SolarSettings::ghost(id).markReadAfterActionValue();
				}) | rpl::flatten_latest()
			)->toggledValue(
			) | rpl::filter(
				[=](bool enabled) {
					return enabled != SolarSettings::ghost(state->selectedUserId.current()).markReadAfterAction();
				}
			) | on_next(
				[=](bool enabled) {
					auto &ghost = SolarSettings::ghost(state->selectedUserId.current());
					ghost.setMarkReadAfterAction(enabled);
					if (enabled) {
						ghost.setUseScheduledMessages(false);
					}
				},
				container->lifetime());
			AddSkip(container);
			AddDividerText(container, tr::solar_MarkReadAfterActionDescription());

			AddSkip(container);
			const auto scheduleButton = AddButtonWithIcon(
				container,
				tr::solar_UseScheduledMessages(),
				st::settingsButtonNoIcon
			);
			if (wctx.highlights) {
				wctx.highlights->push_back(std::make_pair(
					u"solar/useScheduledMessages"_q,
					HighlightEntry{ scheduleButton.get(), {} }));
			}
			scheduleButton->toggleOn(
				state->selectedUserId.value()
				| rpl::map([](uint64 id) {
					return SolarSettings::ghost(id).useScheduledMessagesValue();
				}) | rpl::flatten_latest()
			)->toggledValue(
			) | rpl::filter(
				[=](bool enabled) {
					return enabled != SolarSettings::ghost(state->selectedUserId.current()).useScheduledMessages();
				}
			) | on_next(
				[=](bool enabled) {
					auto &ghost = SolarSettings::ghost(state->selectedUserId.current());
					ghost.setUseScheduledMessages(enabled);
					if (enabled) {
						ghost.setMarkReadAfterAction(false);
					}
				},
				container->lifetime());
			AddSkip(container);
			AddDividerText(container, tr::solar_UseScheduledMessagesDescription());

			AddSkip(container);
			const auto silentOptions = std::vector<QString>{
				tr::solar_SendWithoutSoundByDefaultNever(tr::now),
				tr::solar_SendWithoutSoundByDefaultInGhostMode(tr::now),
				tr::solar_SendWithoutSoundByDefaultAlways(tr::now),
			};
			const auto silentOptionText = state->selectedUserId.value(
			) | rpl::map([=](uint64 id) {
				return SolarSettings::ghost(id).sendWithoutSoundValue(
				) | rpl::map([=](SendWithoutSoundOption value) {
					return silentOptions[static_cast<int>(value)];
				});
			}) | rpl::flatten_latest();
			const auto silentButton = AddButtonWithLabel(
				container,
				tr::solar_SendWithoutSoundByDefault(),
				std::move(silentOptionText),
				st::settingsButtonNoIcon);
			if (wctx.highlights) {
				wctx.highlights->push_back(std::make_pair(
					u"solar/sendWithoutSound"_q,
					HighlightEntry{ silentButton.get(), {} }));
			}
			silentButton->addClickHandler([=] {
				controller->show(Box([=](not_null<Ui::GenericBox*> box) {
					const auto save = [=](int index) {
						SolarSettings::ghost(state->selectedUserId.current()
						).setSendWithoutSound(
							static_cast<SendWithoutSoundOption>(index));
					};
					SingleChoiceBox(box, {
						.title = tr::solar_SendWithoutSoundByDefault(),
						.options = silentOptions,
						.initialSelection = static_cast<int>(
							SolarSettings::ghost(state->selectedUserId.current()
							).sendWithoutSound()),
						.callback = save,
					});
				}));
			});
			AddSkip(container);
			AddDividerText(container, tr::solar_SendWithoutSoundByDefaultDescription());

			AddSkip(container);
			const auto suggestGhostModeButton = AddButtonWithIcon(
				container,
				tr::solar_SuggestGhostModeBeforeViewingStory(),
				st::settingsButtonNoIcon);
			if (wctx.highlights) {
				wctx.highlights->push_back(std::make_pair(
					u"solar/suggestGhostModeBeforeViewingStory"_q,
					HighlightEntry{ suggestGhostModeButton.get(), {} }));
			}
			suggestGhostModeButton->toggleOn(
				state->selectedUserId.value()
				| rpl::map([](uint64 id) {
					return SolarSettings::ghost(id).suggestGhostModeBeforeViewingStoryValue();
				}) | rpl::flatten_latest()
			)->toggledValue(
			) | rpl::filter(
				[=](bool enabled) {
					return enabled != SolarSettings::ghost(state->selectedUserId.current()).suggestGhostModeBeforeViewingStory();
				}
			) | on_next(
				[=](bool enabled) {
					SolarSettings::ghost(state->selectedUserId.current()).setSuggestGhostModeBeforeViewingStory(enabled);
				},
				container->lifetime());
			AddSkip(container);
			AddDividerText(container, tr::solar_SuggestGhostModeBeforeViewingStoryDescription());

			AddSkip(container);
			const auto onlineBlinkButton = AddButtonWithIcon(
				container,
				tr::solar_OnlineBlink(),
				st::settingsButtonNoIcon);
			if (wctx.highlights) {
				wctx.highlights->push_back(std::make_pair(
					u"solar/onlineBlink"_q,
					HighlightEntry{ onlineBlinkButton.get(), {} }));
			}
			onlineBlinkButton->toggleOn(
				state->selectedUserId.value()
				| rpl::map([](uint64 id) {
					return SolarSettings::ghost(id).onlineBlinkValue();
				}) | rpl::flatten_latest()
			)->toggledValue(
			) | rpl::filter(
				[=](bool enabled) {
					return enabled != SolarSettings::ghost(state->selectedUserId.current()).onlineBlink();
				}
			) | on_next(
				[=](bool enabled) {
					SolarSettings::ghost(state->selectedUserId.current()).setOnlineBlink(enabled);
				},
				container->lifetime());
			AddSkip(container);
			AddDividerText(container, tr::solar_OnlineBlinkAbout());

			auto showMenu = [=] {
				state->menu = base::make_unique_q<Ui::PopupMenu>(
					pickerButton,
					st::defaultPopupMenu);

				state->menu->addAction(
					base::make_unique_q<GlobalAction>(
						state->menu->menu(),
						st::defaultPopupMenu.menu,
						tr::solar_GhostModeGlobalSettings(tr::now),
						state->selectedUserId.current() == 0,
						[=] { selectGhostProfile(state, 0); }));

				for (const auto &account : Core::App().domain().orderedAccounts()) {
					if (!account->sessionExists()) {
						continue;
					}
					auto user = account->session().user();
					auto id = account->session().userId().bare;
					state->menu->addAction(
						base::make_unique_q<AccountAction>(
							state->menu->menu(),
							st::defaultPopupMenu.menu,
							user,
							state->selectedUserId.current() == id,
							[=] { selectGhostProfile(state, id); }));
				}

				state->menu->popup(
					pickerButton->mapToGlobal(
						QPoint(pickerButton->width(), pickerButton->height())));
			};
			pickerButton->setClickedCallback(showMenu);
			arrow->setClickedCallback(showMenu);
		}, [&](const SearchContext &sctx) {
			sctx.entries->push_back({
				.id = u"solar/ghostModeToggle"_q,
				.title = tr::solar_GhostModeToggle(tr::now),
				.section = sctx.sectionId,
			});
			sctx.entries->push_back({
				.id = u"solar/markReadAfterAction"_q,
				.title = tr::solar_MarkReadAfterAction(tr::now),
				.section = sctx.sectionId,
			});
			sctx.entries->push_back({
				.id = u"solar/useScheduledMessages"_q,
				.title = tr::solar_UseScheduledMessages(tr::now),
				.section = sctx.sectionId,
			});
			sctx.entries->push_back({
				.id = u"solar/sendWithoutSound"_q,
				.title = tr::solar_SendWithoutSoundByDefault(tr::now),
				.section = sctx.sectionId,
			});
			sctx.entries->push_back({
				.id = u"solar/suggestGhostModeBeforeViewingStory"_q,
				.title = tr::solar_SuggestGhostModeBeforeViewingStory(tr::now),
				.section = sctx.sectionId,
			});
			sctx.entries->push_back({
				.id = u"solar/onlineBlink"_q,
				.title = tr::solar_OnlineBlink(tr::now),
				.section = sctx.sectionId,
			});
		});
	});
}

void BuildGhostDisplay(SectionBuilder &builder, SolarSectionBuilder &solar) {
	const auto controller = builder.controller();
	solar.addSettingToggle({
		.id = u"solar/displayGhostStatus"_q,
		.title = tr::solar_DisplayGhostStatus(),
		.getter = &SolarSettings::displayGhostStatus,
		.setter = &SolarSettings::setDisplayGhostStatus,
	});
	builder.addButton({
		.id = u"solar/ghostModeHotkey"_q,
		.title = tr::solar_GhostModeHotkey(),
		.st = &st::settingsButtonNoIcon,
		.label = SolarSettings::getInstance().ghostModeShortcutValue(),
		.onClick = [=] {
			if (!controller) {
				return;
			}
			controller->show(Box<EditMarkBox>(
				tr::solar_GhostModeHotkey(),
				SolarSettings::getInstance().ghostModeShortcut(),
				u"Ctrl+Shift+G"_q,
				[](const QString &value) {
					SolarSettings::getInstance().setGhostModeShortcut(value.trimmed());
				}));
		},
	});
}

void BuildSpyEssentials(SectionBuilder &builder, SolarSectionBuilder &solar) {
	const auto controller = builder.controller();
	auto *settings = &SolarSettings::getInstance();

	builder.addSubsectionTitle(tr::solar_SpyEssentialsHeader());

	solar.addChooseButton({
		.id = u"solar/deletedSaveMode"_q,
		.altIds = { u"solar/saveDeletedMessages"_q },
		.title = tr::solar_DeletedSaveMode(),
		.boxTitle = tr::solar_DeletedSaveMode(),
		.initialSelection = static_cast<int>(settings->deletedSaveMode()),
		.options = {
			tr::solar_DeletedSaveModeOff(tr::now),
			tr::solar_DeletedSaveModeSession(tr::now),
			tr::solar_DeletedSaveModePersistent(tr::now),
		},
		.setter = [](int index) {
			SolarSettings::getInstance().setDeletedSaveMode(
				static_cast<DeletedSaveMode>(index));
		},
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_DeletedSaveModeAbout());
	builder.addSkip();

	solar.addSettingToggle({
		.id = u"solar/saveMessagesHistory"_q,
		.title = tr::solar_SaveMessagesHistory(),
		.getter = &SolarSettings::saveMessagesHistory,
		.setter = &SolarSettings::setSaveMessagesHistory,
	});

	solar.addCollapsibleToggle({
		.id = u"solar/saveMedia"_q,
		.altIds = { u"solar/saveForBots"_q },
		.title = tr::solar_SaveMedia(),
		.checkboxes = {
			NestedEntry{
				tr::solar_SaveMediaPrivate(tr::now),
				[] { return SolarSettings::getInstance().saveMediaPrivate(); },
				[](bool v) { SolarSettings::getInstance().setSaveMediaPrivate(v); },
			},
			NestedEntry{
				tr::solar_SaveMediaGroups(tr::now),
				[] { return SolarSettings::getInstance().saveMediaGroups(); },
				[](bool v) { SolarSettings::getInstance().setSaveMediaGroups(v); },
			},
			NestedEntry{
				tr::solar_SaveMediaChannels(tr::now),
				[] { return SolarSettings::getInstance().saveMediaChannels(); },
				[](bool v) { SolarSettings::getInstance().setSaveMediaChannels(v); },
			},
			NestedEntry{
				tr::solar_SaveMediaBots(tr::now),
				[] { return SolarSettings::getInstance().saveMediaBots(); },
				[](bool v) { SolarSettings::getInstance().setSaveMediaBots(v); },
			},
			NestedEntry{
				tr::solar_SaveMediaStickers(tr::now),
				[] { return SolarSettings::getInstance().saveMediaStickers(); },
				[](bool v) { SolarSettings::getInstance().setSaveMediaStickers(v); },
			},
		},
		.toggledWhenAll = false,
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_SaveMediaAbout());
	builder.addSkip();

	solar.addSlider({
		.id = u"solar/attachmentsMaxSize"_q,
		.title = tr::solar_AttachmentsMaxSize(),
		.steps = 20,
		.current = std::clamp((settings->attachmentsMaxSizeMb() / 512) - 1, 0, 19),
		.indexToValue = [](int index) { return (index + 1) * 512; },
		.onChanged = nullptr,
		.onFinalChanged = [](int mb) {
			SolarSettings::getInstance().setAttachmentsMaxSizeMb(mb);
		},
		.formatLabel = [](int mb) {
			if (mb < 1024) {
				return u"%1 MB"_q.arg(mb);
			}
			const auto gb = mb / 1024.0;
			return u"%1 %2"_q.arg(
				QString::number(gb, 'f', (mb % 1024 == 0) ? 0 : 1),
				tr::solar_AttachmentsMaxSizeGb(tr::now));
		},
	});

	solar.addSettingToggle({
		.id = u"solar/keepOneViewMedia"_q,
		.title = tr::solar_KeepOneViewMedia(),
		.getter = &SolarSettings::keepOneViewMedia,
		.setter = &SolarSettings::setKeepOneViewMedia,
	});
	solar.addSettingToggle({
		.id = u"solar/spySaveReadMarks"_q,
		.title = tr::solar_SpySaveReadMarks(),
		.getter = &SolarSettings::spySaveReadMarks,
		.setter = &SolarSettings::setSpySaveReadMarks,
	});
	solar.addSettingToggle({
		.id = u"solar/spySaveLocalOnline"_q,
		.title = tr::solar_SpySaveLocalOnline(),
		.getter = &SolarSettings::spySaveLocalOnline,
		.setter = &SolarSettings::setSpySaveLocalOnline,
	});

	builder.addButton({
		.id = u"solar/openAttachmentsFolder"_q,
		.title = tr::solar_OpenAttachmentsFolder(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [] {
			const auto folder = SolarMessages::getAttachmentsFolderPath();
			QDir().mkpath(folder);
			QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
		},
	});
	builder.addButton({
		.id = u"solar/attachmentsClear"_q,
		.title = tr::solar_AttachmentsClear(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [weak = base::make_weak(controller)] {
			SolarMessages::clearAttachmentsFolder();
			if (const auto c = weak.get()) {
				c->showToast(tr::solar_AttachmentsCleared(tr::now));
			}
		},
	});
	builder.addButton({
		.id = u"solar/clearDeletedMessages"_q,
		.title = tr::solar_ClearDeletedMessages(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [weak = base::make_weak(controller)] {
			const auto c = weak.get();
			if (!c) {
				return;
			}
			c->show(Ui::MakeConfirmBox({
				.text = tr::solar_ClearDeletedConfirm(tr::now),
				.confirmed = [weak](Fn<void()> &&close) {
					SolarMessages::clearAllDeletedMessages();
					close();
					if (const auto c2 = weak.get()) {
						c2->showToast(tr::solar_DeletedMessagesCleared(tr::now));
					}
				},
				.confirmText = tr::lng_box_yes(),
				.cancelText = tr::lng_cancel(),
				.confirmStyle = &st::attentionBoxButton,
			}));
		},
	});
}

void BuildPartisanSecurity(SectionBuilder &builder, SolarSectionBuilder &solar) {
	const auto controller = builder.controller();
	auto *settings = &SolarSettings::getInstance();

	builder.addSubsectionTitle(tr::solar_PartisanTitle());

	builder.addButton({
		.id = u"solar/duressPin"_q,
		.title = tr::solar_DuressPin(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [weak = base::make_weak(controller)] {
			const auto c = weak.get();
			if (!c) {
				return;
			}
			ShowSolarPinBox(
				c,
				tr::solar_DuressSetPin(),
				tr::solar_DuressPinPlaceholder(),
				std::nullopt,
				SolarSettings::getInstance().hasDuressPin(),
				[weak](const QString &pin) {
					SolarSettings::getInstance().setDuressPin(pin);
					if (const auto c2 = weak.get()) {
						c2->showToast(tr::lng_box_done(tr::now));
					}
				},
				[weak] {
					SolarSettings::getInstance().clearDuressPin();
					if (const auto c2 = weak.get()) {
						c2->showToast(tr::lng_box_done(tr::now));
					}
				});
		},
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_DuressPinAbout());
	builder.addSkip();

	solar.addChooseButton({
		.id = u"solar/duressAction"_q,
		.title = tr::solar_DuressAction(),
		.boxTitle = tr::solar_DuressAction(),
		.initialSelection = static_cast<int>(settings->duressAction()),
		.options = {
			tr::solar_DuressActionDecoy(tr::now),
			tr::solar_DuressActionWipe(tr::now),
		},
		.setter = [](int index) {
			SolarSettings::getInstance().setDuressAction(
				static_cast<DuressAction>(index));
		},
	});

	builder.addButton({
		.id = u"solar/hiddenChatsPin"_q,
		.title = tr::solar_HiddenChatsTitle(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [weak = base::make_weak(controller)] {
			const auto c = weak.get();
			if (!c) {
				return;
			}
			ShowSolarPinBox(
				c,
				tr::solar_HiddenChatsTitle(),
				tr::solar_DuressPinPlaceholder(),
				std::nullopt,
				SolarSettings::getInstance().hasHiddenChatsPin(),
				[weak](const QString &pin) {
					SolarSettings::getInstance().setHiddenChatsPin(pin);
					if (const auto c2 = weak.get()) {
						c2->showToast(tr::lng_box_done(tr::now));
					}
				},
				[weak] {
					SolarSettings::getInstance().clearHiddenChatsPin();
					if (const auto c2 = weak.get()) {
						c2->showToast(tr::lng_box_done(tr::now));
					}
				});
		},
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_HiddenChatsAbout());
	builder.addSkip();

	builder.addButton({
		.id = u"solar/panicWipe"_q,
		.title = tr::solar_PanicWipe(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			if (!controller) {
				return;
			}
			controller->show(Ui::MakeConfirmBox({
				.text = tr::solar_PanicWipeConfirm(tr::now),
				.confirmed = [](Fn<void()> &&close) {
					close();
					SolarHotkeys::TriggerPanicWipe();
				},
				.confirmText = tr::lng_box_yes(),
				.cancelText = tr::lng_cancel(),
				.confirmStyle = &st::attentionBoxButton,
			}));
		},
	});

	solar.addSettingToggle({
		.id = u"solar/globalHotkeys"_q,
		.title = tr::solar_GlobalHotkeys(),
		.getter = &SolarSettings::globalHotkeysEnabled,
		.setter = &SolarSettings::setGlobalHotkeysEnabled,
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_GlobalHotkeysAbout());
	builder.addSkip();

	solar.addSettingToggle({
		.id = u"solar/presenceLogger"_q,
		.title = tr::solar_PresenceLogger(),
		.getter = &SolarSettings::presenceLoggerEnabled,
		.setter = &SolarSettings::setPresenceLoggerEnabled,
	});
	solar.addSettingToggle({
		.id = u"solar/rememberLastSeen"_q,
		.title = tr::solar_RememberLastSeen(),
		.getter = &SolarSettings::rememberLastSeen,
		.setter = &SolarSettings::setRememberLastSeen,
	});
	solar.addSettingToggle({
		.id = u"solar/presenceOnlineAlerts"_q,
		.title = tr::solar_PresenceOnlineAlerts(),
		.getter = &SolarSettings::presenceOnlineAlerts,
		.setter = &SolarSettings::setPresenceOnlineAlerts,
	});
	solar.addSettingToggle({
		.id = u"solar/phishingGuard"_q,
		.title = tr::solar_PhishingGuard(),
		.getter = &SolarSettings::phishingGuard,
		.setter = &SolarSettings::setPhishingGuard,
	});
	solar.addSettingToggle({
		.id = u"solar/confirmExternalLinks"_q,
		.title = tr::solar_ConfirmExternalLinks(),
		.getter = &SolarSettings::confirmExternalLinks,
		.setter = &SolarSettings::setConfirmExternalLinks,
	});
	solar.addSettingToggle({
		.id = u"solar/blockExternalLookups"_q,
		.title = tr::solar_BlockExternalLookups(),
		.getter = &SolarSettings::blockExternalLookups,
		.setter = &SolarSettings::setBlockExternalLookups,
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_BlockExternalLookupsAbout());
	builder.addSkip();

	solar.addSettingToggle({
		.id = u"solar/deviceSpoof"_q,
		.title = tr::solar_DeviceSpoof(),
		.getter = &SolarSettings::deviceSpoofEnabled,
		.setter = &SolarSettings::setDeviceSpoofEnabled,
	});
	builder.addButton({
		.id = u"solar/deviceSpoofModel"_q,
		.title = tr::solar_DeviceSpoofModel(),
		.st = &st::settingsButtonNoIcon,
		.label = settings->deviceSpoofModelValue(),
		.onClick = [=] {
			if (!controller) {
				return;
			}
			controller->show(Box<EditMarkBox>(
				tr::solar_DeviceSpoofModel(),
				SolarSettings::getInstance().deviceSpoofModel(),
				u"SolarGram Workstation"_q,
				[](const QString &value) {
					SolarSettings::getInstance().setDeviceSpoofModel(value.trimmed());
				}));
		},
	});
	builder.addButton({
		.id = u"solar/deviceSpoofSystem"_q,
		.title = tr::solar_DeviceSpoofSystem(),
		.st = &st::settingsButtonNoIcon,
		.label = settings->deviceSpoofSystemValue(),
		.onClick = [=] {
			if (!controller) {
				return;
			}
			controller->show(Box<EditMarkBox>(
				tr::solar_DeviceSpoofSystem(),
				SolarSettings::getInstance().deviceSpoofSystem(),
				u"Windows 11"_q,
				[](const QString &value) {
					SolarSettings::getInstance().setDeviceSpoofSystem(value.trimmed());
				}));
		},
	});
	builder.addButton({
		.id = u"solar/deviceSpoofAppVersion"_q,
		.title = tr::solar_DeviceSpoofAppVersion(),
		.st = &st::settingsButtonNoIcon,
		.label = settings->deviceSpoofAppVersionValue(),
		.onClick = [=] {
			if (!controller) {
				return;
			}
			controller->show(Box<EditMarkBox>(
				tr::solar_DeviceSpoofAppVersion(),
				SolarSettings::getInstance().deviceSpoofAppVersion(),
				u"5.10.3"_q,
				[](const QString &value) {
					SolarSettings::getInstance().setDeviceSpoofAppVersion(value.trimmed());
				}));
		},
	});
	builder.addButton({
		.id = u"solar/deviceSpoofLangCode"_q,
		.title = tr::solar_DeviceSpoofLangCode(),
		.st = &st::settingsButtonNoIcon,
		.label = settings->deviceSpoofLangCodeValue(),
		.onClick = [=] {
			if (!controller) {
				return;
			}
			controller->show(Box<EditMarkBox>(
				tr::solar_DeviceSpoofLangCode(),
				SolarSettings::getInstance().deviceSpoofLangCode(),
				u"en"_q,
				[](const QString &value) {
					SolarSettings::getInstance().setDeviceSpoofLangCode(value.trimmed());
				}));
		},
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_DeviceSpoofAbout());
	builder.addSkip();

	builder.addButton({
		.id = u"solar/exportBackup"_q,
		.title = tr::solar_ExportBackup(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			if (!controller) {
				return;
			}
			const auto weak = base::make_weak(controller);
			FileDialog::GetWritePath(
				controller->widget(),
				tr::solar_ExportBackup(tr::now),
				u"JSON Files (*.json)"_q,
				u"solargram_backup.json"_q,
				[=](QString &&path) {
					if (path.isEmpty()) {
						return;
					}
					const auto ok = SolarDatabase::exportBackupJson(
						path,
						SolarSettings::getInstance().exportToJsonObject());
					if (const auto c = weak.get()) {
						c->showToast(ok
							? tr::solar_BackupExported(tr::now)
							: tr::solar_BackupFailed(tr::now));
					}
				});
		},
	});
	builder.addButton({
		.id = u"solar/importBackup"_q,
		.title = tr::solar_ImportBackup(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			if (!controller) {
				return;
			}
			const auto weak = base::make_weak(controller);
			FileDialog::GetOpenPath(
				controller->widget(),
				tr::solar_ImportBackup(tr::now),
				u"JSON Files (*.json)"_q,
				[=](FileDialog::OpenResult &&result) {
					if (result.paths.isEmpty()) {
						return;
					}
					const auto c = weak.get();
					if (!c) {
						return;
					}
					const auto path = result.paths.front();
					c->show(Box([=](not_null<Ui::GenericBox*> box) {
						SingleChoiceBox(box, {
							.title = tr::solar_ImportBackupMode(),
							.options = {
								tr::solar_ImportMerge(tr::now),
								tr::solar_ImportReplace(tr::now),
							},
							.initialSelection = 0,
							.callback = [=](int index) {
								const auto replace = (index == 1);
								const auto imported = SolarDatabase::importBackupJson(
									path,
									replace);
								if (const auto c2 = weak.get()) {
									if (imported.has_value()) {
										SolarSettings::getInstance().importFromJsonObject(*imported);
										c2->showToast(tr::solar_BackupImported(tr::now));
									} else {
										c2->showToast(tr::solar_BackupFailed(tr::now));
									}
								}
							},
						});
					}));
				});
		},
	});
}

void BuildOther(SectionBuilder &builder, SolarSectionBuilder &solar) {
	builder.addSubsectionTitle(tr::solar_MessageSavingOtherHeader());

	solar.addSettingToggle({
		.id = u"solar/eternalOnline"_q,
		.title = tr::solar_EternalOnline(),
		.getter = &SolarSettings::eternalOnline,
		.setter = &SolarSettings::setEternalOnline,
	});
	solar.addSettingToggle({
		.id = u"solar/autoStreamerMode"_q,
		.title = tr::solar_AutoStreamerMode(),
		.getter = &SolarSettings::autoStreamerMode,
		.setter = &SolarSettings::setAutoStreamerMode,
	});
	solar.addSettingToggle({
		.id = u"solar/localPremium"_q,
		.title = tr::solar_LocalPremium(),
		.getter = &SolarSettings::localPremium,
		.setter = &SolarSettings::setLocalPremium,
	});
	solar.addSettingToggle({
		.id = u"solar/disableAds"_q,
		.title = tr::solar_DisableAds(),
		.getter = &SolarSettings::disableAds,
		.setter = &SolarSettings::setDisableAds,
	});

	const auto controller = builder.controller();
	auto *settings = &SolarSettings::getInstance();

	solar.addSectionDivider();
	builder.addSubsectionTitle(tr::solar_ActivityRadarTitle());

	solar.addSettingToggle({
		.id = u"solar/storyAutoHide"_q,
		.title = tr::solar_StoryAutoHide(),
		.getter = &SolarSettings::storyAutoHideEnabled,
		.setter = &SolarSettings::setStoryAutoHideEnabled,
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_StoryAutoHideAbout());
	builder.addSkip();

	builder.addButton({
		.id = u"solar/storyBlacklistManage"_q,
		.title = tr::solar_StoryBlacklistManage(),
		.st = &st::settingsButtonNoIcon,
		.label = rpl::single(0) | rpl::then(
			settings->storyPrivacyVersionChanges()
		) | rpl::map([](int) {
			return QString::number(SolarSettings::getInstance().storyHiddenUserIds().size());
		}),
		.onClick = [=] {
			if (controller) {
				SolarFeatures::Guards::ShowBlacklistManagerBox(
					controller,
					SolarFeatures::Guards::BlacklistKind::StoryHide);
			}
		},
	});
	builder.addButton({
		.id = u"solar/applyStoryHideNow"_q,
		.title = tr::solar_ApplyStoryHideNow(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [weak = base::make_weak(controller)] {
			if (const auto c = weak.get()) {
				const auto count = SolarFeatures::Guards::ApplyStoryHideToAllSelfStories(&c->session());
				c->showToast(tr::solar_StoryHideAppliedToast(tr::now).arg(count));
			}
		},
	});
	builder.addButton({
		.id = u"solar/storyFeedHiddenManage"_q,
		.title = tr::solar_StoryFeedHiddenManage(),
		.st = &st::settingsButtonNoIcon,
		.label = rpl::single(0) | rpl::then(
			settings->storyFeedHiddenVersionChanges()
		) | rpl::map([](int) {
			return QString::number(SolarSettings::getInstance().storyFeedHiddenPeerIds().size());
		}),
		.onClick = [=] {
			if (controller) {
				SolarFeatures::Guards::ShowBlacklistManagerBox(
					controller,
					SolarFeatures::Guards::BlacklistKind::StoryFeedHide);
			}
		},
	});

	solar.addSettingToggle({
		.id = u"solar/channelAutoBan"_q,
		.title = tr::solar_ChannelAutoBan(),
		.getter = &SolarSettings::channelAutoBanEnabled,
		.setter = &SolarSettings::setChannelAutoBanEnabled,
	});
	builder.addSkip();
	builder.addDividerText(tr::solar_ChannelAutoBanAbout());
	builder.addSkip();

	builder.addButton({
		.id = u"solar/channelBlacklistManage"_q,
		.title = tr::solar_ChannelBlacklistManage(),
		.st = &st::settingsButtonNoIcon,
		.label = rpl::single(0) | rpl::then(
			settings->channelBlockedVersionChanges()
		) | rpl::map([](int) {
			return QString::number(SolarSettings::getInstance().channelBlockedUserIds().size());
		}),
		.onClick = [=] {
			if (controller) {
				SolarFeatures::Guards::ShowBlacklistManagerBox(
					controller,
					SolarFeatures::Guards::BlacklistKind::ChannelAutoBan);
			}
		},
	});
	builder.addButton({
		.id = u"solar/syncChannelBansNow"_q,
		.title = tr::solar_SyncChannelBansNow(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [weak = base::make_weak(controller)] {
			if (const auto c = weak.get()) {
				const auto count = SolarFeatures::Guards::SyncChannelBlacklists(&c->session());
				c->showToast(tr::solar_ChannelBansSyncedToast(tr::now).arg(count));
			}
		},
	});
}

const auto kMeta = BuildHelper({
	.id = SolarGhost::Id(),
	.parentId = SolarMain::Id(),
	.title = u"SolarGram"_q,
	.icon = &st::menuIconGroupReactions,
}, [](SectionBuilder &builder) {
	auto solar = SolarSectionBuilder(builder);

	builder.addSkip();
	BuildGhostEssentials(builder);
	BuildGhostDisplay(builder, solar);

	builder.addSkip();
	BuildSpyEssentials(builder, solar);

	solar.addSectionDivider();
	BuildPartisanSecurity(builder, solar);

	solar.addSectionDivider();
	BuildOther(builder, solar);
	builder.addSkip();
});

} // namespace

rpl::producer<QString> SolarGhost::title() {
	return rpl::single(QString("SolarGram"));
}

SolarGhost::SolarGhost(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller)
, _controller(controller) {
	setupContent();
}

void SolarGhost::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type SolarGhostId() {
	return SolarGhost::Id();
}

} // namespace Settings
