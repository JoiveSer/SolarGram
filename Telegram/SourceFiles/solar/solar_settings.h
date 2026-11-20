// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "solar/libs/json.hpp"
#include "solar/libs/json_ext.hpp"
#include "rpl/lifetime.h"
#include "rpl/producer.h"
#include "rpl/variable.h"

#include <QtCore/QJsonObject>

#include <map>
#include <unordered_set>
#include <vector>


namespace Main {
class Session;
}

enum class PeerIdDisplay {
	Hidden = 0,
	TelegramApi = 1,
	BotApi = 2,
};

enum class ChannelBottomButton {
	Hidden = 0,
	MuteUnmute = 1,
	DiscussWithFallback = 2,
};

enum class ContextMenuVisibility {
	Hidden = 0,
	Visible = 1,
	VisibleWithModifier = 2,
};

enum class TranslationProvider {
	Telegram = 0,
	Google = 1,
	Yandex = 2,
	Native = 3,
	DeepLX = 4,
	TranSmart = 5,
	SolarBrainLLM = 6,
};

enum class ChatExclusionMode {
	GlobalDefault = 0,
	ForceAllow = 1,
	ForceBlock = 2,
};

struct PerChatGhostPolicy {
	ChatExclusionMode readMessages = ChatExclusionMode::GlobalDefault;
	ChatExclusionMode readStories = ChatExclusionMode::GlobalDefault;
	ChatExclusionMode onlinePackets = ChatExclusionMode::GlobalDefault;
	ChatExclusionMode uploadProgress = ChatExclusionMode::GlobalDefault;
	ChatExclusionMode offlineAfterOnline = ChatExclusionMode::GlobalDefault;

	bool operator==(const PerChatGhostPolicy &) const = default;
};

enum class SendWithoutSoundOption {
	Never = 0,
	InGhostMode = 1,
	Always = 2,
};

enum class DeletedSaveMode {
	Off = 0,
	SessionOnly = 1,
	Persistent = 2,
};

enum class DuressAction {
	DecoyEmptyView = 0,
	SilentPanicWipe = 1,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PeerIdDisplay, {
	{PeerIdDisplay::Hidden, 0},
	{PeerIdDisplay::TelegramApi, 1},
	{PeerIdDisplay::BotApi, 2},
})

NLOHMANN_JSON_SERIALIZE_ENUM(ChannelBottomButton, {
	{ChannelBottomButton::Hidden, 0},
	{ChannelBottomButton::MuteUnmute, 1},
	{ChannelBottomButton::DiscussWithFallback, 2},
})

NLOHMANN_JSON_SERIALIZE_ENUM(ContextMenuVisibility, {
	{ContextMenuVisibility::Hidden, 0},
	{ContextMenuVisibility::Visible, 1},
	{ContextMenuVisibility::VisibleWithModifier, 2},
})

NLOHMANN_JSON_SERIALIZE_ENUM(TranslationProvider, {
	{TranslationProvider::Telegram, "telegram"},
	{TranslationProvider::Google, "google"},
	{TranslationProvider::Yandex, "yandex"},
	{TranslationProvider::Native, "native"},
	{TranslationProvider::DeepLX, "deeplx"},
	{TranslationProvider::TranSmart, "transmart"},
	{TranslationProvider::SolarBrainLLM, "solarbrain"},
})

NLOHMANN_JSON_SERIALIZE_ENUM(SendWithoutSoundOption, {
	{SendWithoutSoundOption::Never, 0},
	{SendWithoutSoundOption::InGhostMode, 1},
	{SendWithoutSoundOption::Always, 2},
})

NLOHMANN_JSON_SERIALIZE_ENUM(DeletedSaveMode, {
	{DeletedSaveMode::Off, 0},
	{DeletedSaveMode::SessionOnly, 1},
	{DeletedSaveMode::Persistent, 2},
})

NLOHMANN_JSON_SERIALIZE_ENUM(ChatExclusionMode, {
	{ChatExclusionMode::GlobalDefault, 0},
	{ChatExclusionMode::ForceAllow, 1},
	{ChatExclusionMode::ForceBlock, 2},
})

NLOHMANN_JSON_SERIALIZE_ENUM(DuressAction, {
	{DuressAction::DecoyEmptyView, 0},
	{DuressAction::SilentPanicWipe, 1},
})

class GhostModeAccountSettings {
public:
	GhostModeAccountSettings();

	[[nodiscard]] bool sendReadMessages() const { return _sendReadMessages.current(); }
	[[nodiscard]] bool sendReadStories() const { return _sendReadStories.current(); }
	[[nodiscard]] bool sendOnlinePackets() const { return _sendOnlinePackets.current(); }
	[[nodiscard]] bool sendUploadProgress() const { return _sendUploadProgress.current(); }
	[[nodiscard]] bool sendOfflinePacketAfterOnline() const { return _sendOfflinePacketAfterOnline.current(); }
	[[nodiscard]] bool onlineBlink() const { return _onlineBlink.current(); }
	[[nodiscard]] bool markReadAfterAction() const { return _markReadAfterAction.current(); }
	[[nodiscard]] bool useScheduledMessages() const { return _useScheduledMessages.current(); }
	[[nodiscard]] SendWithoutSoundOption sendWithoutSound() const { return _sendWithoutSound.current(); }
	[[nodiscard]] bool shouldSendWithoutSound() const;
	[[nodiscard]] bool suggestGhostModeBeforeViewingStory() const { return _suggestGhostModeBeforeViewingStory.current(); }
	[[nodiscard]] bool isGhostModeActive() const { return _ghostModeActive.current(); }
	[[nodiscard]] bool isUseScheduledMessages() const { return isGhostModeActive() && useScheduledMessages(); }

	[[nodiscard]] bool sendReadMessagesLocked() const { return _sendReadMessagesLocked.current(); }
	[[nodiscard]] bool sendReadStoriesLocked() const { return _sendReadStoriesLocked.current(); }
	[[nodiscard]] bool sendOnlinePacketsLocked() const { return _sendOnlinePacketsLocked.current(); }
	[[nodiscard]] bool sendUploadProgressLocked() const { return _sendUploadProgressLocked.current(); }
	[[nodiscard]] bool sendOfflinePacketAfterOnlineLocked() const { return _sendOfflinePacketAfterOnlineLocked.current(); }

	void setSendReadMessages(bool val);
	void setSendReadStories(bool val);
	void setSendOnlinePackets(bool val);
	void setSendUploadProgress(bool val);
	void setSendOfflinePacketAfterOnline(bool val);
	void setOnlineBlink(bool val);
	void setMarkReadAfterAction(bool val);
	void setUseScheduledMessages(bool val);
	void setSendWithoutSound(SendWithoutSoundOption val);
	void setSuggestGhostModeBeforeViewingStory(bool val);
	void setGhostModeEnabled(bool val);

	void setSendReadMessagesLocked(bool val);
	void setSendReadStoriesLocked(bool val);
	void setSendOnlinePacketsLocked(bool val);
	void setSendUploadProgressLocked(bool val);
	void setSendOfflinePacketAfterOnlineLocked(bool val);

	[[nodiscard]] rpl::producer<bool> sendReadMessagesValue() const { return _sendReadMessages.value(); }
	[[nodiscard]] rpl::producer<bool> sendReadMessagesChanges() const { return _sendReadMessages.changes(); }
	[[nodiscard]] rpl::producer<bool> sendReadStoriesValue() const { return _sendReadStories.value(); }
	[[nodiscard]] rpl::producer<bool> sendReadStoriesChanges() const { return _sendReadStories.changes(); }
	[[nodiscard]] rpl::producer<bool> sendOnlinePacketsValue() const { return _sendOnlinePackets.value(); }
	[[nodiscard]] rpl::producer<bool> sendOnlinePacketsChanges() const { return _sendOnlinePackets.changes(); }
	[[nodiscard]] rpl::producer<bool> sendUploadProgressValue() const { return _sendUploadProgress.value(); }
	[[nodiscard]] rpl::producer<bool> sendUploadProgressChanges() const { return _sendUploadProgress.changes(); }
	[[nodiscard]] rpl::producer<bool> sendOfflinePacketAfterOnlineValue() const { return _sendOfflinePacketAfterOnline.value(); }
	[[nodiscard]] rpl::producer<bool> sendOfflinePacketAfterOnlineChanges() const { return _sendOfflinePacketAfterOnline.changes(); }
	[[nodiscard]] rpl::producer<bool> onlineBlinkValue() const { return _onlineBlink.value(); }
	[[nodiscard]] rpl::producer<bool> markReadAfterActionValue() const { return _markReadAfterAction.value(); }
	[[nodiscard]] rpl::producer<bool> markReadAfterActionChanges() const { return _markReadAfterAction.changes(); }
	[[nodiscard]] rpl::producer<bool> useScheduledMessagesValue() const { return _useScheduledMessages.value(); }
	[[nodiscard]] rpl::producer<bool> useScheduledMessagesChanges() const { return _useScheduledMessages.changes(); }
	[[nodiscard]] rpl::producer<SendWithoutSoundOption> sendWithoutSoundValue() const { return _sendWithoutSound.value(); }
	[[nodiscard]] rpl::producer<SendWithoutSoundOption> sendWithoutSoundChanges() const { return _sendWithoutSound.changes(); }
	[[nodiscard]] rpl::producer<bool> suggestGhostModeBeforeViewingStoryValue() const { return _suggestGhostModeBeforeViewingStory.value(); }
	[[nodiscard]] rpl::producer<bool> suggestGhostModeBeforeViewingStoryChanges() const { return _suggestGhostModeBeforeViewingStory.changes(); }
	[[nodiscard]] rpl::producer<bool> ghostModeActiveValue() const { return _ghostModeActive.value(); }
	[[nodiscard]] rpl::producer<bool> ghostModeActiveChanges() const { return _ghostModeActive.changes(); }

	[[nodiscard]] rpl::producer<bool> sendReadMessagesLockedValue() const { return _sendReadMessagesLocked.value(); }
	[[nodiscard]] rpl::producer<bool> sendReadMessagesLockedChanges() const { return _sendReadMessagesLocked.changes(); }
	[[nodiscard]] rpl::producer<bool> sendReadStoriesLockedValue() const { return _sendReadStoriesLocked.value(); }
	[[nodiscard]] rpl::producer<bool> sendReadStoriesLockedChanges() const { return _sendReadStoriesLocked.changes(); }
	[[nodiscard]] rpl::producer<bool> sendOnlinePacketsLockedValue() const { return _sendOnlinePacketsLocked.value(); }
	[[nodiscard]] rpl::producer<bool> sendOnlinePacketsLockedChanges() const { return _sendOnlinePacketsLocked.changes(); }
	[[nodiscard]] rpl::producer<bool> sendUploadProgressLockedValue() const { return _sendUploadProgressLocked.value(); }
	[[nodiscard]] rpl::producer<bool> sendUploadProgressLockedChanges() const { return _sendUploadProgressLocked.changes(); }
	[[nodiscard]] rpl::producer<bool> sendOfflinePacketAfterOnlineLockedValue() const { return _sendOfflinePacketAfterOnlineLocked.value(); }
	[[nodiscard]] rpl::producer<bool> sendOfflinePacketAfterOnlineLockedChanges() const { return _sendOfflinePacketAfterOnlineLocked.changes(); }

	friend void to_json(nlohmann::json &j, const GhostModeAccountSettings &s);
	friend void from_json(const nlohmann::json &j, GhostModeAccountSettings &s);

private:
	friend class SolarSettings;

	rpl::variable<bool> _sendReadMessages = true;
	rpl::variable<bool> _sendReadStories = true;
	rpl::variable<bool> _sendOnlinePackets = true;
	rpl::variable<bool> _sendUploadProgress = true;
	rpl::variable<bool> _sendOfflinePacketAfterOnline = false;
	rpl::variable<bool> _onlineBlink = false;
	rpl::variable<bool> _markReadAfterAction = true;
	rpl::variable<bool> _useScheduledMessages = false;
	rpl::variable<SendWithoutSoundOption> _sendWithoutSound = SendWithoutSoundOption::Never;
	rpl::variable<bool> _suggestGhostModeBeforeViewingStory = true;
	rpl::variable<bool> _ghostModeActive = false;

	rpl::variable<bool> _sendReadMessagesLocked = false;
	rpl::variable<bool> _sendReadStoriesLocked = false;
	rpl::variable<bool> _sendOnlinePacketsLocked = false;
	rpl::variable<bool> _sendUploadProgressLocked = false;
	rpl::variable<bool> _sendOfflinePacketAfterOnlineLocked = false;
};

void to_json(nlohmann::json &j, const GhostModeAccountSettings &s);
void from_json(const nlohmann::json &j, GhostModeAccountSettings &s);

class MessageShotSettings {
public:
	[[nodiscard]] bool showBackground() const { return _showBackground.current(); }
	[[nodiscard]] bool showDate() const { return _showDate.current(); }
	[[nodiscard]] bool showReactions() const { return _showReactions.current(); }
	[[nodiscard]] bool showHeaderDecorations() const { return _showHeaderDecorations.current(); }
	[[nodiscard]] bool showColorfulReplies() const { return _showColorfulReplies.current(); }
	[[nodiscard]] bool revealSpoilers() const { return _revealSpoilers.current(); }
	[[nodiscard]] int embeddedThemeType() const { return _embeddedThemeType.current(); }
	[[nodiscard]] uint32 embeddedThemeAccentColor() const { return _embeddedThemeAccentColor.current(); }
	[[nodiscard]] uint64 cloudThemeId() const { return _cloudThemeId.current(); }
	[[nodiscard]] uint64 cloudThemeAccessHash() const { return _cloudThemeAccessHash.current(); }
	[[nodiscard]] uint64 cloudThemeDocumentId() const { return _cloudThemeDocumentId.current(); }
	[[nodiscard]] const QString &cloudThemeTitle() const { return _cloudThemeTitle.current(); }
	[[nodiscard]] uint64 cloudThemeAccountId() const { return _cloudThemeAccountId.current(); }

	void setShowBackground(bool val);
	void setShowDate(bool val);
	void setShowReactions(bool val);
	void setShowHeaderDecorations(bool val);
	void setShowColorfulReplies(bool val);
	void setRevealSpoilers(bool val);

	void setEmbeddedTheme(int type, uint32 accentColor = 0);
	void setCloudTheme(uint64 accountId, uint64 id, uint64 accessHash, uint64 documentId, const QString &title);
	void clearTheme();

	friend void to_json(nlohmann::json &j, const MessageShotSettings &s);
	friend void from_json(const nlohmann::json &j, MessageShotSettings &s);

private:
	friend class SolarSettings;
	[[nodiscard]] bool isCloudThemeEmpty() const;
	void clearCloudThemeData();

	rpl::variable<bool> _showBackground = true;
	rpl::variable<bool> _showDate = false;
	rpl::variable<bool> _showReactions = false;
	rpl::variable<bool> _showHeaderDecorations = true;
	rpl::variable<bool> _showColorfulReplies = true;
	rpl::variable<bool> _revealSpoilers = true;

	rpl::variable<int> _embeddedThemeType = -1;
	rpl::variable<uint32> _embeddedThemeAccentColor = 0;

	rpl::variable<uint64> _cloudThemeId = 0;
	rpl::variable<uint64> _cloudThemeAccessHash = 0;
	rpl::variable<uint64> _cloudThemeDocumentId = 0;
	rpl::variable<QString> _cloudThemeTitle;
	rpl::variable<uint64> _cloudThemeAccountId = 0;

};

void to_json(nlohmann::json &j, const MessageShotSettings &s);
void from_json(const nlohmann::json &j, MessageShotSettings &s);

class SolarSettings {
public:
	SolarSettings(const SolarSettings &) = delete;
	SolarSettings &operator=(const SolarSettings &) = delete;
	SolarSettings(SolarSettings &&) = default;
	SolarSettings &operator=(SolarSettings &&) = default;

	[[nodiscard]] static SolarSettings &getInstance();

	static void load();
	static void save();
	static void reset();

	[[nodiscard]] static GhostModeAccountSettings &ghost(not_null<Main::Session*> session);
	[[nodiscard]] static GhostModeAccountSettings &ghost(uint64 userId);
	[[nodiscard]] static GhostModeAccountSettings &ghost();

	[[nodiscard]] bool useGlobalGhostMode() const { return _useGlobalGhostMode.current(); }
	void setUseGlobalGhostMode(bool val);

	[[nodiscard]] MessageShotSettings &messageShotSettings() { return _messageShotSettings; }
	[[nodiscard]] const MessageShotSettings &messageShotSettings() const { return _messageShotSettings; }

	void addShadowBan(int64 id);
	void removeShadowBan(int64 id);
	[[nodiscard]] bool isShadowBanned(const int64 id) const { return _shadowBanIds.contains(id); }
	[[nodiscard]] const std::unordered_set<int64> &shadowBanIds() const { return _shadowBanIds; }

	void setSavingExcluded(int64 dialogId, bool excluded);
	[[nodiscard]] bool isSavingExcluded(int64 dialogId) const { return _savingExclusions.contains(dialogId); }
	[[nodiscard]] const std::unordered_set<int64> &savingExclusions() const { return _savingExclusions; }

	void setReadExclusion(int64 dialogId, ChatExclusionMode mode);
	[[nodiscard]] ChatExclusionMode readExclusion(int64 dialogId) const;
	[[nodiscard]] bool effectiveShouldRead(not_null<Main::Session*> session, int64 dialogId) const;
	[[nodiscard]] bool effectiveShouldRead(uint64 userId, int64 dialogId) const;

	void setTypingExclusion(int64 dialogId, ChatExclusionMode mode);
	[[nodiscard]] ChatExclusionMode typingExclusion(int64 dialogId) const;
	[[nodiscard]] bool effectiveShouldSendTyping(not_null<Main::Session*> session, int64 dialogId) const;
	[[nodiscard]] bool effectiveShouldSendTyping(uint64 userId, int64 dialogId) const;

	void setPerChatGhostPolicy(int64 dialogId, const PerChatGhostPolicy &policy);
	[[nodiscard]] PerChatGhostPolicy perChatGhostPolicy(int64 dialogId) const;
	[[nodiscard]] bool effectiveShouldReadStories(not_null<Main::Session*> session, int64 dialogId) const;
	[[nodiscard]] bool effectiveShouldSendOnline(not_null<Main::Session*> session, int64 dialogId) const;
	[[nodiscard]] bool effectiveShouldOfflineAfterOnline(not_null<Main::Session*> session, int64 dialogId) const;

	void setPeerWatched(int64 dialogId, bool watched);
	[[nodiscard]] bool isPeerWatched(int64 dialogId) const { return _watchedPeerIds.contains(dialogId); }
	[[nodiscard]] const std::unordered_set<int64> &watchedPeerIds() const { return _watchedPeerIds; }

	void pushRecentChat(int64 peerId);
	void clearRecentChats();
	[[nodiscard]] const std::vector<int64> &recentChatIds() const { return _recentChatIds; }

	void setDuressPin(const QString &pin);
	void clearDuressPin();
	[[nodiscard]] bool hasDuressPin() const { return !_duressPinHash.current().isEmpty(); }
	[[nodiscard]] bool verifyDuressPin(const QString &pin) const;
	[[nodiscard]] bool decoyModeActive() const { return _decoyModeActive.current(); }
	void setDecoyModeActive(bool active);
	[[nodiscard]] rpl::producer<bool> decoyModeActiveValue() const { return _decoyModeActive.value(); }

	void setHiddenChatsPin(const QString &pin);
	void clearHiddenChatsPin();
	[[nodiscard]] bool hasHiddenChatsPin() const { return !_hiddenChatsPinHash.current().isEmpty(); }
	[[nodiscard]] bool verifyHiddenChatsPin(const QString &pin) const;
	void hideChat(int64 id);
	void unhideChat(int64 id);
	[[nodiscard]] bool isChatHidden(int64 id) const { return _hiddenChatIds.contains(id); }
	[[nodiscard]] const std::unordered_set<int64> &hiddenChatIds() const { return _hiddenChatIds; }
	[[nodiscard]] bool hiddenChatsRevealed() const { return _hiddenChatsRevealed.current(); }
	void setHiddenChatsRevealed(bool revealed);
	[[nodiscard]] rpl::producer<bool> hiddenChatsRevealedValue() const { return _hiddenChatsRevealed.value(); }

	[[nodiscard]] QJsonObject exportToJsonObject() const;
	bool importFromJsonObject(const QJsonObject &obj);

	struct DeviceSpoofSnapshot {
		bool enabled = false;
		QString model;
		QString system;
		QString appVersion;
		QString langCode;
	};
	[[nodiscard]] DeviceSpoofSnapshot deviceSpoofSnapshot() const;

	void validate();

	[[nodiscard]] bool saveDeletedMessages() const {
		return (_deletedSaveMode.current() != DeletedSaveMode::Off)
			&& _saveDeletedMessages.current();
	}
	[[nodiscard]] DeletedSaveMode deletedSaveMode() const { return _deletedSaveMode.current(); }
	[[nodiscard]] bool saveMessagesHistory() const { return _saveMessagesHistory.current(); }
	[[nodiscard]] bool saveForBots() const { return _saveForBots.current(); }
	[[nodiscard]] bool filtersEnabled() const { return _filtersEnabled.current(); }
	[[nodiscard]] bool filtersEnabledInChats() const { return _filtersEnabledInChats.current(); }
	[[nodiscard]] bool hideFromBlocked() const { return _hideFromBlocked.current(); }
	[[nodiscard]] bool semiTransparentDeletedMessages() const { return _semiTransparentDeletedMessages.current(); }
	[[nodiscard]] bool disableAds() const { return _disableAds.current(); }
	[[nodiscard]] bool disableStories() const { return _disableStories.current(); }
	[[nodiscard]] bool disableCustomBackgrounds() const { return _disableCustomBackgrounds.current(); }
	[[nodiscard]] bool hidePremiumStatuses() const { return _hidePremiumStatuses.current(); }
	[[nodiscard]] bool showOnlyAddedEmojisAndStickers() const { return _showOnlyAddedEmojisAndStickers.current(); }
	[[nodiscard]] bool collapseSimilarChannels() const { return _collapseSimilarChannels.current(); }
	[[nodiscard]] bool hideSimilarChannels() const { return _hideSimilarChannels.current(); }
	[[nodiscard]] int messageBubbleRadius() const { return _messageBubbleRadius.current(); }
	[[nodiscard]] bool disableOpenLinkWarning() const { return _disableOpenLinkWarning.current(); }
	[[nodiscard]] double wideMultiplier() const { return _wideMultiplier.current(); }
	[[nodiscard]] bool spoofWebviewAsAndroid() const { return _spoofWebviewAsAndroid.current(); }
	[[nodiscard]] bool increaseWebviewHeight() const { return _increaseWebviewHeight.current(); }
	[[nodiscard]] bool increaseWebviewWidth() const { return _increaseWebviewWidth.current(); }
	[[nodiscard]] bool materialSwitches() const { return _materialSwitches.current(); }
	[[nodiscard]] bool removeMessageTail() const { return _removeMessageTail.current(); }
	[[nodiscard]] bool disableNotificationsDelay() const { return _disableNotificationsDelay.current(); }
	[[nodiscard]] bool localPremium() const { return _localPremium.current(); }
	[[nodiscard]] bool showChannelReactions() const { return _showChannelReactions.current(); }
	[[nodiscard]] bool showGroupReactions() const { return _showGroupReactions.current(); }
	[[nodiscard]] bool showPrivateChatReactions() const { return _showPrivateChatReactions.current(); }
	[[nodiscard]] const QString &appIcon() const { return _appIcon.current(); }
	[[nodiscard]] bool simpleQuotesAndReplies() const { return _simpleQuotesAndReplies.current(); }
	[[nodiscard]] bool hideFastShare() const { return _hideFastShare.current(); }
	[[nodiscard]] bool replaceBottomInfoWithIcons() const { return _replaceBottomInfoWithIcons.current(); }
	[[nodiscard]] const QString &deletedMark() const { return _deletedMark.current(); }
	[[nodiscard]] const QString &editedMark() const { return _editedMark.current(); }
	[[nodiscard]] bool unlimitedRecentStickers() const { return _unlimitedRecentStickers.current(); }
	[[nodiscard]] ContextMenuVisibility showReactionsPanelInContextMenu() const { return _showReactionsPanelInContextMenu.current(); }
	[[nodiscard]] ContextMenuVisibility showViewsPanelInContextMenu() const { return _showViewsPanelInContextMenu.current(); }
	[[nodiscard]] ContextMenuVisibility showHideMessageInContextMenu() const { return _showHideMessageInContextMenu.current(); }
	[[nodiscard]] ContextMenuVisibility showUserMessagesInContextMenu() const { return _showUserMessagesInContextMenu.current(); }
	[[nodiscard]] ContextMenuVisibility showMessageDetailsInContextMenu() const { return _showMessageDetailsInContextMenu.current(); }
	[[nodiscard]] ContextMenuVisibility showRepeatMessageInContextMenu() const { return _showRepeatMessageInContextMenu.current(); }
	[[nodiscard]] ContextMenuVisibility showAddFilterInContextMenu() const { return _showAddFilterInContextMenu.current(); }
	[[nodiscard]] bool showAttachButtonInMessageField() const { return _showAttachButtonInMessageField.current(); }
	[[nodiscard]] bool showCommandsButtonInMessageField() const { return _showCommandsButtonInMessageField.current(); }
	[[nodiscard]] bool showEmojiButtonInMessageField() const { return _showEmojiButtonInMessageField.current(); }
	[[nodiscard]] bool showMicrophoneButtonInMessageField() const { return _showMicrophoneButtonInMessageField.current(); }
	[[nodiscard]] bool showAutoDeleteButtonInMessageField() const { return _showAutoDeleteButtonInMessageField.current(); }
	[[nodiscard]] bool showGiftButtonInMessageField() const { return _showGiftButtonInMessageField.current(); }
	[[nodiscard]] bool showAiEditorButtonInMessageField() const { return _showAiEditorButtonInMessageField.current(); }
	[[nodiscard]] bool showAttachPopup() const { return _showAttachPopup.current(); }
	[[nodiscard]] bool showEmojiPopup() const { return _showEmojiPopup.current(); }
	[[nodiscard]] bool showMyProfileInDrawer() const { return _showMyProfileInDrawer.current(); }
	[[nodiscard]] bool showBotsInDrawer() const { return _showBotsInDrawer.current(); }
	[[nodiscard]] bool showNewGroupInDrawer() const { return _showNewGroupInDrawer.current(); }
	[[nodiscard]] bool showNewChannelInDrawer() const { return _showNewChannelInDrawer.current(); }
	[[nodiscard]] bool showContactsInDrawer() const { return _showContactsInDrawer.current(); }
	[[nodiscard]] bool showCallsInDrawer() const { return _showCallsInDrawer.current(); }
	[[nodiscard]] bool showSavedMessagesInDrawer() const { return _showSavedMessagesInDrawer.current(); }
	[[nodiscard]] bool showLReadToggleInDrawer() const { return _showLReadToggleInDrawer.current(); }
	[[nodiscard]] bool showSReadToggleInDrawer() const { return _showSReadToggleInDrawer.current(); }
	[[nodiscard]] bool showNightModeToggleInDrawer() const { return _showNightModeToggleInDrawer.current(); }
	[[nodiscard]] bool showGhostToggleInDrawer() const { return _showGhostToggleInDrawer.current(); }
	[[nodiscard]] bool showStreamerToggleInDrawer() const { return _showStreamerToggleInDrawer.current(); }
	[[nodiscard]] bool showGhostToggleInTray() const { return _showGhostToggleInTray.current(); }
	[[nodiscard]] bool showStreamerToggleInTray() const { return _showStreamerToggleInTray.current(); }
	[[nodiscard]] const QString &monoFont() const { return _monoFont.current(); }
	[[nodiscard]] bool hideNotificationCounters() const { return _hideNotificationCounters.current(); }
	[[nodiscard]] bool hideNotificationBadge() const { return _hideNotificationBadge.current(); }
	[[nodiscard]] bool hideAllChatsFolder() const { return _hideAllChatsFolder.current(); }
	[[nodiscard]] ChannelBottomButton channelBottomButton() const { return _channelBottomButton.current(); }
	[[nodiscard]] bool quickAdminShortcuts() const { return _quickAdminShortcuts.current(); }
	[[nodiscard]] bool disableGreetingSticker() const { return _disableGreetingSticker.current(); }
	[[nodiscard]] PeerIdDisplay showPeerId() const { return _showPeerId.current(); }
	[[nodiscard]] bool showMessageSeconds() const { return _showMessageSeconds.current(); }
	[[nodiscard]] bool showMessageShot() const { return _showMessageShot.current(); }
	[[nodiscard]] bool filterZalgo() const { return _filterZalgo.current(); }
	[[nodiscard]] bool stickerConfirmation() const { return _stickerConfirmation.current(); }
	[[nodiscard]] bool gifConfirmation() const { return _gifConfirmation.current(); }
	[[nodiscard]] bool voiceConfirmation() const { return _voiceConfirmation.current(); }
	[[nodiscard]] bool roundConfirmation() const { return _roundConfirmation.current(); }
	[[nodiscard]] TranslationProvider translationProvider() const { return _translationProvider.current(); }
	[[nodiscard]] bool adaptiveCoverColor() const { return _adaptiveCoverColor.current(); }
	[[nodiscard]] bool improveLinkPreviews() const { return _improveLinkPreviews.current(); }
	[[nodiscard]] bool crashReporting() const { return _crashReporting.current(); }
	[[nodiscard]] int avatarCorners() const { return _avatarCorners.current(); }
	[[nodiscard]] bool singleCornerRadius() const { return _singleCornerRadius.current(); }
	[[nodiscard]] bool streamerMode() const { return _streamerMode.current(); }
	[[nodiscard]] bool autoStreamerMode() const { return _autoStreamerMode.current(); }
	[[nodiscard]] bool autoStripTrackingOnSend() const { return _autoStripTrackingOnSend.current(); }
	[[nodiscard]] bool antiCaps() const { return _antiCaps.current(); }
	[[nodiscard]] bool loginCodeLeakGuard() const { return _loginCodeLeakGuard.current(); }
	[[nodiscard]] bool eternalOnline() const { return _eternalOnline.current(); }
	[[nodiscard]] bool forceRelayCalls() const { return _forceRelayCalls.current(); }
	[[nodiscard]] bool callConfirmation() const { return _callConfirmation.current(); }
	[[nodiscard]] bool exactCounters() const { return _exactCounters.current(); }
	[[nodiscard]] bool alwaysOnTop() const { return _alwaysOnTop.current(); }
	[[nodiscard]] bool solarBrainEnabled() const { return _solarBrainEnabled.current(); }
	[[nodiscard]] const QString &solarBrainEndpoint() const { return _solarBrainEndpoint.current(); }
	[[nodiscard]] const QString &solarBrainModel() const { return _solarBrainModel.current(); }

	[[nodiscard]] bool saveMediaPrivate() const { return _saveMediaPrivate.current(); }
	[[nodiscard]] bool saveMediaGroups() const { return _saveMediaGroups.current(); }
	[[nodiscard]] bool saveMediaChannels() const { return _saveMediaChannels.current(); }
	[[nodiscard]] bool saveMediaBots() const { return _saveMediaBots.current(); }
	[[nodiscard]] bool saveMediaStickers() const { return _saveMediaStickers.current(); }
	[[nodiscard]] int attachmentsMaxSizeMb() const { return _attachmentsMaxSizeMb.current(); }
	[[nodiscard]] bool keepOneViewMedia() const { return _keepOneViewMedia.current(); }
	[[nodiscard]] bool spySaveReadMarks() const { return _spySaveReadMarks.current(); }
	[[nodiscard]] bool spySaveLocalOnline() const { return _spySaveLocalOnline.current(); }
	[[nodiscard]] bool displayGhostStatus() const { return _displayGhostStatus.current(); }
	[[nodiscard]] const QString &ghostModeShortcut() const { return _ghostModeShortcut.current(); }
	[[nodiscard]] bool uploadSpeedBoost() const { return _uploadSpeedBoost.current(); }
	[[nodiscard]] bool copyUsernameAsLink() const { return _copyUsernameAsLink.current(); }
	[[nodiscard]] bool walMode() const { return _walMode.current(); }
	[[nodiscard]] const QString &mainFont() const { return _mainFont.current(); }
	[[nodiscard]] DuressAction duressAction() const { return _duressAction.current(); }
	[[nodiscard]] bool globalHotkeysEnabled() const { return _globalHotkeysEnabled.current(); }
	[[nodiscard]] bool streamerMaskPrivacy() const { return _streamerMaskPrivacy.current(); }
	[[nodiscard]] bool streamerMaskNotifications() const { return _streamerMaskNotifications.current(); }
	[[nodiscard]] bool deleteForAllByDefault() const { return _deleteForAllByDefault.current(); }
	[[nodiscard]] bool showAlwaysOnTopButton() const { return _showAlwaysOnTopButton.current(); }
	[[nodiscard]] bool seekbarForRoundVideos() const { return _seekbarForRoundVideos.current(); }
	[[nodiscard]] bool textReplacementsEnabled() const { return _textReplacementsEnabled.current(); }
	[[nodiscard]] bool localFoldersEnabled() const { return _localFoldersEnabled.current(); }
	[[nodiscard]] bool presenceLoggerEnabled() const { return _presenceLoggerEnabled.current(); }
	[[nodiscard]] bool rememberLastSeen() const { return _rememberLastSeen.current(); }
	[[nodiscard]] bool stripImageExif() const { return _stripImageExif.current(); }
	[[nodiscard]] bool forcedCopy() const { return _forcedCopy.current(); }
	[[nodiscard]] bool deviceSpoofEnabled() const { return _deviceSpoofEnabled.current(); }
	[[nodiscard]] const QString &deviceSpoofModel() const { return _deviceSpoofModel.current(); }
	[[nodiscard]] const QString &deviceSpoofSystem() const { return _deviceSpoofSystem.current(); }
	[[nodiscard]] const QString &deviceSpoofAppVersion() const { return _deviceSpoofAppVersion.current(); }
	[[nodiscard]] const QString &deviceSpoofLangCode() const { return _deviceSpoofLangCode.current(); }
	[[nodiscard]] bool blockExternalLookups() const { return _blockExternalLookups.current(); }
	[[nodiscard]] bool randomDownloadNames() const { return _randomDownloadNames.current(); }
	[[nodiscard]] bool confirmExternalLinks() const { return _confirmExternalLinks.current(); }
	[[nodiscard]] bool phishingGuard() const { return _phishingGuard.current(); }
	[[nodiscard]] bool presenceOnlineAlerts() const { return _presenceOnlineAlerts.current(); }
	[[nodiscard]] bool downloadSpeedBoost() const { return _downloadSpeedBoost.current(); }
	[[nodiscard]] bool panguSpacing() const { return _panguSpacing.current(); }
	[[nodiscard]] bool stripForeignAgentDisclaimers() const { return _stripForeignAgentDisclaimers.current(); }
	[[nodiscard]] bool hideBirthdays() const { return _hideBirthdays.current(); }
	[[nodiscard]] bool hideGiftsUi() const { return _hideGiftsUi.current(); }
	[[nodiscard]] bool unreadChatsFirst() const { return _unreadChatsFirst.current(); }

	void setDeletedSaveMode(DeletedSaveMode val);
	void setSaveDeletedMessages(bool val);
	void setSaveMessagesHistory(bool val);
	void setSaveForBots(bool val);
	void setFiltersEnabled(bool val);
	void setFiltersEnabledInChats(bool val);
	void setHideFromBlocked(bool val);
	void setSemiTransparentDeletedMessages(bool val);
	void setDisableAds(bool val);
	void setDisableStories(bool val);
	void setDisableCustomBackgrounds(bool val);
	void setHidePremiumStatuses(bool val);
	void setShowOnlyAddedEmojisAndStickers(bool val);
	void setCollapseSimilarChannels(bool val);
	void setHideSimilarChannels(bool val);
	void setMessageBubbleRadius(int val);
	void setDisableOpenLinkWarning(bool val);
	void setWideMultiplier(double val);
	void setSpoofWebviewAsAndroid(bool val);
	void setIncreaseWebviewHeight(bool val);
	void setIncreaseWebviewWidth(bool val);
	void setMaterialSwitches(bool val);
	void setRemoveMessageTail(bool val);
	void setDisableNotificationsDelay(bool val);
	void setLocalPremium(bool val);
	void setShowChannelReactions(bool val);
	void setShowGroupReactions(bool val);
	void setShowPrivateChatReactions(bool val);
	void setAppIcon(const QString &val);
	void setSimpleQuotesAndReplies(bool val);
	void setHideFastShare(bool val);
	void setReplaceBottomInfoWithIcons(bool val);
	void setDeletedMark(const QString &val);
	void setEditedMark(const QString &val);
	void setUnlimitedRecentStickers(bool val);
	void setShowReactionsPanelInContextMenu(ContextMenuVisibility val);
	void setShowViewsPanelInContextMenu(ContextMenuVisibility val);
	void setShowHideMessageInContextMenu(ContextMenuVisibility val);
	void setShowUserMessagesInContextMenu(ContextMenuVisibility val);
	void setShowMessageDetailsInContextMenu(ContextMenuVisibility val);
	void setShowRepeatMessageInContextMenu(ContextMenuVisibility val);
	void setShowAddFilterInContextMenu(ContextMenuVisibility val);
	void setShowAttachButtonInMessageField(bool val);
	void setShowCommandsButtonInMessageField(bool val);
	void setShowEmojiButtonInMessageField(bool val);
	void setShowMicrophoneButtonInMessageField(bool val);
	void setShowAutoDeleteButtonInMessageField(bool val);
	void setShowGiftButtonInMessageField(bool val);
	void setShowAiEditorButtonInMessageField(bool val);
	void setShowAttachPopup(bool val);
	void setShowEmojiPopup(bool val);
	void setShowMyProfileInDrawer(bool val);
	void setShowBotsInDrawer(bool val);
	void setShowNewGroupInDrawer(bool val);
	void setShowNewChannelInDrawer(bool val);
	void setShowContactsInDrawer(bool val);
	void setShowCallsInDrawer(bool val);
	void setShowSavedMessagesInDrawer(bool val);
	void setShowLReadToggleInDrawer(bool val);
	void setShowSReadToggleInDrawer(bool val);
	void setShowNightModeToggleInDrawer(bool val);
	void setShowGhostToggleInDrawer(bool val);
	void setShowStreamerToggleInDrawer(bool val);
	void setShowGhostToggleInTray(bool val);
	void setShowStreamerToggleInTray(bool val);
	void setMonoFont(const QString &val);
	void setHideNotificationCounters(bool val);
	void setHideNotificationBadge(bool val);
	void setHideAllChatsFolder(bool val);
	void setChannelBottomButton(ChannelBottomButton val);
	void setQuickAdminShortcuts(bool val);
	void setDisableGreetingSticker(bool val);
	void setShowPeerId(PeerIdDisplay val);
	void setShowMessageSeconds(bool val);
	void setShowMessageShot(bool val);
	void setFilterZalgo(bool val);
	void setStickerConfirmation(bool val);
	void setGifConfirmation(bool val);
	void setVoiceConfirmation(bool val);
	void setRoundConfirmation(bool val);
	void setTranslationProvider(TranslationProvider val);
	void setAdaptiveCoverColor(bool val);
	void setImproveLinkPreviews(bool val);
	void setCrashReporting(bool val);
	void setAvatarCorners(int val);
	void setSingleCornerRadius(bool val);
	void setStreamerMode(bool val);
	void setAutoStreamerMode(bool val);
	void setAutoStripTrackingOnSend(bool val);
	void setAntiCaps(bool val);
	void setLoginCodeLeakGuard(bool val);
	void setEternalOnline(bool val);
	void setForceRelayCalls(bool val);
	void setCallConfirmation(bool val);
	void setExactCounters(bool val);
	void setAlwaysOnTop(bool val);
	void setSolarBrainEnabled(bool val);
	void setSolarBrainEndpoint(const QString &val);
	void setSolarBrainModel(const QString &val);

	void setSaveMediaPrivate(bool val);
	void setSaveMediaGroups(bool val);
	void setSaveMediaChannels(bool val);
	void setSaveMediaBots(bool val);
	void setSaveMediaStickers(bool val);
	void setAttachmentsMaxSizeMb(int val);
	void setKeepOneViewMedia(bool val);
	void setSpySaveReadMarks(bool val);
	void setSpySaveLocalOnline(bool val);
	void setDisplayGhostStatus(bool val);
	void setGhostModeShortcut(const QString &val);
	void setUploadSpeedBoost(bool val);
	void setCopyUsernameAsLink(bool val);
	void setWalMode(bool val);
	void setMainFont(const QString &val);
	void setDuressAction(DuressAction val);
	void setGlobalHotkeysEnabled(bool val);
	void setStreamerMaskPrivacy(bool val);
	void setStreamerMaskNotifications(bool val);
	void setDeleteForAllByDefault(bool val);
	void setShowAlwaysOnTopButton(bool val);
	void setSeekbarForRoundVideos(bool val);
	void setTextReplacementsEnabled(bool val);
	void setLocalFoldersEnabled(bool val);
	void setPresenceLoggerEnabled(bool val);
	void setRememberLastSeen(bool val);
	void setStripImageExif(bool val);
	void setForcedCopy(bool val);
	void setDeviceSpoofEnabled(bool val);
	void setDeviceSpoofModel(const QString &val);
	void setDeviceSpoofSystem(const QString &val);
	void setDeviceSpoofAppVersion(const QString &val);
	void setDeviceSpoofLangCode(const QString &val);
	void setBlockExternalLookups(bool val);
	void setRandomDownloadNames(bool val);
	void setConfirmExternalLinks(bool val);
	void setPhishingGuard(bool val);
	void setPresenceOnlineAlerts(bool val);
	void setDownloadSpeedBoost(bool val);
	void setPanguSpacing(bool val);
	void setStripForeignAgentDisclaimers(bool val);
	void setHideBirthdays(bool val);
	void setHideGiftsUi(bool val);
	void setUnreadChatsFirst(bool val);

	[[nodiscard]] rpl::producer<bool> useGlobalGhostModeValue() const { return _useGlobalGhostMode.value(); }
	[[nodiscard]] rpl::producer<bool> useGlobalGhostModeChanges() const { return _useGlobalGhostMode.changes(); }
	[[nodiscard]] rpl::producer<bool> saveDeletedMessagesValue() const { return _saveDeletedMessages.value(); }
	[[nodiscard]] rpl::producer<bool> saveDeletedMessagesChanges() const { return _saveDeletedMessages.changes(); }
	[[nodiscard]] rpl::producer<bool> saveMessagesHistoryValue() const { return _saveMessagesHistory.value(); }
	[[nodiscard]] rpl::producer<bool> saveMessagesHistoryChanges() const { return _saveMessagesHistory.changes(); }
	[[nodiscard]] rpl::producer<bool> saveForBotsValue() const { return _saveForBots.value(); }
	[[nodiscard]] rpl::producer<bool> saveForBotsChanges() const { return _saveForBots.changes(); }
	[[nodiscard]] rpl::producer<bool> filtersEnabledValue() const { return _filtersEnabled.value(); }
	[[nodiscard]] rpl::producer<bool> filtersEnabledChanges() const { return _filtersEnabled.changes(); }
	[[nodiscard]] rpl::producer<bool> filtersEnabledInChatsValue() const { return _filtersEnabledInChats.value(); }
	[[nodiscard]] rpl::producer<bool> filtersEnabledInChatsChanges() const { return _filtersEnabledInChats.changes(); }
	[[nodiscard]] rpl::producer<bool> hideFromBlockedValue() const { return _hideFromBlocked.value(); }
	[[nodiscard]] rpl::producer<bool> hideFromBlockedChanges() const { return _hideFromBlocked.changes(); }
	[[nodiscard]] rpl::producer<bool> semiTransparentDeletedMessagesValue() const { return _semiTransparentDeletedMessages.value(); }
	[[nodiscard]] rpl::producer<bool> semiTransparentDeletedMessagesChanges() const { return _semiTransparentDeletedMessages.changes(); }
	[[nodiscard]] rpl::producer<bool> disableAdsValue() const { return _disableAds.value(); }
	[[nodiscard]] rpl::producer<bool> disableAdsChanges() const { return _disableAds.changes(); }
	[[nodiscard]] rpl::producer<bool> disableStoriesValue() const { return _disableStories.value(); }
	[[nodiscard]] rpl::producer<bool> disableStoriesChanges() const { return _disableStories.changes(); }
	[[nodiscard]] rpl::producer<bool> disableCustomBackgroundsValue() const { return _disableCustomBackgrounds.value(); }
	[[nodiscard]] rpl::producer<bool> disableCustomBackgroundsChanges() const { return _disableCustomBackgrounds.changes(); }
	[[nodiscard]] rpl::producer<bool> hidePremiumStatusesValue() const { return _hidePremiumStatuses.value(); }
	[[nodiscard]] rpl::producer<bool> hidePremiumStatusesChanges() const { return _hidePremiumStatuses.changes(); }
	[[nodiscard]] rpl::producer<bool> showOnlyAddedEmojisAndStickersValue() const { return _showOnlyAddedEmojisAndStickers.value(); }
	[[nodiscard]] rpl::producer<bool> showOnlyAddedEmojisAndStickersChanges() const { return _showOnlyAddedEmojisAndStickers.changes(); }
	[[nodiscard]] rpl::producer<bool> collapseSimilarChannelsValue() const { return _collapseSimilarChannels.value(); }
	[[nodiscard]] rpl::producer<bool> collapseSimilarChannelsChanges() const { return _collapseSimilarChannels.changes(); }
	[[nodiscard]] rpl::producer<bool> hideSimilarChannelsValue() const { return _hideSimilarChannels.value(); }
	[[nodiscard]] rpl::producer<bool> hideSimilarChannelsChanges() const { return _hideSimilarChannels.changes(); }
	[[nodiscard]] rpl::producer<int> messageBubbleRadiusValue() const { return _messageBubbleRadius.value(); }
	[[nodiscard]] rpl::producer<int> messageBubbleRadiusChanges() const { return _messageBubbleRadius.changes(); }
	[[nodiscard]] rpl::producer<bool> disableOpenLinkWarningValue() const { return _disableOpenLinkWarning.value(); }
	[[nodiscard]] rpl::producer<bool> disableOpenLinkWarningChanges() const { return _disableOpenLinkWarning.changes(); }
	[[nodiscard]] rpl::producer<double> wideMultiplierValue() const { return _wideMultiplier.value(); }
	[[nodiscard]] rpl::producer<double> wideMultiplierChanges() const { return _wideMultiplier.changes(); }
	[[nodiscard]] rpl::producer<bool> spoofWebviewAsAndroidValue() const { return _spoofWebviewAsAndroid.value(); }
	[[nodiscard]] rpl::producer<bool> spoofWebviewAsAndroidChanges() const { return _spoofWebviewAsAndroid.changes(); }
	[[nodiscard]] rpl::producer<bool> increaseWebviewHeightValue() const { return _increaseWebviewHeight.value(); }
	[[nodiscard]] rpl::producer<bool> increaseWebviewHeightChanges() const { return _increaseWebviewHeight.changes(); }
	[[nodiscard]] rpl::producer<bool> increaseWebviewWidthValue() const { return _increaseWebviewWidth.value(); }
	[[nodiscard]] rpl::producer<bool> increaseWebviewWidthChanges() const { return _increaseWebviewWidth.changes(); }
	[[nodiscard]] rpl::producer<bool> materialSwitchesValue() const { return _materialSwitches.value(); }
	[[nodiscard]] rpl::producer<bool> materialSwitchesChanges() const { return _materialSwitches.changes(); }
	[[nodiscard]] rpl::producer<bool> removeMessageTailValue() const { return _removeMessageTail.value(); }
	[[nodiscard]] rpl::producer<bool> removeMessageTailChanges() const { return _removeMessageTail.changes(); }
	[[nodiscard]] rpl::producer<bool> disableNotificationsDelayValue() const { return _disableNotificationsDelay.value(); }
	[[nodiscard]] rpl::producer<bool> disableNotificationsDelayChanges() const { return _disableNotificationsDelay.changes(); }
	[[nodiscard]] rpl::producer<bool> localPremiumValue() const { return _localPremium.value(); }
	[[nodiscard]] rpl::producer<bool> localPremiumChanges() const { return _localPremium.changes(); }
	[[nodiscard]] rpl::producer<bool> showChannelReactionsValue() const { return _showChannelReactions.value(); }
	[[nodiscard]] rpl::producer<bool> showChannelReactionsChanges() const { return _showChannelReactions.changes(); }
	[[nodiscard]] rpl::producer<bool> showGroupReactionsValue() const { return _showGroupReactions.value(); }
	[[nodiscard]] rpl::producer<bool> showGroupReactionsChanges() const { return _showGroupReactions.changes(); }
	[[nodiscard]] rpl::producer<bool> showPrivateChatReactionsValue() const { return _showPrivateChatReactions.value(); }
	[[nodiscard]] rpl::producer<bool> showPrivateChatReactionsChanges() const { return _showPrivateChatReactions.changes(); }
	[[nodiscard]] rpl::producer<QString> appIconValue() const { return _appIcon.value(); }
	[[nodiscard]] rpl::producer<QString> appIconChanges() const { return _appIcon.changes(); }
	[[nodiscard]] rpl::producer<bool> simpleQuotesAndRepliesValue() const { return _simpleQuotesAndReplies.value(); }
	[[nodiscard]] rpl::producer<bool> simpleQuotesAndRepliesChanges() const { return _simpleQuotesAndReplies.changes(); }
	[[nodiscard]] rpl::producer<bool> hideFastShareValue() const { return _hideFastShare.value(); }
	[[nodiscard]] rpl::producer<bool> hideFastShareChanges() const { return _hideFastShare.changes(); }
	[[nodiscard]] rpl::producer<bool> replaceBottomInfoWithIconsValue() const { return _replaceBottomInfoWithIcons.value(); }
	[[nodiscard]] rpl::producer<bool> replaceBottomInfoWithIconsChanges() const { return _replaceBottomInfoWithIcons.changes(); }
	[[nodiscard]] rpl::producer<QString> deletedMarkValue() const { return _deletedMark.value(); }
	[[nodiscard]] rpl::producer<QString> deletedMarkChanges() const { return _deletedMark.changes(); }
	[[nodiscard]] rpl::producer<QString> editedMarkValue() const { return _editedMark.value(); }
	[[nodiscard]] rpl::producer<QString> editedMarkChanges() const { return _editedMark.changes(); }
	[[nodiscard]] rpl::producer<bool> unlimitedRecentStickersValue() const { return _unlimitedRecentStickers.value(); }
	[[nodiscard]] rpl::producer<bool> unlimitedRecentStickersChanges() const { return _unlimitedRecentStickers.changes(); }
	[[nodiscard]] rpl::producer<ContextMenuVisibility> showReactionsPanelInContextMenuValue() const { return _showReactionsPanelInContextMenu.value(); }
	[[nodiscard]] rpl::producer<ContextMenuVisibility> showReactionsPanelInContextMenuChanges() const { return _showReactionsPanelInContextMenu.changes(); }
	[[nodiscard]] rpl::producer<ContextMenuVisibility> showViewsPanelInContextMenuValue() const { return _showViewsPanelInContextMenu.value(); }
	[[nodiscard]] rpl::producer<ContextMenuVisibility> showViewsPanelInContextMenuChanges() const { return _showViewsPanelInContextMenu.changes(); }
	[[nodiscard]] rpl::producer<ContextMenuVisibility> showHideMessageInContextMenuValue() const { return _showHideMessageInContextMenu.value(); }
	[[nodiscard]] rpl::producer<ContextMenuVisibility> showHideMessageInContextMenuChanges() const { return _showHideMessageInContextMenu.changes(); }
	[[nodiscard]] rpl::producer<ContextMenuVisibility> showUserMessagesInContextMenuValue() const { return _showUserMessagesInContextMenu.value(); }
	[[nodiscard]] rpl::producer<ContextMenuVisibility> showUserMessagesInContextMenuChanges() const { return _showUserMessagesInContextMenu.changes(); }
	[[nodiscard]] rpl::producer<ContextMenuVisibility> showMessageDetailsInContextMenuValue() const { return _showMessageDetailsInContextMenu.value(); }
	[[nodiscard]] rpl::producer<ContextMenuVisibility> showMessageDetailsInContextMenuChanges() const { return _showMessageDetailsInContextMenu.changes(); }
	[[nodiscard]] rpl::producer<ContextMenuVisibility> showRepeatMessageInContextMenuValue() const { return _showRepeatMessageInContextMenu.value(); }
	[[nodiscard]] rpl::producer<ContextMenuVisibility> showRepeatMessageInContextMenuChanges() const { return _showRepeatMessageInContextMenu.changes(); }
	[[nodiscard]] rpl::producer<ContextMenuVisibility> showAddFilterInContextMenuValue() const { return _showAddFilterInContextMenu.value(); }
	[[nodiscard]] rpl::producer<ContextMenuVisibility> showAddFilterInContextMenuChanges() const { return _showAddFilterInContextMenu.changes(); }
	[[nodiscard]] rpl::producer<bool> showAttachButtonInMessageFieldValue() const { return _showAttachButtonInMessageField.value(); }
	[[nodiscard]] rpl::producer<bool> showAttachButtonInMessageFieldChanges() const { return _showAttachButtonInMessageField.changes(); }
	[[nodiscard]] rpl::producer<bool> showCommandsButtonInMessageFieldValue() const { return _showCommandsButtonInMessageField.value(); }
	[[nodiscard]] rpl::producer<bool> showCommandsButtonInMessageFieldChanges() const { return _showCommandsButtonInMessageField.changes(); }
	[[nodiscard]] rpl::producer<bool> showEmojiButtonInMessageFieldValue() const { return _showEmojiButtonInMessageField.value(); }
	[[nodiscard]] rpl::producer<bool> showEmojiButtonInMessageFieldChanges() const { return _showEmojiButtonInMessageField.changes(); }
	[[nodiscard]] rpl::producer<bool> showMicrophoneButtonInMessageFieldValue() const { return _showMicrophoneButtonInMessageField.value(); }
	[[nodiscard]] rpl::producer<bool> showMicrophoneButtonInMessageFieldChanges() const { return _showMicrophoneButtonInMessageField.changes(); }
	[[nodiscard]] rpl::producer<bool> showAutoDeleteButtonInMessageFieldValue() const { return _showAutoDeleteButtonInMessageField.value(); }
	[[nodiscard]] rpl::producer<bool> showAutoDeleteButtonInMessageFieldChanges() const { return _showAutoDeleteButtonInMessageField.changes(); }
	[[nodiscard]] rpl::producer<bool> showGiftButtonInMessageFieldValue() const { return _showGiftButtonInMessageField.value(); }
	[[nodiscard]] rpl::producer<bool> showGiftButtonInMessageFieldChanges() const { return _showGiftButtonInMessageField.changes(); }
	[[nodiscard]] rpl::producer<bool> showAiEditorButtonInMessageFieldValue() const { return _showAiEditorButtonInMessageField.value(); }
	[[nodiscard]] rpl::producer<bool> showAiEditorButtonInMessageFieldChanges() const { return _showAiEditorButtonInMessageField.changes(); }
	[[nodiscard]] rpl::producer<bool> showAttachPopupValue() const { return _showAttachPopup.value(); }
	[[nodiscard]] rpl::producer<bool> showAttachPopupChanges() const { return _showAttachPopup.changes(); }
	[[nodiscard]] rpl::producer<bool> showEmojiPopupValue() const { return _showEmojiPopup.value(); }
	[[nodiscard]] rpl::producer<bool> showEmojiPopupChanges() const { return _showEmojiPopup.changes(); }
	[[nodiscard]] rpl::producer<bool> showMyProfileInDrawerValue() const { return _showMyProfileInDrawer.value(); }
	[[nodiscard]] rpl::producer<bool> showMyProfileInDrawerChanges() const { return _showMyProfileInDrawer.changes(); }
	[[nodiscard]] rpl::producer<bool> showBotsInDrawerValue() const { return _showBotsInDrawer.value(); }
	[[nodiscard]] rpl::producer<bool> showBotsInDrawerChanges() const { return _showBotsInDrawer.changes(); }
	[[nodiscard]] rpl::producer<bool> showNewGroupInDrawerValue() const { return _showNewGroupInDrawer.value(); }
	[[nodiscard]] rpl::producer<bool> showNewGroupInDrawerChanges() const { return _showNewGroupInDrawer.changes(); }
	[[nodiscard]] rpl::producer<bool> showNewChannelInDrawerValue() const { return _showNewChannelInDrawer.value(); }
	[[nodiscard]] rpl::producer<bool> showNewChannelInDrawerChanges() const { return _showNewChannelInDrawer.changes(); }
	[[nodiscard]] rpl::producer<bool> showContactsInDrawerValue() const { return _showContactsInDrawer.value(); }
	[[nodiscard]] rpl::producer<bool> showContactsInDrawerChanges() const { return _showContactsInDrawer.changes(); }
	[[nodiscard]] rpl::producer<bool> showCallsInDrawerValue() const { return _showCallsInDrawer.value(); }
	[[nodiscard]] rpl::producer<bool> showCallsInDrawerChanges() const { return _showCallsInDrawer.changes(); }
	[[nodiscard]] rpl::producer<bool> showSavedMessagesInDrawerValue() const { return _showSavedMessagesInDrawer.value(); }
	[[nodiscard]] rpl::producer<bool> showSavedMessagesInDrawerChanges() const { return _showSavedMessagesInDrawer.changes(); }
	[[nodiscard]] rpl::producer<bool> showLReadToggleInDrawerValue() const { return _showLReadToggleInDrawer.value(); }
	[[nodiscard]] rpl::producer<bool> showLReadToggleInDrawerChanges() const { return _showLReadToggleInDrawer.changes(); }
	[[nodiscard]] rpl::producer<bool> showSReadToggleInDrawerValue() const { return _showSReadToggleInDrawer.value(); }
	[[nodiscard]] rpl::producer<bool> showSReadToggleInDrawerChanges() const { return _showSReadToggleInDrawer.changes(); }
	[[nodiscard]] rpl::producer<bool> showNightModeToggleInDrawerValue() const { return _showNightModeToggleInDrawer.value(); }
	[[nodiscard]] rpl::producer<bool> showNightModeToggleInDrawerChanges() const { return _showNightModeToggleInDrawer.changes(); }
	[[nodiscard]] rpl::producer<bool> showGhostToggleInDrawerValue() const { return _showGhostToggleInDrawer.value(); }
	[[nodiscard]] rpl::producer<bool> showGhostToggleInDrawerChanges() const { return _showGhostToggleInDrawer.changes(); }
	[[nodiscard]] rpl::producer<bool> showStreamerToggleInDrawerValue() const { return _showStreamerToggleInDrawer.value(); }
	[[nodiscard]] rpl::producer<bool> showStreamerToggleInDrawerChanges() const { return _showStreamerToggleInDrawer.changes(); }
	[[nodiscard]] rpl::producer<bool> showGhostToggleInTrayValue() const { return _showGhostToggleInTray.value(); }
	[[nodiscard]] rpl::producer<bool> showGhostToggleInTrayChanges() const { return _showGhostToggleInTray.changes(); }
	[[nodiscard]] rpl::producer<bool> showStreamerToggleInTrayValue() const { return _showStreamerToggleInTray.value(); }
	[[nodiscard]] rpl::producer<bool> showStreamerToggleInTrayChanges() const { return _showStreamerToggleInTray.changes(); }
	[[nodiscard]] rpl::producer<QString> monoFontValue() const { return _monoFont.value(); }
	[[nodiscard]] rpl::producer<QString> monoFontChanges() const { return _monoFont.changes(); }
	[[nodiscard]] rpl::producer<bool> hideNotificationCountersValue() const { return _hideNotificationCounters.value(); }
	[[nodiscard]] rpl::producer<bool> hideNotificationCountersChanges() const { return _hideNotificationCounters.changes(); }
	[[nodiscard]] rpl::producer<bool> hideNotificationBadgeValue() const { return _hideNotificationBadge.value(); }
	[[nodiscard]] rpl::producer<bool> hideNotificationBadgeChanges() const { return _hideNotificationBadge.changes(); }
	[[nodiscard]] rpl::producer<bool> hideAllChatsFolderValue() const { return _hideAllChatsFolder.value(); }
	[[nodiscard]] rpl::producer<bool> hideAllChatsFolderChanges() const { return _hideAllChatsFolder.changes(); }
	[[nodiscard]] rpl::producer<ChannelBottomButton> channelBottomButtonValue() const { return _channelBottomButton.value(); }
	[[nodiscard]] rpl::producer<ChannelBottomButton> channelBottomButtonChanges() const { return _channelBottomButton.changes(); }
	[[nodiscard]] rpl::producer<bool> quickAdminShortcutsValue() const { return _quickAdminShortcuts.value(); }
	[[nodiscard]] rpl::producer<bool> quickAdminShortcutsChanges() const { return _quickAdminShortcuts.changes(); }
	[[nodiscard]] rpl::producer<bool> disableGreetingStickerValue() const { return _disableGreetingSticker.value(); }
	[[nodiscard]] rpl::producer<bool> disableGreetingStickerChanges() const { return _disableGreetingSticker.changes(); }
	[[nodiscard]] rpl::producer<PeerIdDisplay> showPeerIdValue() const { return _showPeerId.value(); }
	[[nodiscard]] rpl::producer<PeerIdDisplay> showPeerIdChanges() const { return _showPeerId.changes(); }
	[[nodiscard]] rpl::producer<bool> showMessageSecondsValue() const { return _showMessageSeconds.value(); }
	[[nodiscard]] rpl::producer<bool> showMessageSecondsChanges() const { return _showMessageSeconds.changes(); }
	[[nodiscard]] rpl::producer<bool> showMessageShotValue() const { return _showMessageShot.value(); }
	[[nodiscard]] rpl::producer<bool> showMessageShotChanges() const { return _showMessageShot.changes(); }
	[[nodiscard]] rpl::producer<bool> filterZalgoValue() const { return _filterZalgo.value(); }
	[[nodiscard]] rpl::producer<bool> filterZalgoChanges() const { return _filterZalgo.changes(); }
	[[nodiscard]] rpl::producer<bool> stickerConfirmationValue() const { return _stickerConfirmation.value(); }
	[[nodiscard]] rpl::producer<bool> stickerConfirmationChanges() const { return _stickerConfirmation.changes(); }
	[[nodiscard]] rpl::producer<bool> gifConfirmationValue() const { return _gifConfirmation.value(); }
	[[nodiscard]] rpl::producer<bool> gifConfirmationChanges() const { return _gifConfirmation.changes(); }
	[[nodiscard]] rpl::producer<bool> voiceConfirmationValue() const { return _voiceConfirmation.value(); }
	[[nodiscard]] rpl::producer<bool> voiceConfirmationChanges() const { return _voiceConfirmation.changes(); }
	[[nodiscard]] rpl::producer<bool> roundConfirmationValue() const { return _roundConfirmation.value(); }
	[[nodiscard]] rpl::producer<bool> roundConfirmationChanges() const { return _roundConfirmation.changes(); }
	[[nodiscard]] rpl::producer<TranslationProvider> translationProviderValue() const { return _translationProvider.value(); }
	[[nodiscard]] rpl::producer<TranslationProvider> translationProviderChanges() const { return _translationProvider.changes(); }
	[[nodiscard]] rpl::producer<bool> adaptiveCoverColorValue() const { return _adaptiveCoverColor.value(); }
	[[nodiscard]] rpl::producer<bool> adaptiveCoverColorChanges() const { return _adaptiveCoverColor.changes(); }
	[[nodiscard]] rpl::producer<bool> improveLinkPreviewsValue() const { return _improveLinkPreviews.value(); }
	[[nodiscard]] rpl::producer<bool> improveLinkPreviewsChanges() const { return _improveLinkPreviews.changes(); }
	[[nodiscard]] rpl::producer<bool> crashReportingValue() const { return _crashReporting.value(); }
	[[nodiscard]] rpl::producer<bool> crashReportingChanges() const { return _crashReporting.changes(); }
	[[nodiscard]] rpl::producer<int> avatarCornersValue() const { return _avatarCorners.value(); }
	[[nodiscard]] rpl::producer<int> avatarCornersChanges() const { return _avatarCorners.changes(); }
	[[nodiscard]] rpl::producer<bool> singleCornerRadiusValue() const { return _singleCornerRadius.value(); }
	[[nodiscard]] rpl::producer<bool> singleCornerRadiusChanges() const { return _singleCornerRadius.changes(); }
	[[nodiscard]] rpl::producer<bool> streamerModeValue() const { return _streamerMode.value(); }
	[[nodiscard]] rpl::producer<bool> streamerModeChanges() const { return _streamerMode.changes(); }
	[[nodiscard]] rpl::producer<bool> autoStreamerModeValue() const { return _autoStreamerMode.value(); }
	[[nodiscard]] rpl::producer<bool> autoStreamerModeChanges() const { return _autoStreamerMode.changes(); }
	[[nodiscard]] rpl::producer<bool> autoStripTrackingOnSendValue() const { return _autoStripTrackingOnSend.value(); }
	[[nodiscard]] rpl::producer<bool> autoStripTrackingOnSendChanges() const { return _autoStripTrackingOnSend.changes(); }
	[[nodiscard]] rpl::producer<bool> antiCapsValue() const { return _antiCaps.value(); }
	[[nodiscard]] rpl::producer<bool> antiCapsChanges() const { return _antiCaps.changes(); }
	[[nodiscard]] rpl::producer<bool> loginCodeLeakGuardValue() const { return _loginCodeLeakGuard.value(); }
	[[nodiscard]] rpl::producer<bool> loginCodeLeakGuardChanges() const { return _loginCodeLeakGuard.changes(); }
	[[nodiscard]] rpl::producer<bool> eternalOnlineValue() const { return _eternalOnline.value(); }
	[[nodiscard]] rpl::producer<bool> eternalOnlineChanges() const { return _eternalOnline.changes(); }
	[[nodiscard]] rpl::producer<bool> forceRelayCallsValue() const { return _forceRelayCalls.value(); }
	[[nodiscard]] rpl::producer<bool> forceRelayCallsChanges() const { return _forceRelayCalls.changes(); }
	[[nodiscard]] rpl::producer<bool> callConfirmationValue() const { return _callConfirmation.value(); }
	[[nodiscard]] rpl::producer<bool> callConfirmationChanges() const { return _callConfirmation.changes(); }
	[[nodiscard]] rpl::producer<bool> exactCountersValue() const { return _exactCounters.value(); }
	[[nodiscard]] rpl::producer<bool> exactCountersChanges() const { return _exactCounters.changes(); }
	[[nodiscard]] rpl::producer<bool> alwaysOnTopValue() const { return _alwaysOnTop.value(); }
	[[nodiscard]] rpl::producer<bool> alwaysOnTopChanges() const { return _alwaysOnTop.changes(); }
	[[nodiscard]] rpl::producer<bool> solarBrainEnabledValue() const { return _solarBrainEnabled.value(); }
	[[nodiscard]] rpl::producer<bool> solarBrainEnabledChanges() const { return _solarBrainEnabled.changes(); }
	[[nodiscard]] rpl::producer<QString> solarBrainEndpointValue() const { return _solarBrainEndpoint.value(); }
	[[nodiscard]] rpl::producer<QString> solarBrainEndpointChanges() const { return _solarBrainEndpoint.changes(); }
	[[nodiscard]] rpl::producer<QString> solarBrainModelValue() const { return _solarBrainModel.value(); }
	[[nodiscard]] rpl::producer<QString> solarBrainModelChanges() const { return _solarBrainModel.changes(); }

	[[nodiscard]] rpl::producer<DeletedSaveMode> deletedSaveModeValue() const { return _deletedSaveMode.value(); }
	[[nodiscard]] rpl::producer<DeletedSaveMode> deletedSaveModeChanges() const { return _deletedSaveMode.changes(); }
	[[nodiscard]] rpl::producer<bool> saveMediaPrivateValue() const { return _saveMediaPrivate.value(); }
	[[nodiscard]] rpl::producer<bool> saveMediaGroupsValue() const { return _saveMediaGroups.value(); }
	[[nodiscard]] rpl::producer<bool> saveMediaChannelsValue() const { return _saveMediaChannels.value(); }
	[[nodiscard]] rpl::producer<bool> saveMediaBotsValue() const { return _saveMediaBots.value(); }
	[[nodiscard]] rpl::producer<bool> saveMediaStickersValue() const { return _saveMediaStickers.value(); }
	[[nodiscard]] rpl::producer<int> attachmentsMaxSizeMbValue() const { return _attachmentsMaxSizeMb.value(); }
	[[nodiscard]] rpl::producer<bool> keepOneViewMediaValue() const { return _keepOneViewMedia.value(); }
	[[nodiscard]] rpl::producer<bool> spySaveReadMarksValue() const { return _spySaveReadMarks.value(); }
	[[nodiscard]] rpl::producer<bool> spySaveLocalOnlineValue() const { return _spySaveLocalOnline.value(); }
	[[nodiscard]] rpl::producer<bool> displayGhostStatusValue() const { return _displayGhostStatus.value(); }
	[[nodiscard]] rpl::producer<bool> displayGhostStatusChanges() const { return _displayGhostStatus.changes(); }
	[[nodiscard]] rpl::producer<QString> ghostModeShortcutValue() const { return _ghostModeShortcut.value(); }
	[[nodiscard]] rpl::producer<bool> uploadSpeedBoostValue() const { return _uploadSpeedBoost.value(); }
	[[nodiscard]] rpl::producer<bool> copyUsernameAsLinkValue() const { return _copyUsernameAsLink.value(); }
	[[nodiscard]] rpl::producer<bool> walModeValue() const { return _walMode.value(); }
	[[nodiscard]] rpl::producer<QString> mainFontValue() const { return _mainFont.value(); }
	[[nodiscard]] rpl::producer<DuressAction> duressActionValue() const { return _duressAction.value(); }
	[[nodiscard]] rpl::producer<bool> globalHotkeysEnabledValue() const { return _globalHotkeysEnabled.value(); }
	[[nodiscard]] rpl::producer<bool> streamerMaskPrivacyValue() const { return _streamerMaskPrivacy.value(); }
	[[nodiscard]] rpl::producer<bool> streamerMaskNotificationsValue() const { return _streamerMaskNotifications.value(); }
	[[nodiscard]] rpl::producer<bool> deleteForAllByDefaultValue() const { return _deleteForAllByDefault.value(); }
	[[nodiscard]] rpl::producer<bool> showAlwaysOnTopButtonValue() const { return _showAlwaysOnTopButton.value(); }
	[[nodiscard]] rpl::producer<bool> showAlwaysOnTopButtonChanges() const { return _showAlwaysOnTopButton.changes(); }
	[[nodiscard]] rpl::producer<bool> seekbarForRoundVideosValue() const { return _seekbarForRoundVideos.value(); }
	[[nodiscard]] rpl::producer<bool> textReplacementsEnabledValue() const { return _textReplacementsEnabled.value(); }
	[[nodiscard]] rpl::producer<bool> localFoldersEnabledValue() const { return _localFoldersEnabled.value(); }
	[[nodiscard]] rpl::producer<bool> localFoldersEnabledChanges() const { return _localFoldersEnabled.changes(); }
	[[nodiscard]] rpl::producer<bool> presenceLoggerEnabledValue() const { return _presenceLoggerEnabled.value(); }
	[[nodiscard]] rpl::producer<bool> rememberLastSeenValue() const { return _rememberLastSeen.value(); }
	[[nodiscard]] rpl::producer<bool> stripImageExifValue() const { return _stripImageExif.value(); }
	[[nodiscard]] rpl::producer<bool> forcedCopyValue() const { return _forcedCopy.value(); }
	[[nodiscard]] rpl::producer<bool> deviceSpoofEnabledValue() const { return _deviceSpoofEnabled.value(); }
	[[nodiscard]] rpl::producer<QString> deviceSpoofModelValue() const { return _deviceSpoofModel.value(); }
	[[nodiscard]] rpl::producer<QString> deviceSpoofSystemValue() const { return _deviceSpoofSystem.value(); }
	[[nodiscard]] rpl::producer<QString> deviceSpoofAppVersionValue() const { return _deviceSpoofAppVersion.value(); }
	[[nodiscard]] rpl::producer<QString> deviceSpoofLangCodeValue() const { return _deviceSpoofLangCode.value(); }
	[[nodiscard]] rpl::producer<bool> blockExternalLookupsValue() const { return _blockExternalLookups.value(); }
	[[nodiscard]] rpl::producer<bool> randomDownloadNamesValue() const { return _randomDownloadNames.value(); }
	[[nodiscard]] rpl::producer<bool> confirmExternalLinksValue() const { return _confirmExternalLinks.value(); }
	[[nodiscard]] rpl::producer<bool> phishingGuardValue() const { return _phishingGuard.value(); }
	[[nodiscard]] rpl::producer<bool> presenceOnlineAlertsValue() const { return _presenceOnlineAlerts.value(); }
	[[nodiscard]] rpl::producer<bool> downloadSpeedBoostValue() const { return _downloadSpeedBoost.value(); }
	[[nodiscard]] rpl::producer<bool> panguSpacingValue() const { return _panguSpacing.value(); }
	[[nodiscard]] rpl::producer<bool> stripForeignAgentDisclaimersValue() const { return _stripForeignAgentDisclaimers.value(); }
	[[nodiscard]] rpl::producer<bool> hideBirthdaysValue() const { return _hideBirthdays.value(); }
	[[nodiscard]] rpl::producer<bool> hideGiftsUiValue() const { return _hideGiftsUi.value(); }
	[[nodiscard]] rpl::producer<bool> unreadChatsFirstValue() const { return _unreadChatsFirst.value(); }
	[[nodiscard]] rpl::producer<bool> unreadChatsFirstChanges() const { return _unreadChatsFirst.changes(); }
	[[nodiscard]] rpl::producer<int> savingExclusionsVersionChanges() const { return _savingExclusionsVersion.changes(); }
	[[nodiscard]] rpl::producer<int> hiddenChatsVersionChanges() const { return _hiddenChatsVersion.changes(); }

	friend void to_json(nlohmann::json &j, const SolarSettings &s);
	friend void from_json(const nlohmann::json &j, SolarSettings &s);

private:
	SolarSettings();

	[[nodiscard]] uint64 getOverriddenGhostUserId(uint64 userId) const { return _useGlobalGhostMode.current() ? 0 : userId; }

	rpl::variable<bool> _saveDeletedMessages = true;
	rpl::variable<bool> _saveMessagesHistory = true;
	rpl::variable<bool> _saveForBots = false;
	std::unordered_set<int64> _shadowBanIds;
	rpl::variable<bool> _filtersEnabled = false;
	rpl::variable<bool> _filtersEnabledInChats = false;
	rpl::variable<bool> _hideFromBlocked = false;
	rpl::variable<bool> _semiTransparentDeletedMessages = false;
	rpl::variable<bool> _disableAds = true;
	rpl::variable<bool> _disableStories = false;
	rpl::variable<bool> _disableCustomBackgrounds = false;
	rpl::variable<bool> _showOnlyAddedEmojisAndStickers = false;
	rpl::variable<bool> _collapseSimilarChannels = true;
	rpl::variable<bool> _hideSimilarChannels = false;
	rpl::variable<int> _messageBubbleRadius = 16;
	rpl::variable<bool> _disableOpenLinkWarning = false;
	rpl::variable<double> _wideMultiplier = 1.0;
	rpl::variable<bool> _spoofWebviewAsAndroid = false;
	rpl::variable<bool> _increaseWebviewHeight = false;
	rpl::variable<bool> _increaseWebviewWidth = false;
	rpl::variable<bool> _materialSwitches = true;
	rpl::variable<bool> _removeMessageTail = false;
	rpl::variable<bool> _disableNotificationsDelay = false;
	rpl::variable<bool> _localPremium = false;
	rpl::variable<bool> _showChannelReactions = true;
	rpl::variable<bool> _showGroupReactions = true;
	rpl::variable<bool> _showPrivateChatReactions = true;
	rpl::variable<QString> _appIcon;
	rpl::variable<bool> _simpleQuotesAndReplies = false;
	rpl::variable<bool> _hideFastShare = false;
	rpl::variable<bool> _replaceBottomInfoWithIcons = true;
	rpl::variable<QString> _deletedMark = QString::fromUtf8("🧹");
	rpl::variable<QString> _editedMark;
	rpl::variable<bool> _unlimitedRecentStickers = false;
	rpl::variable<ContextMenuVisibility> _showReactionsPanelInContextMenu = ContextMenuVisibility::Visible;
	rpl::variable<ContextMenuVisibility> _showViewsPanelInContextMenu = ContextMenuVisibility::Visible;
	rpl::variable<ContextMenuVisibility> _showHideMessageInContextMenu = ContextMenuVisibility::Hidden;
	rpl::variable<ContextMenuVisibility> _showUserMessagesInContextMenu = ContextMenuVisibility::VisibleWithModifier;
	rpl::variable<ContextMenuVisibility> _showMessageDetailsInContextMenu = ContextMenuVisibility::VisibleWithModifier;
	rpl::variable<ContextMenuVisibility> _showRepeatMessageInContextMenu = ContextMenuVisibility::Hidden;
	rpl::variable<ContextMenuVisibility> _showAddFilterInContextMenu = ContextMenuVisibility::Visible;
	rpl::variable<bool> _showAttachButtonInMessageField = true;
	rpl::variable<bool> _showCommandsButtonInMessageField = true;
	rpl::variable<bool> _showEmojiButtonInMessageField = true;
	rpl::variable<bool> _showMicrophoneButtonInMessageField = true;
	rpl::variable<bool> _showAutoDeleteButtonInMessageField = true;
	rpl::variable<bool> _showGiftButtonInMessageField = true;
	rpl::variable<bool> _showAiEditorButtonInMessageField = true;
	rpl::variable<bool> _showAttachPopup = true;
	rpl::variable<bool> _showEmojiPopup = true;
	rpl::variable<bool> _showMyProfileInDrawer = true;
	rpl::variable<bool> _showBotsInDrawer = true;
	rpl::variable<bool> _showNewGroupInDrawer = true;
	rpl::variable<bool> _showNewChannelInDrawer = true;
	rpl::variable<bool> _showContactsInDrawer = true;
	rpl::variable<bool> _showCallsInDrawer = true;
	rpl::variable<bool> _showSavedMessagesInDrawer = true;
	rpl::variable<bool> _showLReadToggleInDrawer = false;
	rpl::variable<bool> _showSReadToggleInDrawer = true;
	rpl::variable<bool> _showNightModeToggleInDrawer = true;
	rpl::variable<bool> _showGhostToggleInDrawer = true;
	rpl::variable<bool> _showStreamerToggleInDrawer = false;
	rpl::variable<bool> _showGhostToggleInTray = true;
	rpl::variable<bool> _showStreamerToggleInTray = false;
	rpl::variable<bool> _hidePremiumStatuses = false;
	rpl::variable<QString> _monoFont;
	rpl::variable<bool> _hideNotificationCounters = false;
	rpl::variable<bool> _hideNotificationBadge = false;
	rpl::variable<bool> _hideAllChatsFolder = false;
	rpl::variable<ChannelBottomButton> _channelBottomButton = ChannelBottomButton::DiscussWithFallback;
	rpl::variable<bool> _quickAdminShortcuts = true;
	rpl::variable<bool> _disableGreetingSticker = false;
	rpl::variable<PeerIdDisplay> _showPeerId = PeerIdDisplay::BotApi;
	rpl::variable<bool> _showMessageSeconds = false;
	rpl::variable<bool> _showMessageShot = true;
	rpl::variable<bool> _filterZalgo = false;
	rpl::variable<bool> _stickerConfirmation = false;
	rpl::variable<bool> _gifConfirmation = false;
	rpl::variable<bool> _voiceConfirmation = false;
	rpl::variable<bool> _roundConfirmation = false;
	rpl::variable<TranslationProvider> _translationProvider = TranslationProvider::Telegram;
	rpl::variable<bool> _adaptiveCoverColor = true;
	rpl::variable<bool> _improveLinkPreviews = false;
	rpl::variable<bool> _crashReporting = true;
	rpl::variable<int> _avatarCorners = 23;
	rpl::variable<bool> _singleCornerRadius = false;
	rpl::variable<bool> _streamerMode = false;
	rpl::variable<bool> _autoStreamerMode = true;
	rpl::variable<bool> _autoStripTrackingOnSend = true;
	rpl::variable<bool> _antiCaps = false;
	rpl::variable<bool> _loginCodeLeakGuard = true;
	rpl::variable<bool> _eternalOnline = false;
	rpl::variable<bool> _forceRelayCalls = false;
	rpl::variable<bool> _callConfirmation = true;
	rpl::variable<bool> _exactCounters = true;
	rpl::variable<bool> _alwaysOnTop = false;
	rpl::variable<bool> _solarBrainEnabled = true;
	rpl::variable<QString> _solarBrainEndpoint = QStringLiteral("http://127.0.0.1:11434/api/chat");
	rpl::variable<QString> _solarBrainModel = QStringLiteral("qwen2.5:7b");

	rpl::variable<DeletedSaveMode> _deletedSaveMode = DeletedSaveMode::Persistent;
	rpl::variable<bool> _saveMediaPrivate = true;
	rpl::variable<bool> _saveMediaGroups = true;
	rpl::variable<bool> _saveMediaChannels = false;
	rpl::variable<bool> _saveMediaBots = false;
	rpl::variable<bool> _saveMediaStickers = false;
	rpl::variable<int> _attachmentsMaxSizeMb = 2048;
	rpl::variable<bool> _keepOneViewMedia = true;
	rpl::variable<bool> _spySaveReadMarks = true;
	rpl::variable<bool> _spySaveLocalOnline = true;
	rpl::variable<bool> _displayGhostStatus = true;
	rpl::variable<QString> _ghostModeShortcut = QStringLiteral("Ctrl+Shift+G");
	rpl::variable<bool> _uploadSpeedBoost = false;
	rpl::variable<bool> _copyUsernameAsLink = true;
	rpl::variable<bool> _walMode = true;
	rpl::variable<QString> _mainFont;
	rpl::variable<QString> _duressPinHash;
	rpl::variable<DuressAction> _duressAction = DuressAction::DecoyEmptyView;
	rpl::variable<bool> _decoyModeActive = false;
	rpl::variable<QString> _hiddenChatsPinHash;
	std::unordered_set<int64> _hiddenChatIds;
	rpl::variable<bool> _hiddenChatsRevealed = false;
	rpl::variable<bool> _globalHotkeysEnabled = true;
	rpl::variable<bool> _streamerMaskPrivacy = true;
	rpl::variable<bool> _streamerMaskNotifications = true;
	rpl::variable<bool> _deleteForAllByDefault = true;
	rpl::variable<bool> _showAlwaysOnTopButton = true;
	rpl::variable<bool> _seekbarForRoundVideos = true;
	rpl::variable<bool> _textReplacementsEnabled = true;
	rpl::variable<bool> _localFoldersEnabled = true;
	rpl::variable<bool> _presenceLoggerEnabled = true;
	rpl::variable<bool> _rememberLastSeen = true;
	rpl::variable<bool> _stripImageExif = true;
	rpl::variable<bool> _forcedCopy = false;
	rpl::variable<bool> _deviceSpoofEnabled = false;
	rpl::variable<QString> _deviceSpoofModel = QStringLiteral("Workstation Pro");
	rpl::variable<QString> _deviceSpoofSystem = QStringLiteral("Windows 11");
	rpl::variable<QString> _deviceSpoofAppVersion;
	rpl::variable<QString> _deviceSpoofLangCode;
	rpl::variable<bool> _blockExternalLookups = true;
	rpl::variable<bool> _randomDownloadNames = false;
	rpl::variable<bool> _confirmExternalLinks = false;
	rpl::variable<bool> _phishingGuard = true;
	rpl::variable<bool> _presenceOnlineAlerts = false;
	rpl::variable<bool> _downloadSpeedBoost = false;
	rpl::variable<bool> _panguSpacing = false;
	rpl::variable<bool> _stripForeignAgentDisclaimers = false;
	rpl::variable<bool> _hideBirthdays = false;
	rpl::variable<bool> _hideGiftsUi = false;
	rpl::variable<bool> _unreadChatsFirst = false;
	std::unordered_set<int64> _savingExclusions;
	std::map<int64, ChatExclusionMode> _readExclusions;
	std::map<int64, ChatExclusionMode> _typingExclusions;
	std::map<int64, PerChatGhostPolicy> _perChatGhost;
	std::unordered_set<int64> _watchedPeerIds;
	std::vector<int64> _recentChatIds;
	rpl::variable<int> _savingExclusionsVersion = 0;
	rpl::variable<int> _hiddenChatsVersion = 0;

	rpl::variable<bool> _useGlobalGhostMode = true;
	std::map<uint64, std::unique_ptr<GhostModeAccountSettings>> _ghostAccounts;

	MessageShotSettings _messageShotSettings;
};

void to_json(nlohmann::json &j, const SolarSettings &s);
void from_json(const nlohmann::json &j, SolarSettings &s);
