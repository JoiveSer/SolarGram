// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/ui/settings/settings_chats.h"

#include "lang_auto.h"
#include "solar/solar_settings.h"
#include "solar/ui/boxes/edit_mark_box.h"
#include "solar/ui/components/message_preview.h"
#include "solar/ui/settings/solar_builder.h"
#include "solar/ui/settings/settings_solar_utils.h"
#include "solar/ui/settings/settings_main.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_solar_icons.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

#include <memory>

namespace Settings {

using namespace Builder;
using namespace SolarBuilder;

namespace {

struct PreviewState {
	MessagePreview *widget = nullptr;
};

void BuildStickersAndEmoji(SectionBuilder &builder, SolarSectionBuilder &solar) {
	builder.addSubsectionTitle(tr::lng_settings_stickers_emoji());

	solar.addSettingToggle({
		.id = u"solar/showOnlyAddedEmojisAndStickers"_q,
		.title = tr::solar_ShowOnlyAddedEmojisAndStickers(),
		.getter = &SolarSettings::showOnlyAddedEmojisAndStickers,
		.setter = &SolarSettings::setShowOnlyAddedEmojisAndStickers,
	});

	solar.addSettingToggle({
		.id = u"solar/unlimitedRecentStickers"_q,
		.altIds = { u"solar/recentStickersCount"_q },
		.title = tr::solar_SettingsUnlimitedRecentStickers(),
		.getter = &SolarSettings::unlimitedRecentStickers,
		.setter = &SolarSettings::setUnlimitedRecentStickers,
	});

	solar.addCollapsibleToggle({
		.id = u"solar/hideReactions"_q,
		.title = tr::solar_HideReactions(),
		.checkboxes = {
			NestedEntry{
				tr::solar_HideReactionsInChannels(tr::now),
				[] { return !SolarSettings::getInstance().showChannelReactions(); },
				[](bool v) { SolarSettings::getInstance().setShowChannelReactions(!v); }
			},
			NestedEntry{
				tr::solar_HideReactionsInGroups(tr::now),
				[] { return !SolarSettings::getInstance().showGroupReactions(); },
				[](bool v) { SolarSettings::getInstance().setShowGroupReactions(!v); }
			},
			NestedEntry{
				tr::solar_HideReactionsInPrivateChats(tr::now),
				[] { return !SolarSettings::getInstance().showPrivateChatReactions(); },
				[](bool v) { SolarSettings::getInstance().setShowPrivateChatReactions(!v); }
			}
		},
		.toggledWhenAll = false,
	});

	solar.addSectionDivider();
}

void BuildGroupsAndChannels(SectionBuilder &builder, SolarSectionBuilder &solar) {
	auto *settings = &SolarSettings::getInstance();

	builder.addSubsectionTitle(tr::lng_premium_double_limits_subtitle_channels());

	solar.addChooseButton({
		.id = u"solar/channelBottomButton"_q,
		.altIds = { u"solar/bottomButton"_q },
		.title = tr::solar_ChannelBottomButton(),
		.boxTitle = tr::solar_ChannelBottomButton(),
		.initialSelection = static_cast<int>(settings->channelBottomButton()),
		.options = {
			tr::solar_ChannelBottomButtonHide(tr::now),
			tr::solar_ChannelBottomButtonMute(tr::now),
			tr::solar_ChannelBottomButtonDiscuss(tr::now),
		},
		.setter = [](int index) {
			SolarSettings::getInstance().setChannelBottomButton(
				static_cast<ChannelBottomButton>(index));
		},
	});

	solar.addSettingToggle({
		.id = u"solar/quickAdminShortcuts"_q,
		.title = tr::solar_QuickAdminShortcuts(),
		.getter = &SolarSettings::quickAdminShortcuts,
		.setter = &SolarSettings::setQuickAdminShortcuts,
	});
	solar.addSettingToggle({
		.id = u"solar/disableGreetingSticker"_q,
		.title = tr::solar_DisableGreetingSticker(),
		.getter = &SolarSettings::disableGreetingSticker,
		.setter = &SolarSettings::setDisableGreetingSticker,
	});
	solar.addSettingToggle({
		.id = u"solar/showMessageShot"_q,
		.title = tr::solar_SettingsShowMessageShot(),
		.getter = &SolarSettings::showMessageShot,
		.setter = &SolarSettings::setShowMessageShot,
	});

	builder.addSkip();
	builder.addDividerText(tr::solar_SettingsShowMessageShotDescription());
	builder.addSkip();
}

void BuildMarks(
		SectionBuilder &builder,
		SolarSectionBuilder &solar,
		std::shared_ptr<PreviewState> previewState) {
	auto *settings = &SolarSettings::getInstance();
	const auto controller = builder.controller();

	builder.addSubsectionTitle(tr::lng_settings_messages());

	builder.add([=](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		auto preview = object_ptr<MessagePreview>(ctx.container, controller);
		previewState->widget = preview.data();
		return {
			.widget = std::move(preview),
			.margin = style::margins(
				0,
				st::defaultVerticalListSkip,
				0,
				st::settingsPrivacySkipTop),
		};
	});

	solar.addSettingToggle({
		.id = u"solar/replaceBottomInfoWithIcons"_q,
		.altIds = { u"solar/replaceEditedWithIcon"_q },
		.title = tr::solar_ReplaceMarksWithIcons(),
		.getter = &SolarSettings::replaceBottomInfoWithIcons,
		.setter = &SolarSettings::setReplaceBottomInfoWithIcons,
	});

	builder.scope([&] {
		builder.addButton({
			.id = u"solar/deletedMark"_q,
			.title = tr::solar_DeletedMarkText(),
			.st = &st::settingsButtonNoIcon,
			.label = SolarSettings::getInstance().deletedMarkValue(),
			.onClick = [=] {
				auto box = Box<EditMarkBox>(
					tr::solar_DeletedMarkText(),
					settings->deletedMark(),
					QString("🧹"),
					[=](const QString &value) {
						SolarSettings::getInstance().setDeletedMark(value);
					});
				Ui::show(std::move(box));
			},
		});

		builder.addButton({
			.id = u"solar/editedMark"_q,
			.title = tr::solar_EditedMarkText(),
			.st = &st::settingsButtonNoIcon,
			.label = SolarSettings::getInstance().editedMarkValue(),
			.onClick = [=] {
				auto box = Box<EditMarkBox>(
					tr::solar_EditedMarkText(),
					settings->editedMark(),
					tr::lng_edited(tr::now),
					[=](const QString &value) {
						SolarSettings::getInstance().setEditedMark(value);
					});
				Ui::show(std::move(box));
			},
		});
	}, SolarSettings::getInstance().replaceBottomInfoWithIconsValue()
		| rpl::map([](bool v) { return !v; }));

	solar.addSettingToggle({
		.id = u"solar/removeMessageTail"_q,
		.title = tr::solar_RemoveMessageTail(),
		.getter = &SolarSettings::removeMessageTail,
		.setter = &SolarSettings::setRemoveMessageTail,
	});

	solar.addSettingToggle({
		.id = u"solar/hideFastShare"_q,
		.altIds = { u"solar/hideShareButton"_q },
		.title = tr::solar_HideShareButton(),
		.getter = &SolarSettings::hideFastShare,
		.setter = &SolarSettings::setHideFastShare,
	});
	solar.addSettingToggle({
		.id = u"solar/simpleQuotesAndReplies"_q,
		.altIds = { u"solar/disableColorfulReplies"_q, u"solar/replyElements"_q },
		.title = tr::solar_SimpleQuotesAndReplies(),
		.getter = &SolarSettings::simpleQuotesAndReplies,
		.setter = &SolarSettings::setSimpleQuotesAndReplies,
	});

	const auto semiTransparent = solar.addSettingToggle({
		.id = u"solar/semiTransparentDeletedMessages"_q,
		.altIds = { u"solar/translucentDeletedMessages"_q },
		.title = tr::solar_SemiTransparentDeletedMessages(),
		.getter = &SolarSettings::semiTransparentDeletedMessages,
		.setter = &SolarSettings::setSemiTransparentDeletedMessages,
	});
	if (semiTransparent) {
		solar.addBetaBadge(semiTransparent);
	}

	solar.addSectionDivider();
}

void BuildWideMessagesMultiplier(
		SectionBuilder &builder,
		SolarSectionBuilder &solar,
		std::shared_ptr<PreviewState> previewState) {
	auto *settings = &SolarSettings::getInstance();

	constexpr auto kMinSize = 1.00;
	constexpr auto kStep = 0.05;

	const auto valueToIndex = [=](double value) {
		return static_cast<int>(std::round((value - kMinSize) / kStep));
	};

	const auto controller = builder.controller();
	solar.addSlider({
		.id = u"solar/messageBubbleRadius"_q,
		.title = tr::solar_MessageBubbleRadius(),
		.steps = 17,
		.current = settings->messageBubbleRadius(),
		.indexToValue = [](int index) { return index; },
		.onChanged = [=](int index) {
			if (previewState->widget) {
				previewState->widget->setBubbleRadius(index);
			}
		},
		.onFinalChanged = [=](int index) {
			if (previewState->widget) {
				previewState->widget->setBubbleRadius(index);
			}
			SolarSettings::getInstance().setMessageBubbleRadius(index);
			ShowRestartPrompt(controller);
		},
		.formatLabel = [](int index) {
			return QString::number(index);
		},
	});

	solar.addSectionDivider();

	solar.addSlider({
		.id = u"solar/wideMultiplier"_q,
		.title = tr::solar_SettingsWideMultiplier(),
		.steps = 61, // (4.00 - 1.00) / 0.05 + 1
		.current = valueToIndex(settings->wideMultiplier()),
		.indexToValue = [](int index) { return index; },
		.onChanged = nullptr,
		.onFinalChanged = [=](int index) {
			SolarSettings::getInstance().setWideMultiplier(
				kMinSize + index * kStep);
			ShowRestartPrompt(controller);
		},
		.formatLabel = [=](int index) {
			return QString::number(kMinSize + index * kStep, 'f', 2);
		},
	});

	builder.addSkip();
	builder.addDividerText(tr::solar_SettingsWideMultiplierDescription());
	builder.addSkip();
}

void BuildContextMenuElements(SectionBuilder &builder, SolarSectionBuilder &solar) {
	auto *settings = &SolarSettings::getInstance();

	builder.addSubsectionTitle(tr::solar_ContextMenuElementsHeader());

	const auto options = std::vector{
		tr::solar_SettingsContextMenuItemHidden(tr::now),
		tr::solar_SettingsContextMenuItemShown(tr::now),
		tr::solar_SettingsContextMenuItemExtended(tr::now),
	};

	solar.addChooseButton({
		.id = u"solar/showReactionsPanelInContextMenu"_q,
		.title = tr::solar_SettingsContextMenuReactionsPanel(),
		.boxTitle = tr::solar_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showReactionsPanelInContextMenu()),
		.options = options,
		.setter = [](int i) { SolarSettings::getInstance().setShowReactionsPanelInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconReactions },
	});
	solar.addChooseButton({
		.id = u"solar/showViewsPanelInContextMenu"_q,
		.title = tr::solar_SettingsContextMenuViewsPanel(),
		.boxTitle = tr::solar_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showViewsPanelInContextMenu()),
		.options = options,
		.setter = [](int i) { SolarSettings::getInstance().setShowViewsPanelInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconShowInChat },
	});
	solar.addChooseButton({
		.id = u"solar/showHideMessageInContextMenu"_q,
		.title = tr::solar_ContextHideMessage(),
		.boxTitle = tr::solar_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showHideMessageInContextMenu()),
		.options = options,
		.setter = [](int i) { SolarSettings::getInstance().setShowHideMessageInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconClear },
	});
	solar.addChooseButton({
		.id = u"solar/showUserMessagesInContextMenu"_q,
		.title = tr::solar_UserMessagesMenuText(),
		.boxTitle = tr::solar_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showUserMessagesInContextMenu()),
		.options = options,
		.setter = [](int i) { SolarSettings::getInstance().setShowUserMessagesInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconTTL },
	});
	solar.addChooseButton({
		.id = u"solar/showMessageDetailsInContextMenu"_q,
		.title = tr::solar_MessageDetailsPC(),
		.boxTitle = tr::solar_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showMessageDetailsInContextMenu()),
		.options = options,
		.setter = [](int i) { SolarSettings::getInstance().setShowMessageDetailsInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconInfo },
	});
	solar.addChooseButton({
		.id = u"solar/showRepeatMessageInContextMenu"_q,
		.title = tr::solar_RepeatMessage(),
		.boxTitle = tr::solar_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showRepeatMessageInContextMenu()),
		.options = options,
		.setter = [](int i) { SolarSettings::getInstance().setShowRepeatMessageInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::solarRepeatMenuIcon },
	});
	if (settings->filtersEnabled()) {
		solar.addChooseButton({
			.id = u"solar/showAddFilterInContextMenu"_q,
			.title = tr::solar_RegexFilterQuickAdd(),
			.boxTitle = tr::solar_SettingsContextMenuTitle(),
			.initialSelection = static_cast<int>(settings->showAddFilterInContextMenu()),
			.options = options,
			.setter = [](int i) { SolarSettings::getInstance().setShowAddFilterInContextMenu(static_cast<ContextMenuVisibility>(i)); },
			.icon = { &st::menuIconAddToFolder },
		});
	}

	builder.addSkip();
	builder.addDividerText(tr::solar_SettingsContextMenuDescription());
	builder.addSkip();
}

void BuildMessageFieldElements(SectionBuilder &builder, SolarSectionBuilder &solar) {
	builder.addSubsectionTitle(tr::solar_MessageFieldElementsHeader());

	solar.addSettingToggle({
		.id = u"solar/showAttachButtonInMessageField"_q,
		.title = tr::solar_MessageFieldElementAttach(),
		.getter = &SolarSettings::showAttachButtonInMessageField,
		.setter = &SolarSettings::setShowAttachButtonInMessageField,
		.icon = { &st::messageFieldAttachIcon },
	});
	solar.addSettingToggle({
		.id = u"solar/showCommandsButtonInMessageField"_q,
		.title = tr::solar_MessageFieldElementCommands(),
		.getter = &SolarSettings::showCommandsButtonInMessageField,
		.setter = &SolarSettings::setShowCommandsButtonInMessageField,
		.icon = { &st::messageFieldCommandsIcon },
	});
	solar.addSettingToggle({
		.id = u"solar/showAutoDeleteButtonInMessageField"_q,
		.title = tr::solar_MessageFieldElementTTL(),
		.getter = &SolarSettings::showAutoDeleteButtonInMessageField,
		.setter = &SolarSettings::setShowAutoDeleteButtonInMessageField,
		.icon = { &st::messageFieldTTLIcon },
	});
	solar.addSettingToggle({
		.id = u"solar/showEmojiButtonInMessageField"_q,
		.title = tr::solar_MessageFieldElementEmoji(),
		.getter = &SolarSettings::showEmojiButtonInMessageField,
		.setter = &SolarSettings::setShowEmojiButtonInMessageField,
		.icon = { &st::messageFieldEmojiIcon },
	});
	solar.addSettingToggle({
		.id = u"solar/showMicrophoneButtonInMessageField"_q,
		.title = tr::solar_MessageFieldElementVoice(),
		.getter = &SolarSettings::showMicrophoneButtonInMessageField,
		.setter = &SolarSettings::setShowMicrophoneButtonInMessageField,
		.icon = { &st::messageFieldVoiceIcon },
	});
	solar.addSettingToggle({
		.id = u"solar/showGiftButtonInMessageField"_q,
		.title = tr::lng_profile_action_short_gift(),
		.getter = &SolarSettings::showGiftButtonInMessageField,
		.setter = &SolarSettings::setShowGiftButtonInMessageField,
		.icon = { &st::settingsButtonIconGift },
	});
	solar.addSettingToggle({
		.id = u"solar/showAiEditorButtonInMessageField"_q,
		.title = tr::lng_ai_compose_title(),
		.getter = &SolarSettings::showAiEditorButtonInMessageField,
		.setter = &SolarSettings::setShowAiEditorButtonInMessageField,
		.icon = { &st::messageFieldCocoonAiIcon },
	});

	solar.addSectionDivider();
}

void BuildMessageFieldPopups(SectionBuilder &builder, SolarSectionBuilder &solar) {
	builder.addSubsectionTitle(tr::solar_MessageFieldPopupsHeader());

	solar.addSettingToggle({
		.id = u"solar/showAttachPopup"_q,
		.title = tr::solar_MessageFieldElementAttach(),
		.getter = &SolarSettings::showAttachPopup,
		.setter = &SolarSettings::setShowAttachPopup,
		.icon = { &st::messageFieldAttachIcon },
	});
	solar.addSettingToggle({
		.id = u"solar/showEmojiPopup"_q,
		.title = tr::solar_MessageFieldElementEmoji(),
		.getter = &SolarSettings::showEmojiPopup,
		.setter = &SolarSettings::setShowEmojiPopup,
		.icon = { &st::messageFieldEmojiIcon },
	});
}

const auto kMeta = BuildHelper({
	.id = SolarChats::Id(),
	.parentId = SolarMain::Id(),
	.title = &tr::solar_CategoryChats,
	.icon = &st::menuIconChatBubble,
}, [](SectionBuilder &builder) {
	auto solar = SolarSectionBuilder(builder);
	const auto previewState = std::make_shared<PreviewState>();

	builder.addSkip();
	BuildStickersAndEmoji(builder, solar);
	BuildGroupsAndChannels(builder, solar);
	BuildMarks(builder, solar, previewState);
	BuildWideMessagesMultiplier(builder, solar, previewState);
	BuildContextMenuElements(builder, solar);
	BuildMessageFieldElements(builder, solar);
	BuildMessageFieldPopups(builder, solar);
	builder.addSkip();
});

} // namespace

rpl::producer<QString> SolarChats::title() {
	return tr::solar_CategoryChats();
}

SolarChats::SolarChats(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void SolarChats::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type SolarChatsId() {
	return SolarChats::Id();
}

} // namespace Settings
