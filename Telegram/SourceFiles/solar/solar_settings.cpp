// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/solar_settings.h"

#include "lang_auto.h"
#include "tray.h"
#include "solar/solar_ui_settings.h"
#include "solar/solar_worker.h"
#include "solar/features/streamer_mode/streamer_mode.h"
#include "solar/ui/solar_logo.h"
#include "core/application.h"
#include "features/filters/filters_cache_controller.h"
#include "features/translator/solar_translator.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "platform/platform_translate_provider.h"
#include "rpl/combine.h"
#include "window/window_controller.h"

#include <algorithm>
#include <fstream>
#include <mutex>
#include <vector>
#include <QApplication>
#include <QtCore/QCryptographicHash>
#include <QtCore/QJsonDocument>

using json = nlohmann::json;

namespace {

std::string getSettingsPath() {
	return (cWorkingDir() + u"tdata/solar_settings.json"_q).toStdString();
}

void repaintApp() {
	for (QWidget *widget : QApplication::allWidgets()) {
		widget->update();
	}
}

rpl::lifetime lifetime; // idk reactivity dies when placed in `GhostModeAccountSettings` as field

QString hashPartisanPin(const QString &pin) {
	if (pin.isEmpty()) {
		return QString();
	}
	const auto salted = QByteArray("solargram_partisan_v1:") + pin.toUtf8();
	return QString::fromLatin1(
		QCryptographicHash::hash(salted, QCryptographicHash::Sha256).toHex());
}

std::mutex &spoofSnapshotMutex() {
	static std::mutex mutex;
	return mutex;
}

SolarSettings::DeviceSpoofSnapshot &cachedSpoofSnapshot() {
	static SolarSettings::DeviceSpoofSnapshot snapshot;
	return snapshot;
}

void updateSpoofSnapshot(
		bool enabled,
		const QString &model,
		const QString &system,
		const QString &appVersion,
		const QString &langCode) {
	std::lock_guard lock(spoofSnapshotMutex());
	auto &s = cachedSpoofSnapshot();
	s.enabled = enabled;
	s.model = model;
	s.system = system;
	s.appVersion = appVersion;
	s.langCode = langCode;
}

} // namespace

GhostModeAccountSettings::GhostModeAccountSettings() {
	rpl::combine(
		_sendReadMessages.value(),
		_sendReadMessagesLocked.value(),
		_sendReadStories.value(),
		_sendReadStoriesLocked.value(),
		_sendOnlinePackets.value(),
		_sendOnlinePacketsLocked.value(),
		_sendUploadProgress.value(),
		_sendUploadProgressLocked.value(),
		_sendOfflinePacketAfterOnline.value(),
		_sendOfflinePacketAfterOnlineLocked.value()
	) | rpl::on_next([=](
			bool readMsg, bool readMsgLocked,
			bool readStories, bool readStoriesLocked,
			bool online, bool onlineLocked,
			bool upload, bool uploadLocked,
			bool offline, bool offlineLocked) {
		_ghostModeActive = (readMsgLocked || !readMsg)
			&& (readStoriesLocked || !readStories)
			&& (onlineLocked || !online)
			&& (uploadLocked || !upload)
			&& (offlineLocked || offline);
	}, lifetime);
}

void GhostModeAccountSettings::setSendReadMessages(bool val) {
	if (_sendReadMessages.current() == val) return;
	_sendReadMessages = val;
	SolarSettings::save();
}

void GhostModeAccountSettings::setSendReadStories(bool val) {
	if (_sendReadStories.current() == val) return;
	_sendReadStories = val;
	SolarSettings::save();
}

void GhostModeAccountSettings::setSendOnlinePackets(bool val) {
	if (_sendOnlinePackets.current() == val) return;
	_sendOnlinePackets = val;
	SolarSettings::save();
}

void GhostModeAccountSettings::setSendUploadProgress(bool val) {
	if (_sendUploadProgress.current() == val) return;
	_sendUploadProgress = val;
	SolarSettings::save();
}

void GhostModeAccountSettings::setSendOfflinePacketAfterOnline(bool val) {
	if (_sendOfflinePacketAfterOnline.current() == val) return;
	_sendOfflinePacketAfterOnline = val;
	SolarSettings::save();
}

void GhostModeAccountSettings::setOnlineBlink(bool val) {
	if (_onlineBlink.current() == val) return;
	_onlineBlink = val;
	SolarSettings::save();
}

void GhostModeAccountSettings::setMarkReadAfterAction(bool val) {
	if (_markReadAfterAction.current() == val) return;
	_markReadAfterAction = val;
	SolarSettings::save();
}

void GhostModeAccountSettings::setUseScheduledMessages(bool val) {
	if (_useScheduledMessages.current() == val) return;
	_useScheduledMessages = val;
	SolarSettings::save();
}

bool GhostModeAccountSettings::shouldSendWithoutSound() const {
	switch (_sendWithoutSound.current()) {
	case SendWithoutSoundOption::Never:
		return false;
	case SendWithoutSoundOption::InGhostMode:
		return isGhostModeActive();
	case SendWithoutSoundOption::Always:
		return true;
	}
	Unexpected("Value in GhostModeAccountSettings::shouldSendWithoutSound.");
}

void GhostModeAccountSettings::setSendWithoutSound(
		SendWithoutSoundOption val) {
	if (_sendWithoutSound.current() == val) return;
	_sendWithoutSound = val;
	SolarSettings::save();
}

void GhostModeAccountSettings::setSuggestGhostModeBeforeViewingStory(bool val) {
	if (_suggestGhostModeBeforeViewingStory.current() == val) return;
	_suggestGhostModeBeforeViewingStory = val;
	SolarSettings::save();
}

void GhostModeAccountSettings::setGhostModeEnabled(bool val) {
	if (!_sendReadMessagesLocked.current()) _sendReadMessages = !val;
	if (!_sendReadStoriesLocked.current()) _sendReadStories = !val;
	if (!_sendOnlinePacketsLocked.current()) _sendOnlinePackets = !val;
	if (!_sendUploadProgressLocked.current()) _sendUploadProgress = !val;
	if (!_sendOfflinePacketAfterOnlineLocked.current()) _sendOfflinePacketAfterOnline = val;
	SolarSettings::save();

	if (val) {
		if (const auto window = Core::App().activeWindow()) {
			if (const auto session = window->maybeSession()) {
				SolarWorker::markAsOnline(session);
			}
		}
	}
}

void GhostModeAccountSettings::setSendReadMessagesLocked(bool val) {
	if (_sendReadMessagesLocked.current() == val) return;
	_sendReadMessagesLocked = val;
	SolarSettings::save();
}

void GhostModeAccountSettings::setSendReadStoriesLocked(bool val) {
	if (_sendReadStoriesLocked.current() == val) return;
	_sendReadStoriesLocked = val;
	SolarSettings::save();
}

void GhostModeAccountSettings::setSendOnlinePacketsLocked(bool val) {
	if (_sendOnlinePacketsLocked.current() == val) return;
	_sendOnlinePacketsLocked = val;
	SolarSettings::save();
}

void GhostModeAccountSettings::setSendUploadProgressLocked(bool val) {
	if (_sendUploadProgressLocked.current() == val) return;
	_sendUploadProgressLocked = val;
	SolarSettings::save();
}

void GhostModeAccountSettings::setSendOfflinePacketAfterOnlineLocked(bool val) {
	if (_sendOfflinePacketAfterOnlineLocked.current() == val) return;
	_sendOfflinePacketAfterOnlineLocked = val;
	SolarSettings::save();
}

void to_json(nlohmann::json &j, const GhostModeAccountSettings &s) {
	j = nlohmann::json{
		{"sendReadMessages", s._sendReadMessages.current()},
		{"sendReadStories", s._sendReadStories.current()},
		{"sendOnlinePackets", s._sendOnlinePackets.current()},
		{"sendUploadProgress", s._sendUploadProgress.current()},
		{"sendOfflinePacketAfterOnline", s._sendOfflinePacketAfterOnline.current()},
		{"onlineBlink", s._onlineBlink.current()},
		{"markReadAfterAction", s._markReadAfterAction.current()},
		{"useScheduledMessages", s._useScheduledMessages.current()},
		{"sendWithoutSound", s._sendWithoutSound.current()},
		{"suggestGhostModeBeforeViewingStory", s._suggestGhostModeBeforeViewingStory.current()},
		{"sendReadMessagesLocked", s._sendReadMessagesLocked.current()},
		{"sendReadStoriesLocked", s._sendReadStoriesLocked.current()},
		{"sendOnlinePacketsLocked", s._sendOnlinePacketsLocked.current()},
		{"sendUploadProgressLocked", s._sendUploadProgressLocked.current()},
		{"sendOfflinePacketAfterOnlineLocked", s._sendOfflinePacketAfterOnlineLocked.current()}
	};
}

void from_json(const nlohmann::json &j, GhostModeAccountSettings &s) {
	s._sendReadMessages = j.value("sendReadMessages", true);
	s._sendReadStories = j.value("sendReadStories", true);
	s._sendOnlinePackets = j.value("sendOnlinePackets", true);
	s._sendUploadProgress = j.value("sendUploadProgress", true);
	s._sendOfflinePacketAfterOnline = j.value("sendOfflinePacketAfterOnline", false);
	s._onlineBlink = j.value("onlineBlink", false);
	s._markReadAfterAction = j.value("markReadAfterAction", true);
	s._useScheduledMessages = j.value("useScheduledMessages", false);
	const auto sendWithoutSound = j.find("sendWithoutSound");
	s._sendWithoutSound = (sendWithoutSound == j.end())
		? SendWithoutSoundOption::Never
		: sendWithoutSound->is_boolean()
		? (sendWithoutSound->get<bool>()
			? SendWithoutSoundOption::Always
			: SendWithoutSoundOption::Never)
		: sendWithoutSound->get<SendWithoutSoundOption>();
	s._suggestGhostModeBeforeViewingStory = j.value("suggestGhostModeBeforeViewingStory", true);
	s._sendReadMessagesLocked = j.value("sendReadMessagesLocked", false);
	s._sendReadStoriesLocked = j.value("sendReadStoriesLocked", false);
	s._sendOnlinePacketsLocked = j.value("sendOnlinePacketsLocked", false);
	s._sendUploadProgressLocked = j.value("sendUploadProgressLocked", false);
	s._sendOfflinePacketAfterOnlineLocked = j.value("sendOfflinePacketAfterOnlineLocked", false);
}

void MessageShotSettings::setShowBackground(bool val) {
	if (_showBackground.current() == val) return;
	_showBackground = val;
	SolarSettings::save();
}

void MessageShotSettings::setShowDate(bool val) {
	if (_showDate.current() == val) return;
	_showDate = val;
	SolarSettings::save();
}

void MessageShotSettings::setShowReactions(bool val) {
	if (_showReactions.current() == val) return;
	_showReactions = val;
	SolarSettings::save();
}

void MessageShotSettings::setShowHeaderDecorations(bool val) {
	if (_showHeaderDecorations.current() == val) return;
	_showHeaderDecorations = val;
	SolarSettings::save();
}

void MessageShotSettings::setShowColorfulReplies(bool val) {
	if (_showColorfulReplies.current() == val) return;
	_showColorfulReplies = val;
	SolarSettings::save();
}

void MessageShotSettings::setRevealSpoilers(bool val) {
	if (_revealSpoilers.current() == val) return;
	_revealSpoilers = val;
	SolarSettings::save();
}

bool MessageShotSettings::isCloudThemeEmpty() const {
	return !_cloudThemeId.current()
		&& !_cloudThemeAccessHash.current()
		&& !_cloudThemeDocumentId.current()
		&& _cloudThemeTitle.current().isEmpty();
}

void MessageShotSettings::clearCloudThemeData() {
	_cloudThemeId = uint64(0);
	_cloudThemeAccessHash = uint64(0);
	_cloudThemeDocumentId = uint64(0);
	_cloudThemeTitle = QString();
	_cloudThemeAccountId = uint64(0);
}

void MessageShotSettings::setEmbeddedTheme(int type, uint32 accentColor) {
	if (_embeddedThemeType.current() == type
		&& _embeddedThemeAccentColor.current() == accentColor
		&& isCloudThemeEmpty()) {
		return;
	}
	_embeddedThemeType = type;
	_embeddedThemeAccentColor = accentColor;
	clearCloudThemeData();
	SolarSettings::save();
}

void MessageShotSettings::setCloudTheme(uint64 accountId, uint64 id, uint64 accessHash, uint64 documentId, const QString &title) {
	if (_embeddedThemeType.current() == -1
		&& _embeddedThemeAccentColor.current() == 0
		&& _cloudThemeAccountId.current() == accountId
		&& _cloudThemeId.current() == id
		&& _cloudThemeAccessHash.current() == accessHash
		&& _cloudThemeDocumentId.current() == documentId
		&& _cloudThemeTitle.current() == title) {
		return;
	}
	_embeddedThemeType = -1;
	_embeddedThemeAccentColor = uint32(0);
	_cloudThemeAccountId = accountId;
	_cloudThemeId = id;
	_cloudThemeAccessHash = accessHash;
	_cloudThemeDocumentId = documentId;
	_cloudThemeTitle = title;
	SolarSettings::save();
}

void MessageShotSettings::clearTheme() {
	if (_embeddedThemeType.current() == -1
		&& _embeddedThemeAccentColor.current() == 0
		&& isCloudThemeEmpty()) {
		return;
	}
	_embeddedThemeType = -1;
	_embeddedThemeAccentColor = uint32(0);
	clearCloudThemeData();
	SolarSettings::save();
}

void to_json(nlohmann::json &j, const MessageShotSettings &s) {
	j = nlohmann::json{
		{"showBackground", s._showBackground.current()},
		{"showDate", s._showDate.current()},
		{"showReactions", s._showReactions.current()},
		{"showHeaderDecorations", s._showHeaderDecorations.current()},
		{"showColorfulReplies", s._showColorfulReplies.current()},
		{"revealSpoilers", s._revealSpoilers.current()},
		{"embeddedThemeType", s._embeddedThemeType.current()},
		{"embeddedThemeAccentColor", s._embeddedThemeAccentColor.current()},
		{"cloudThemeId", s._cloudThemeId.current()},
		{"cloudThemeAccessHash", s._cloudThemeAccessHash.current()},
		{"cloudThemeDocumentId", s._cloudThemeDocumentId.current()},
		{"cloudThemeTitle", s._cloudThemeTitle.current()},
		{"cloudThemeAccountId", s._cloudThemeAccountId.current()},
	};
}

void from_json(const nlohmann::json &j, MessageShotSettings &s) {
	s._showBackground = j.value("showBackground", true);
	s._showDate = j.value("showDate", false);
	s._showReactions = j.value("showReactions", false);
	s._showHeaderDecorations = j.value("showHeaderDecorations", true);
	s._showColorfulReplies = j.value("showColorfulReplies", true);
	s._revealSpoilers = j.value("revealSpoilers", true);
	s._embeddedThemeType = j.value("embeddedThemeType", j.value("themeType", -1));
	s._embeddedThemeAccentColor = j.value("embeddedThemeAccentColor", j.value("themeAccentColor", uint32(0)));
	s._cloudThemeId = j.value("cloudThemeId", uint64(0));
	s._cloudThemeAccessHash = j.value("cloudThemeAccessHash", uint64(0));
	s._cloudThemeDocumentId = j.value("cloudThemeDocumentId", uint64(0));
	s._cloudThemeTitle = j.value("cloudThemeTitle", QString());
	s._cloudThemeAccountId = j.value("cloudThemeAccountId", uint64(0));
}

SolarSettings::SolarSettings()
: _appIcon(SolarAssets::DEFAULT_ICON)
, _editedMark(Core::IsAppLaunched() ? tr::lng_edited(tr::now) : QString("edited")) {
}

SolarSettings &SolarSettings::getInstance() {
	static SolarSettings instance;
	return instance;
}

void SolarSettings::load() {
	std::ifstream file(getSettingsPath());
	if (!file.good()) {
		return;
	}

	auto &settings = getInstance();

	try {
		json p;
		file >> p;
		file.close();

		if (!p.contains("ghostModeSettings")) {
			p["ghostModeSettings"] = nlohmann::json::object({
				{"0", {
					{"sendReadMessages", p.value("sendReadMessages", true)},
					{"sendReadStories", p.value("sendReadStories", true)},
					{"sendOnlinePackets", p.value("sendOnlinePackets", true)},
					{"sendUploadProgress", p.value("sendUploadProgress", true)},
					{"sendOfflinePacketAfterOnline", p.value("sendOfflinePacketAfterOnline", false)},
					{"markReadAfterAction", p.value("markReadAfterAction", true)},
					{"useScheduledMessages", p.value("useScheduledMessages", false)},
					{"sendWithoutSound", p.value("sendWithoutSound", false)}
				}}
			});
			p["useGlobalGhostMode"] = true;

			LOG(("SolarGramSettings: migrated ghost mode settings to per-account format"));
		}

		try {
			from_json(p, settings);
		} catch (...) {
			LOG(("SolarGramSettings: failed to parse settings file"));
		}
	} catch (...) {
		LOG(("SolarGramSettings: failed to read settings file (not json-like)"));
	}

	if (cGhost()) {
		auto &ghost = SolarSettings::ghost();
		ghost._sendReadMessages = false;
		ghost._sendReadStories = false;
		ghost._sendOnlinePackets = false;
		ghost._sendUploadProgress = false;
		ghost._sendOfflinePacketAfterOnline = true;
	}

	settings.validate();
}

void SolarSettings::save() {
	auto &settings = getInstance();
	json p = settings;

	std::ofstream file;
	file.open(getSettingsPath());
	file << p.dump(4);
	file.close();
}

void SolarSettings::reset() {
	auto &inst = getInstance();
	inst = SolarSettings();
	updateSpoofSnapshot(
		inst._deviceSpoofEnabled.current(),
		inst._deviceSpoofModel.current(),
		inst._deviceSpoofSystem.current(),
		inst._deviceSpoofAppVersion.current(),
		inst._deviceSpoofLangCode.current());
	inst._hiddenChatsVersion = 1;
	FiltersCacheController::rebuildCache();
	FiltersCacheController::fireUpdate();
	save();
	repaintApp();
}

GhostModeAccountSettings &SolarSettings::ghost(not_null<Main::Session*> session) {
	return ghost(session->userId().bare);
}

GhostModeAccountSettings &SolarSettings::ghost(uint64 userId) {
	auto &settings = getInstance();
	auto overriddenId = settings.getOverriddenGhostUserId(userId);

	auto it = settings._ghostAccounts.find(overriddenId);
	if (it == settings._ghostAccounts.end()) {
		auto account = std::make_unique<GhostModeAccountSettings>();
		it = settings._ghostAccounts.emplace(overriddenId, std::move(account)).first;
	}

	return *it->second;
}

GhostModeAccountSettings &SolarSettings::ghost() {
	if (const auto window = Core::App().activeWindow()) {
		if (const auto session = window->maybeSession()) {
			return ghost(session);
		}
	}
	return ghost(0);
}

void SolarSettings::setUseGlobalGhostMode(bool val) {
	if (_useGlobalGhostMode.current() == val) return;
	_useGlobalGhostMode = val;
	save();
}

void SolarSettings::addShadowBan(int64 id) {
	if (_shadowBanIds.insert(id).second) {
		FiltersCacheController::rebuildCache();
		FiltersCacheController::fireUpdate();
		save();
	}
}

void SolarSettings::removeShadowBan(int64 id) {
	if (_shadowBanIds.erase(id) > 0) {
		FiltersCacheController::rebuildCache();
		FiltersCacheController::fireUpdate();
		save();
	}
}

void SolarSettings::setSavingExcluded(int64 dialogId, bool excluded) {
	if (excluded) {
		if (_savingExclusions.insert(dialogId).second) {
			_savingExclusionsVersion = _savingExclusionsVersion.current() + 1;
			save();
		}
	} else {
		if (_savingExclusions.erase(dialogId) > 0) {
			_savingExclusionsVersion = _savingExclusionsVersion.current() + 1;
			save();
		}
	}
}

void SolarSettings::setReadExclusion(int64 dialogId, ChatExclusionMode mode) {
	if (mode == ChatExclusionMode::GlobalDefault) {
		if (_readExclusions.erase(dialogId) > 0) {
			save();
		}
	} else {
		auto &slot = _readExclusions[dialogId];
		if (slot != mode) {
			slot = mode;
			save();
		}
	}
}

ChatExclusionMode SolarSettings::readExclusion(int64 dialogId) const {
	const auto it = _readExclusions.find(dialogId);
	return (it != _readExclusions.end()) ? it->second : ChatExclusionMode::GlobalDefault;
}

bool SolarSettings::effectiveShouldRead(not_null<Main::Session*> session, int64 dialogId) const {
	return effectiveShouldRead(session->userId().bare, dialogId);
}

bool SolarSettings::effectiveShouldRead(uint64 userId, int64 dialogId) const {
	switch (readExclusion(dialogId)) {
	case ChatExclusionMode::ForceAllow:
		return true;
	case ChatExclusionMode::ForceBlock:
		return false;
	case ChatExclusionMode::GlobalDefault:
		break;
	}
	switch (perChatGhostPolicy(dialogId).readMessages) {
	case ChatExclusionMode::ForceAllow:
		return true;
	case ChatExclusionMode::ForceBlock:
		return false;
	case ChatExclusionMode::GlobalDefault:
		break;
	}
	return ghost(userId).sendReadMessages();
}

void SolarSettings::setTypingExclusion(int64 dialogId, ChatExclusionMode mode) {
	if (mode == ChatExclusionMode::GlobalDefault) {
		if (_typingExclusions.erase(dialogId) > 0) {
			save();
		}
	} else {
		auto &slot = _typingExclusions[dialogId];
		if (slot != mode) {
			slot = mode;
			save();
		}
	}
}

ChatExclusionMode SolarSettings::typingExclusion(int64 dialogId) const {
	const auto it = _typingExclusions.find(dialogId);
	return (it != _typingExclusions.end()) ? it->second : ChatExclusionMode::GlobalDefault;
}

bool SolarSettings::effectiveShouldSendTyping(not_null<Main::Session*> session, int64 dialogId) const {
	return effectiveShouldSendTyping(session->userId().bare, dialogId);
}

bool SolarSettings::effectiveShouldSendTyping(uint64 userId, int64 dialogId) const {
	switch (typingExclusion(dialogId)) {
	case ChatExclusionMode::ForceAllow:
		return true;
	case ChatExclusionMode::ForceBlock:
		return false;
	case ChatExclusionMode::GlobalDefault:
		break;
	}
	switch (perChatGhostPolicy(dialogId).uploadProgress) {
	case ChatExclusionMode::ForceAllow:
		return true;
	case ChatExclusionMode::ForceBlock:
		return false;
	case ChatExclusionMode::GlobalDefault:
		break;
	}
	return ghost(userId).sendUploadProgress();
}

void SolarSettings::setPerChatGhostPolicy(
		int64 dialogId,
		const PerChatGhostPolicy &policy) {
	const auto isDefault = (policy == PerChatGhostPolicy{});
	if (isDefault) {
		if (_perChatGhost.erase(dialogId) > 0) {
			save();
		}
	} else {
		auto &slot = _perChatGhost[dialogId];
		if (slot != policy) {
			slot = policy;
			save();
		}
	}
}

PerChatGhostPolicy SolarSettings::perChatGhostPolicy(int64 dialogId) const {
	const auto it = _perChatGhost.find(dialogId);
	return (it != _perChatGhost.end()) ? it->second : PerChatGhostPolicy{};
}

bool SolarSettings::effectiveShouldReadStories(
		not_null<Main::Session*> session,
		int64 dialogId) const {
	switch (perChatGhostPolicy(dialogId).readStories) {
	case ChatExclusionMode::ForceAllow:
		return true;
	case ChatExclusionMode::ForceBlock:
		return false;
	case ChatExclusionMode::GlobalDefault:
		break;
	}
	return ghost(session).sendReadStories();
}

bool SolarSettings::effectiveShouldSendOnline(
		not_null<Main::Session*> session,
		int64 dialogId) const {
	switch (perChatGhostPolicy(dialogId).onlinePackets) {
	case ChatExclusionMode::ForceAllow:
		return true;
	case ChatExclusionMode::ForceBlock:
		return false;
	case ChatExclusionMode::GlobalDefault:
		break;
	}
	return ghost(session).sendOnlinePackets();
}

bool SolarSettings::effectiveShouldOfflineAfterOnline(
		not_null<Main::Session*> session,
		int64 dialogId) const {
	switch (perChatGhostPolicy(dialogId).offlineAfterOnline) {
	case ChatExclusionMode::ForceAllow:
		return true;
	case ChatExclusionMode::ForceBlock:
		return false;
	case ChatExclusionMode::GlobalDefault:
		break;
	}
	return ghost(session).sendOfflinePacketAfterOnline();
}

void SolarSettings::setPeerWatched(int64 dialogId, bool watched) {
	if (watched) {
		if (_watchedPeerIds.insert(dialogId).second) {
			save();
		}
	} else if (_watchedPeerIds.erase(dialogId) > 0) {
		save();
	}
}

namespace {

bool updateTrackedIdSet(
		std::unordered_set<int64> &target,
		int64 id,
		bool add,
		rpl::variable<int> &version) {
	if (!id) {
		return false;
	}
	const auto changed = add
		? target.insert(id).second
		: (target.erase(id) > 0);
	if (changed) {
		version = version.current() + 1;
	}
	return changed;
}

bool clearTrackedIdSet(
		std::unordered_set<int64> &target,
		rpl::variable<int> &version) {
	if (target.empty()) {
		return false;
	}
	target.clear();
	version = version.current() + 1;
	return true;
}

} // namespace

void SolarSettings::setStoryHiddenForUser(int64 userId, bool hidden) {
	if (updateTrackedIdSet(_storyHiddenUserIds, userId, hidden, _storyPrivacyVersion)) {
		save();
	}
}

void SolarSettings::clearStoryHiddenUsers() {
	if (clearTrackedIdSet(_storyHiddenUserIds, _storyPrivacyVersion)) {
		save();
	}
}

void SolarSettings::setStoryFeedHidden(int64 peerId, bool hidden) {
	if (updateTrackedIdSet(_storyFeedHiddenPeerIds, peerId, hidden, _storyFeedHiddenVersion)) {
		save();
	}
}

void SolarSettings::clearStoryFeedHidden() {
	if (clearTrackedIdSet(_storyFeedHiddenPeerIds, _storyFeedHiddenVersion)) {
		save();
	}
}

void SolarSettings::setChannelBlockedUser(int64 userId, bool blocked) {
	if (updateTrackedIdSet(_channelBlockedUserIds, userId, blocked, _channelBlockedVersion)) {
		save();
	}
}

void SolarSettings::clearChannelBlockedUsers() {
	if (clearTrackedIdSet(_channelBlockedUserIds, _channelBlockedVersion)) {
		save();
	}
}

void SolarSettings::pushRecentChat(int64 peerId) {
	if (!peerId) {
		return;
	}
	constexpr auto kMaxRecentChats = size_t(50);
	std::erase(_recentChatIds, peerId);
	_recentChatIds.insert(_recentChatIds.begin(), peerId);
	if (_recentChatIds.size() > kMaxRecentChats) {
		_recentChatIds.resize(kMaxRecentChats);
	}
	save();
}

void SolarSettings::clearRecentChats() {
	if (_recentChatIds.empty()) {
		return;
	}
	_recentChatIds.clear();
	save();
}

void SolarSettings::setDuressPin(const QString &pin) {
	_duressPinHash = hashPartisanPin(pin);
	save();
}

void SolarSettings::clearDuressPin() {
	if (_duressPinHash.current().isEmpty()) {
		return;
	}
	_duressPinHash = QString();
	_decoyModeActive = false;
	save();
}

bool SolarSettings::verifyDuressPin(const QString &pin) const {
	const auto &stored = _duressPinHash.current();
	if (stored.isEmpty() || pin.isEmpty()) {
		return false;
	}
	return stored == hashPartisanPin(pin);
}

void SolarSettings::setDecoyModeActive(bool active) {
	if (_decoyModeActive.current() == active) {
		return;
	}
	_decoyModeActive = active;
	repaintApp();
}

void SolarSettings::setHiddenChatsPin(const QString &pin) {
	_hiddenChatsPinHash = hashPartisanPin(pin);
	save();
}

void SolarSettings::clearHiddenChatsPin() {
	if (_hiddenChatsPinHash.current().isEmpty() && _hiddenChatIds.empty()) {
		return;
	}
	_hiddenChatsPinHash = QString();
	_hiddenChatIds.clear();
	_hiddenChatsRevealed = false;
	_hiddenChatsVersion = _hiddenChatsVersion.current() + 1;
	FiltersCacheController::rebuildCache();
	FiltersCacheController::fireUpdate();
	save();
}

bool SolarSettings::verifyHiddenChatsPin(const QString &pin) const {
	const auto &stored = _hiddenChatsPinHash.current();
	if (stored.isEmpty() || pin.isEmpty()) {
		return false;
	}
	return stored == hashPartisanPin(pin);
}

void SolarSettings::hideChat(int64 id) {
	if (_hiddenChatIds.insert(id).second) {
		_hiddenChatsVersion = _hiddenChatsVersion.current() + 1;
		FiltersCacheController::rebuildCache();
		FiltersCacheController::fireUpdate();
		save();
	}
}

void SolarSettings::unhideChat(int64 id) {
	if (_hiddenChatIds.erase(id) > 0) {
		_hiddenChatsVersion = _hiddenChatsVersion.current() + 1;
		FiltersCacheController::rebuildCache();
		FiltersCacheController::fireUpdate();
		save();
	}
}

void SolarSettings::setHiddenChatsRevealed(bool revealed) {
	if (_hiddenChatsRevealed.current() == revealed) {
		return;
	}
	_hiddenChatsRevealed = revealed;
	_hiddenChatsVersion = _hiddenChatsVersion.current() + 1;
	FiltersCacheController::rebuildCache();
	FiltersCacheController::fireUpdate();
	repaintApp();
}

QJsonObject SolarSettings::exportToJsonObject() const {
	nlohmann::json j = *this;
	j.erase("duressPinHash");
	j.erase("hiddenChatsPinHash");
	const auto raw = QByteArray::fromStdString(j.dump());
	return QJsonDocument::fromJson(raw).object();
}

bool SolarSettings::importFromJsonObject(const QJsonObject &obj) {
	try {
		const auto raw = QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString();
		const auto j = nlohmann::json::parse(raw);
		const auto savedDuress = _duressPinHash.current();
		const auto savedHiddenPin = _hiddenChatsPinHash.current();
		from_json(j, *this);
		if (!j.contains("duressPinHash") && !savedDuress.isEmpty()) {
			_duressPinHash = savedDuress;
		}
		if (!j.contains("hiddenChatsPinHash") && !savedHiddenPin.isEmpty()) {
			_hiddenChatsPinHash = savedHiddenPin;
		}
		validate();
		_hiddenChatsVersion = _hiddenChatsVersion.current() + 1;
		updateSpoofSnapshot(
			_deviceSpoofEnabled.current(),
			_deviceSpoofModel.current(),
			_deviceSpoofSystem.current(),
			_deviceSpoofAppVersion.current(),
			_deviceSpoofLangCode.current());
		FiltersCacheController::rebuildCache();
		FiltersCacheController::fireUpdate();
		save();
		repaintApp();
		return true;
	} catch (...) {
		return false;
	}
}

void SolarSettings::validate() {
	SolarSettings defaults;
	auto modified = false;

	auto validateRange = [&](auto &var, auto min, auto max, const auto &defaultVar) {
		if (var.current() < min || var.current() > max) {
			var = defaultVar.current();
			modified = true;
		}
	};

	auto validateEnum = [&](auto &var, const auto &defaultVar, int max = 2) {
		auto intVal = static_cast<int>(var.current());
		if (intVal < 0 || intVal > max) {
			var = defaultVar.current();
			modified = true;
		}
	};

	validateEnum(_showPeerId, defaults._showPeerId);
	validateEnum(_channelBottomButton, defaults._channelBottomButton);
	validateEnum(_showReactionsPanelInContextMenu, defaults._showReactionsPanelInContextMenu);
	validateEnum(_showViewsPanelInContextMenu, defaults._showViewsPanelInContextMenu);
	validateEnum(_showHideMessageInContextMenu, defaults._showHideMessageInContextMenu);
	validateEnum(_showUserMessagesInContextMenu, defaults._showUserMessagesInContextMenu);
	validateEnum(_showMessageDetailsInContextMenu, defaults._showMessageDetailsInContextMenu);
	validateEnum(_showRepeatMessageInContextMenu, defaults._showRepeatMessageInContextMenu);
	validateEnum(_showAddFilterInContextMenu, defaults._showAddFilterInContextMenu);

	validateEnum(_translationProvider, defaults._translationProvider, 6);
	if ((_translationProvider.current() == TranslationProvider::Native)
		&& !Platform::IsTranslateProviderAvailable()) {
		_translationProvider = defaults._translationProvider.current();
		modified = true;
	}

	validateRange(_messageBubbleRadius, 0, 16, defaults._messageBubbleRadius);
	validateRange(_wideMultiplier, 0.5, 4.0, defaults._wideMultiplier);
	validateRange(_avatarCorners, 0, SolarUiSettings::kMaxAvatarCorners, defaults._avatarCorners);
	validateEnum(_deletedSaveMode, defaults._deletedSaveMode, 2);
	validateEnum(_duressAction, defaults._duressAction, 1);
	validateRange(_attachmentsMaxSizeMb, 128, 65536, defaults._attachmentsMaxSizeMb);

	const auto &icon = _appIcon.current();
	if (icon != SolarAssets::DEFAULT_ICON
		&& icon != SolarAssets::EMBLEM_ICON
		&& icon != SolarAssets::OBSIDIAN_ICON
		&& icon != SolarAssets::NOVA_ICON) {
		_appIcon = defaults._appIcon.current();
		modified = true;
	}

	const auto embeddedType = _messageShotSettings._embeddedThemeType.current();
	auto embeddedTypeValid = (embeddedType == -1) || (embeddedType >= 0 && embeddedType <= 3); // from Window::Theme::EmbeddedType::DayBlue to Window::Theme::EmbeddedType::NightGreen
	if (!embeddedTypeValid) {
		_messageShotSettings._embeddedThemeType = defaults._messageShotSettings._embeddedThemeType.current();
		_messageShotSettings._embeddedThemeAccentColor = defaults._messageShotSettings._embeddedThemeAccentColor.current();
		modified = true;
	}

	if (modified) {
		save();
	}
}

void SolarSettings::setDeletedSaveMode(DeletedSaveMode val) {
	if (_deletedSaveMode.current() == val) return;
	_deletedSaveMode = val;
	_saveDeletedMessages = (val != DeletedSaveMode::Off);
	save();
}

void SolarSettings::setSaveDeletedMessages(bool val) {
	const auto nextMode = val
		? ((_deletedSaveMode.current() == DeletedSaveMode::Off)
			? DeletedSaveMode::Persistent
			: _deletedSaveMode.current())
		: DeletedSaveMode::Off;
	if (_saveDeletedMessages.current() == val && _deletedSaveMode.current() == nextMode) {
		return;
	}
	_saveDeletedMessages = val;
	_deletedSaveMode = nextMode;
	save();
}

void SolarSettings::setSaveMessagesHistory(bool val) {
	if (_saveMessagesHistory.current() == val) return;
	_saveMessagesHistory = val;
	save();
}

void SolarSettings::setSaveForBots(bool val) {
	if (_saveForBots.current() == val) return;
	_saveForBots = val;
	save();
}

void SolarSettings::setFiltersEnabled(bool val) {
	if (_filtersEnabled.current() == val) return;
	_filtersEnabled = val;
	save();
}

void SolarSettings::setFiltersEnabledInChats(bool val) {
	if (_filtersEnabledInChats.current() == val) return;
	_filtersEnabledInChats = val;
	save();
}

void SolarSettings::setHideFromBlocked(bool val) {
	if (_hideFromBlocked.current() == val) return;
	_hideFromBlocked = val;
	save();
}

void SolarSettings::setSemiTransparentDeletedMessages(bool val) {
	if (_semiTransparentDeletedMessages.current() == val) return;
	_semiTransparentDeletedMessages = val;
	save();
}

void SolarSettings::setDisableAds(bool val) {
	if (_disableAds.current() == val) return;
	_disableAds = val;
	save();
}

void SolarSettings::setDisableStories(bool val) {
	if (_disableStories.current() == val) return;
	_disableStories = val;
	save();
}

void SolarSettings::setDisableCustomBackgrounds(bool val) {
	if (_disableCustomBackgrounds.current() == val) return;
	_disableCustomBackgrounds = val;
	save();
}

void SolarSettings::setHidePremiumStatuses(bool val) {
	if (_hidePremiumStatuses.current() == val) return;
	_hidePremiumStatuses = val;
	save();
}

void SolarSettings::setShowOnlyAddedEmojisAndStickers(bool val) {
	if (_showOnlyAddedEmojisAndStickers.current() == val) return;
	_showOnlyAddedEmojisAndStickers = val;
	save();
}

void SolarSettings::setCollapseSimilarChannels(bool val) {
	if (_collapseSimilarChannels.current() == val) return;
	_collapseSimilarChannels = val;
	save();
}

void SolarSettings::setHideSimilarChannels(bool val) {
	if (_hideSimilarChannels.current() == val) return;
	_hideSimilarChannels = val;
	save();
}

void SolarSettings::setMessageBubbleRadius(int val) {
	if (_messageBubbleRadius.current() == val) return;
	_messageBubbleRadius = val;
	save();
}

void SolarSettings::setWideMultiplier(double val) {
	if (_wideMultiplier.current() == val) return;
	_wideMultiplier = val;
	// doesn't work because it should be set before style::StartManager()
	// SolarUiSettings::setWideMultiplier(val);
	// repaintApp();
	save();
}

void SolarSettings::setSpoofWebviewAsAndroid(bool val) {
	if (_spoofWebviewAsAndroid.current() == val) return;
	_spoofWebviewAsAndroid = val;
	save();
}

void SolarSettings::setDisableOpenLinkWarning(bool val) {
	if (_disableOpenLinkWarning.current() == val) return;
	_disableOpenLinkWarning = val;
	save();
}

void SolarSettings::setIncreaseWebviewHeight(bool val) {
	if (_increaseWebviewHeight.current() == val) return;
	_increaseWebviewHeight = val;
	save();
}

void SolarSettings::setIncreaseWebviewWidth(bool val) {
	if (_increaseWebviewWidth.current() == val) return;
	_increaseWebviewWidth = val;
	save();
}

void SolarSettings::setMaterialSwitches(bool val) {
	if (_materialSwitches.current() == val) return;
	_materialSwitches = val;
	SolarUiSettings::setMaterialSwitches(val);
	repaintApp();
	save();
}

void SolarSettings::setRemoveMessageTail(bool val) {
	if (_removeMessageTail.current() == val) return;
	_removeMessageTail = val;
	save();
}

void SolarSettings::setDisableNotificationsDelay(bool val) {
	if (_disableNotificationsDelay.current() == val) return;
	_disableNotificationsDelay = val;
	save();
}

void SolarSettings::setLocalPremium(bool val) {
	if (_localPremium.current() == val) return;
	_localPremium = val;
	save();
}

void SolarSettings::setShowChannelReactions(bool val) {
	if (_showChannelReactions.current() == val) return;
	_showChannelReactions = val;
	save();
}

void SolarSettings::setShowGroupReactions(bool val) {
	if (_showGroupReactions.current() == val) return;
	_showGroupReactions = val;
	save();
}

void SolarSettings::setShowPrivateChatReactions(bool val) {
	if (_showPrivateChatReactions.current() == val) return;
	_showPrivateChatReactions = val;
	save();
}

void SolarSettings::setAppIcon(const QString &val) {
	if (_appIcon.current() == val) return;
	_appIcon = val;
	save();
}

void SolarSettings::setSimpleQuotesAndReplies(bool val) {
	if (_simpleQuotesAndReplies.current() == val) return;
	_simpleQuotesAndReplies = val;
	save();
}

void SolarSettings::setHideFastShare(bool val) {
	if (_hideFastShare.current() == val) return;
	_hideFastShare = val;
	save();
}

void SolarSettings::setReplaceBottomInfoWithIcons(bool val) {
	if (_replaceBottomInfoWithIcons.current() == val) return;
	_replaceBottomInfoWithIcons = val;
	save();
}

void SolarSettings::setDeletedMark(const QString &val) {
	if (_deletedMark.current() == val) return;
	_deletedMark = val;
	save();
}

void SolarSettings::setEditedMark(const QString &val) {
	if (_editedMark.current() == val) return;
	_editedMark = val;
	save();
}

void SolarSettings::setUnlimitedRecentStickers(bool val) {
	if (_unlimitedRecentStickers.current() == val) return;
	_unlimitedRecentStickers = val;
	save();
}

void SolarSettings::setShowReactionsPanelInContextMenu(ContextMenuVisibility val) {
	if (_showReactionsPanelInContextMenu.current() == val) return;
	_showReactionsPanelInContextMenu = val;
	save();
}

void SolarSettings::setShowViewsPanelInContextMenu(ContextMenuVisibility val) {
	if (_showViewsPanelInContextMenu.current() == val) return;
	_showViewsPanelInContextMenu = val;
	save();
}

void SolarSettings::setShowHideMessageInContextMenu(ContextMenuVisibility val) {
	if (_showHideMessageInContextMenu.current() == val) return;
	_showHideMessageInContextMenu = val;
	save();
}

void SolarSettings::setShowUserMessagesInContextMenu(ContextMenuVisibility val) {
	if (_showUserMessagesInContextMenu.current() == val) return;
	_showUserMessagesInContextMenu = val;
	save();
}

void SolarSettings::setShowMessageDetailsInContextMenu(ContextMenuVisibility val) {
	if (_showMessageDetailsInContextMenu.current() == val) return;
	_showMessageDetailsInContextMenu = val;
	save();
}

void SolarSettings::setShowRepeatMessageInContextMenu(ContextMenuVisibility val) {
	if (_showRepeatMessageInContextMenu.current() == val) return;
	_showRepeatMessageInContextMenu = val;
	save();
}

void SolarSettings::setShowAddFilterInContextMenu(ContextMenuVisibility val) {
	if (_showAddFilterInContextMenu.current() == val) return;
	_showAddFilterInContextMenu = val;
	save();
}

void SolarSettings::setShowAttachButtonInMessageField(bool val) {
	if (_showAttachButtonInMessageField.current() == val) return;
	_showAttachButtonInMessageField = val;
	save();
}

void SolarSettings::setShowCommandsButtonInMessageField(bool val) {
	if (_showCommandsButtonInMessageField.current() == val) return;
	_showCommandsButtonInMessageField = val;
	save();
}

void SolarSettings::setShowEmojiButtonInMessageField(bool val) {
	if (_showEmojiButtonInMessageField.current() == val) return;
	_showEmojiButtonInMessageField = val;
	save();
}

void SolarSettings::setShowMicrophoneButtonInMessageField(bool val) {
	if (_showMicrophoneButtonInMessageField.current() == val) return;
	_showMicrophoneButtonInMessageField = val;
	save();
}

void SolarSettings::setShowAutoDeleteButtonInMessageField(bool val) {
	if (_showAutoDeleteButtonInMessageField.current() == val) return;
	_showAutoDeleteButtonInMessageField = val;
	save();
}

void SolarSettings::setShowGiftButtonInMessageField(bool val) {
	if (_showGiftButtonInMessageField.current() == val) return;
	_showGiftButtonInMessageField = val;
	save();
}

void SolarSettings::setShowAiEditorButtonInMessageField(bool val) {
	if (_showAiEditorButtonInMessageField.current() == val) return;
	_showAiEditorButtonInMessageField = val;
	save();
}

void SolarSettings::setShowAttachPopup(bool val) {
	if (_showAttachPopup.current() == val) return;
	_showAttachPopup = val;
	save();
}

void SolarSettings::setShowEmojiPopup(bool val) {
	if (_showEmojiPopup.current() == val) return;
	_showEmojiPopup = val;
	save();
}

void SolarSettings::setShowMyProfileInDrawer(bool val) {
	if (_showMyProfileInDrawer.current() == val) return;
	_showMyProfileInDrawer = val;
	save();
}

void SolarSettings::setShowBotsInDrawer(bool val) {
	if (_showBotsInDrawer.current() == val) return;
	_showBotsInDrawer = val;
	save();
}

void SolarSettings::setShowNewGroupInDrawer(bool val) {
	if (_showNewGroupInDrawer.current() == val) return;
	_showNewGroupInDrawer = val;
	save();
}

void SolarSettings::setShowNewChannelInDrawer(bool val) {
	if (_showNewChannelInDrawer.current() == val) return;
	_showNewChannelInDrawer = val;
	save();
}

void SolarSettings::setShowContactsInDrawer(bool val) {
	if (_showContactsInDrawer.current() == val) return;
	_showContactsInDrawer = val;
	save();
}

void SolarSettings::setShowCallsInDrawer(bool val) {
	if (_showCallsInDrawer.current() == val) return;
	_showCallsInDrawer = val;
	save();
}

void SolarSettings::setShowSavedMessagesInDrawer(bool val) {
	if (_showSavedMessagesInDrawer.current() == val) return;
	_showSavedMessagesInDrawer = val;
	save();
}

void SolarSettings::setShowLReadToggleInDrawer(bool val) {
	if (_showLReadToggleInDrawer.current() == val) return;
	_showLReadToggleInDrawer = val;
	save();
}

void SolarSettings::setShowSReadToggleInDrawer(bool val) {
	if (_showSReadToggleInDrawer.current() == val) return;
	_showSReadToggleInDrawer = val;
	save();
}

void SolarSettings::setShowNightModeToggleInDrawer(bool val) {
	if (_showNightModeToggleInDrawer.current() == val) return;
	_showNightModeToggleInDrawer = val;
	save();
}

void SolarSettings::setShowGhostToggleInDrawer(bool val) {
	if (_showGhostToggleInDrawer.current() == val) return;
	_showGhostToggleInDrawer = val;
	save();
}

void SolarSettings::setShowStreamerToggleInDrawer(bool val) {
	if (_showStreamerToggleInDrawer.current() == val) return;
	_showStreamerToggleInDrawer = val;
	save();
}

void SolarSettings::setShowGhostToggleInTray(bool val) {
	if (_showGhostToggleInTray.current() == val) return;
	_showGhostToggleInTray = val;
	save();
}

void SolarSettings::setShowStreamerToggleInTray(bool val) {
	if (_showStreamerToggleInTray.current() == val) return;
	_showStreamerToggleInTray = val;
	save();
}

void SolarSettings::setMonoFont(const QString &val) {
	if (_monoFont.current() == val) return;
	_monoFont = val;
	// doesn't work because `static const auto family = ...`
	// SolarUiSettings::setMonoFont(val);
	// repaintApp();
	save();
}

void SolarSettings::setHideNotificationCounters(bool val) {
	if (_hideNotificationCounters.current() == val) return;
	_hideNotificationCounters = val;
	save();
}

void SolarSettings::setHideNotificationBadge(bool val) {
	if (_hideNotificationBadge.current() == val) return;
	_hideNotificationBadge = val;
	Core::App().refreshApplicationIcon();
	Core::App().tray().updateIconCounters();
	Core::App().domain().notifyUnreadBadgeChanged();
	save();
}

void SolarSettings::setHideAllChatsFolder(bool val) {
	if (_hideAllChatsFolder.current() == val) return;
	_hideAllChatsFolder = val;
	save();
}

void SolarSettings::setChannelBottomButton(ChannelBottomButton val) {
	if (_channelBottomButton.current() == val) return;
	_channelBottomButton = val;
	save();
}

void SolarSettings::setQuickAdminShortcuts(bool val) {
	if (_quickAdminShortcuts.current() == val) return;
	_quickAdminShortcuts = val;
	save();
}

void SolarSettings::setDisableGreetingSticker(bool val) {
	if (_disableGreetingSticker.current() == val) return;
	_disableGreetingSticker = val;
	save();
}

void SolarSettings::setShowPeerId(PeerIdDisplay val) {
	if (_showPeerId.current() == val) return;
	_showPeerId = val;
	save();
}

void SolarSettings::setShowMessageSeconds(bool val) {
	if (_showMessageSeconds.current() == val) return;
	_showMessageSeconds = val;
	save();
}

void SolarSettings::setShowMessageShot(bool val) {
	if (_showMessageShot.current() == val) return;
	_showMessageShot = val;
	save();
}

void SolarSettings::setFilterZalgo(bool val) {
	if (_filterZalgo.current() == val) return;
	_filterZalgo = val;
	save();
}

void SolarSettings::setStickerConfirmation(bool val) {
	if (_stickerConfirmation.current() == val) return;
	_stickerConfirmation = val;
	save();
}

void SolarSettings::setGifConfirmation(bool val) {
	if (_gifConfirmation.current() == val) return;
	_gifConfirmation = val;
	save();
}

void SolarSettings::setVoiceConfirmation(bool val) {
	if (_voiceConfirmation.current() == val) return;
	_voiceConfirmation = val;
	save();
}

void SolarSettings::setRoundConfirmation(bool val) {
	if (_roundConfirmation.current() == val) return;
	_roundConfirmation = val;
	save();
}

void SolarSettings::setTranslationProvider(TranslationProvider val) {
	if ((val == TranslationProvider::Native)
		&& !Platform::IsTranslateProviderAvailable()) {
		val = TranslationProvider::Telegram;
	}
	if (_translationProvider.current() == val) return;
	_translationProvider = val;
	if (const auto manager = Solar::Translator::TranslateManager::currentInstance()) {
		manager->resetCache();
	}
	save();
}

void SolarSettings::setAdaptiveCoverColor(bool val) {
	if (_adaptiveCoverColor.current() == val) return;
	_adaptiveCoverColor = val;
	save();
}

void SolarSettings::setImproveLinkPreviews(bool val) {
	if (_improveLinkPreviews.current() == val) return;
	_improveLinkPreviews = val;
	save();
}

void SolarSettings::setCrashReporting(bool val) {
	if (_crashReporting.current() == val) return;
	_crashReporting = val;
	save();
}

void SolarSettings::setAvatarCorners(int val) {
	if (_avatarCorners.current() == val) return;
	_avatarCorners = val;
	SolarUiSettings::setAvatarCorners(val);
	save();
}

void SolarSettings::setSingleCornerRadius(bool val) {
	if (_singleCornerRadius.current() == val) return;
	_singleCornerRadius = val;
	repaintApp();
	save();
}

void SolarSettings::setStreamerMode(bool val) {
	if (_streamerMode.current() == val) return;
	_streamerMode = val;
	SolarFeatures::StreamerMode::apply(val);
	save();
}

void SolarSettings::setAutoStreamerMode(bool val) {
	if (_autoStreamerMode.current() == val) return;
	_autoStreamerMode = val;
	SolarFeatures::StreamerMode::apply(_streamerMode.current());
	save();
}

void SolarSettings::setAutoStripTrackingOnSend(bool val) {
	if (_autoStripTrackingOnSend.current() == val) return;
	_autoStripTrackingOnSend = val;
	save();
}

void SolarSettings::setAntiCaps(bool val) {
	if (_antiCaps.current() == val) return;
	_antiCaps = val;
	save();
}

void SolarSettings::setLoginCodeLeakGuard(bool val) {
	if (_loginCodeLeakGuard.current() == val) return;
	_loginCodeLeakGuard = val;
	save();
}

void SolarSettings::setEternalOnline(bool val) {
	if (_eternalOnline.current() == val) return;
	_eternalOnline = val;
	save();
}

void SolarSettings::setForceRelayCalls(bool val) {
	if (_forceRelayCalls.current() == val) return;
	_forceRelayCalls = val;
	save();
}

void SolarSettings::setCallConfirmation(bool val) {
	if (_callConfirmation.current() == val) return;
	_callConfirmation = val;
	save();
}

void SolarSettings::setExactCounters(bool val) {
	if (_exactCounters.current() == val) return;
	_exactCounters = val;
	repaintApp();
	save();
}

void SolarSettings::setAlwaysOnTop(bool val) {
	if (_alwaysOnTop.current() == val) return;
	_alwaysOnTop = val;
	Core::App().enumerateWindows([=](not_null<Window::Controller*> window) {
		window->applyAlwaysOnTop(val);
	});
	save();
}

void SolarSettings::setSolarBrainEnabled(bool val) {
	if (_solarBrainEnabled.current() == val) return;
	_solarBrainEnabled = val;
	save();
}

void SolarSettings::setSolarBrainEndpoint(const QString &val) {
	if (_solarBrainEndpoint.current() == val) return;
	_solarBrainEndpoint = val;
	save();
}

void SolarSettings::setSolarBrainModel(const QString &val) {
	if (_solarBrainModel.current() == val) return;
	_solarBrainModel = val;
	save();
}

void SolarSettings::setSaveMediaPrivate(bool val) {
	if (_saveMediaPrivate.current() == val) return;
	_saveMediaPrivate = val;
	save();
}

void SolarSettings::setSaveMediaGroups(bool val) {
	if (_saveMediaGroups.current() == val) return;
	_saveMediaGroups = val;
	save();
}

void SolarSettings::setSaveMediaChannels(bool val) {
	if (_saveMediaChannels.current() == val) return;
	_saveMediaChannels = val;
	save();
}

void SolarSettings::setSaveMediaBots(bool val) {
	if (_saveMediaBots.current() == val) return;
	_saveMediaBots = val;
	save();
}

void SolarSettings::setSaveMediaStickers(bool val) {
	if (_saveMediaStickers.current() == val) return;
	_saveMediaStickers = val;
	save();
}

void SolarSettings::setAttachmentsMaxSizeMb(int val) {
	if (_attachmentsMaxSizeMb.current() == val) return;
	_attachmentsMaxSizeMb = val;
	save();
}

void SolarSettings::setKeepOneViewMedia(bool val) {
	if (_keepOneViewMedia.current() == val) return;
	_keepOneViewMedia = val;
	save();
}

void SolarSettings::setSpySaveReadMarks(bool val) {
	if (_spySaveReadMarks.current() == val) return;
	_spySaveReadMarks = val;
	save();
}

void SolarSettings::setSpySaveLocalOnline(bool val) {
	if (_spySaveLocalOnline.current() == val) return;
	_spySaveLocalOnline = val;
	save();
}

void SolarSettings::setDisplayGhostStatus(bool val) {
	if (_displayGhostStatus.current() == val) return;
	_displayGhostStatus = val;
	repaintApp();
	save();
}

void SolarSettings::setGhostModeShortcut(const QString &val) {
	if (_ghostModeShortcut.current() == val) return;
	_ghostModeShortcut = val;
	save();
}

void SolarSettings::setUploadSpeedBoost(bool val) {
	if (_uploadSpeedBoost.current() == val) return;
	_uploadSpeedBoost = val;
	save();
}

void SolarSettings::setCopyUsernameAsLink(bool val) {
	if (_copyUsernameAsLink.current() == val) return;
	_copyUsernameAsLink = val;
	save();
}

void SolarSettings::setWalMode(bool val) {
	if (_walMode.current() == val) return;
	_walMode = val;
	save();
}

void SolarSettings::setMainFont(const QString &val) {
	if (_mainFont.current() == val) return;
	_mainFont = val;
	save();
}

void SolarSettings::setDuressAction(DuressAction val) {
	if (_duressAction.current() == val) return;
	_duressAction = val;
	save();
}

void SolarSettings::setGlobalHotkeysEnabled(bool val) {
	if (_globalHotkeysEnabled.current() == val) return;
	_globalHotkeysEnabled = val;
	save();
}

void SolarSettings::setStreamerMaskPrivacy(bool val) {
	if (_streamerMaskPrivacy.current() == val) return;
	_streamerMaskPrivacy = val;
	repaintApp();
	save();
}

void SolarSettings::setStreamerMaskNotifications(bool val) {
	if (_streamerMaskNotifications.current() == val) return;
	_streamerMaskNotifications = val;
	save();
}

void SolarSettings::setDeleteForAllByDefault(bool val) {
	if (_deleteForAllByDefault.current() == val) return;
	_deleteForAllByDefault = val;
	save();
}

void SolarSettings::setShowAlwaysOnTopButton(bool val) {
	if (_showAlwaysOnTopButton.current() == val) return;
	_showAlwaysOnTopButton = val;
	repaintApp();
	save();
}

void SolarSettings::setSeekbarForRoundVideos(bool val) {
	if (_seekbarForRoundVideos.current() == val) return;
	_seekbarForRoundVideos = val;
	save();
}

void SolarSettings::setTextReplacementsEnabled(bool val) {
	if (_textReplacementsEnabled.current() == val) return;
	_textReplacementsEnabled = val;
	save();
}

void SolarSettings::setLocalFoldersEnabled(bool val) {
	if (_localFoldersEnabled.current() == val) return;
	_localFoldersEnabled = val;
	repaintApp();
	save();
}

void SolarSettings::setPresenceLoggerEnabled(bool val) {
	if (_presenceLoggerEnabled.current() == val) return;
	_presenceLoggerEnabled = val;
	save();
}

void SolarSettings::setRememberLastSeen(bool val) {
	if (_rememberLastSeen.current() == val) return;
	_rememberLastSeen = val;
	save();
}

void SolarSettings::setStripImageExif(bool val) {
	if (_stripImageExif.current() == val) return;
	_stripImageExif = val;
	save();
}

void SolarSettings::setForcedCopy(bool val) {
	if (_forcedCopy.current() == val) return;
	_forcedCopy = val;
	save();
}

void SolarSettings::setDeviceSpoofEnabled(bool val) {
	if (_deviceSpoofEnabled.current() == val) return;
	_deviceSpoofEnabled = val;
	updateSpoofSnapshot(
		val,
		_deviceSpoofModel.current(),
		_deviceSpoofSystem.current(),
		_deviceSpoofAppVersion.current(),
		_deviceSpoofLangCode.current());
	save();
}

void SolarSettings::setDeviceSpoofModel(const QString &val) {
	if (_deviceSpoofModel.current() == val) return;
	_deviceSpoofModel = val;
	updateSpoofSnapshot(
		_deviceSpoofEnabled.current(),
		val,
		_deviceSpoofSystem.current(),
		_deviceSpoofAppVersion.current(),
		_deviceSpoofLangCode.current());
	save();
}

void SolarSettings::setDeviceSpoofSystem(const QString &val) {
	if (_deviceSpoofSystem.current() == val) return;
	_deviceSpoofSystem = val;
	updateSpoofSnapshot(
		_deviceSpoofEnabled.current(),
		_deviceSpoofModel.current(),
		val,
		_deviceSpoofAppVersion.current(),
		_deviceSpoofLangCode.current());
	save();
}

void SolarSettings::setDeviceSpoofAppVersion(const QString &val) {
	if (_deviceSpoofAppVersion.current() == val) return;
	_deviceSpoofAppVersion = val;
	updateSpoofSnapshot(
		_deviceSpoofEnabled.current(),
		_deviceSpoofModel.current(),
		_deviceSpoofSystem.current(),
		val,
		_deviceSpoofLangCode.current());
	save();
}

void SolarSettings::setDeviceSpoofLangCode(const QString &val) {
	if (_deviceSpoofLangCode.current() == val) return;
	_deviceSpoofLangCode = val;
	updateSpoofSnapshot(
		_deviceSpoofEnabled.current(),
		_deviceSpoofModel.current(),
		_deviceSpoofSystem.current(),
		_deviceSpoofAppVersion.current(),
		val);
	save();
}

void SolarSettings::setBlockExternalLookups(bool val) {
	if (_blockExternalLookups.current() == val) return;
	_blockExternalLookups = val;
	save();
}

void SolarSettings::setRandomDownloadNames(bool val) {
	if (_randomDownloadNames.current() == val) return;
	_randomDownloadNames = val;
	save();
}

void SolarSettings::setConfirmExternalLinks(bool val) {
	if (_confirmExternalLinks.current() == val) return;
	_confirmExternalLinks = val;
	save();
}

void SolarSettings::setPhishingGuard(bool val) {
	if (_phishingGuard.current() == val) return;
	_phishingGuard = val;
	save();
}

void SolarSettings::setPresenceOnlineAlerts(bool val) {
	if (_presenceOnlineAlerts.current() == val) return;
	_presenceOnlineAlerts = val;
	save();
}

void SolarSettings::setDownloadSpeedBoost(bool val) {
	if (_downloadSpeedBoost.current() == val) return;
	_downloadSpeedBoost = val;
	save();
}

void SolarSettings::setPanguSpacing(bool val) {
	if (_panguSpacing.current() == val) return;
	_panguSpacing = val;
	save();
}

void SolarSettings::setStripForeignAgentDisclaimers(bool val) {
	if (_stripForeignAgentDisclaimers.current() == val) return;
	_stripForeignAgentDisclaimers = val;
	save();
}

void SolarSettings::setHideBirthdays(bool val) {
	if (_hideBirthdays.current() == val) return;
	_hideBirthdays = val;
	save();
}

void SolarSettings::setHideGiftsUi(bool val) {
	if (_hideGiftsUi.current() == val) return;
	_hideGiftsUi = val;
	save();
}

void SolarSettings::setUnreadChatsFirst(bool val) {
	if (_unreadChatsFirst.current() == val) return;
	_unreadChatsFirst = val;
	save();
}

void SolarSettings::setChatStreaksEnabled(bool val) {
	if (_chatStreaksEnabled.current() == val) return;
	_chatStreaksEnabled = val;
	repaintApp();
	save();
}

void SolarSettings::setStoryAutoHideEnabled(bool val) {
	if (_storyAutoHideEnabled.current() == val) return;
	_storyAutoHideEnabled = val;
	_storyPrivacyVersion = _storyPrivacyVersion.current() + 1;
	save();
}

void SolarSettings::setChannelAutoBanEnabled(bool val) {
	if (_channelAutoBanEnabled.current() == val) return;
	_channelAutoBanEnabled = val;
	_channelBlockedVersion = _channelBlockedVersion.current() + 1;
	save();
}

SolarSettings::DeviceSpoofSnapshot SolarSettings::deviceSpoofSnapshot() const {
	std::lock_guard lock(spoofSnapshotMutex());
	return cachedSpoofSnapshot();
}

void to_json(nlohmann::json &j, const SolarSettings &s) {
	auto ghostAccounts = nlohmann::json::object();
	for (const auto &[key, value] : s._ghostAccounts) {
		ghostAccounts[std::to_string(key)] = *value;
	}

	auto readExcl = nlohmann::json::object();
	for (const auto &[key, value] : s._readExclusions) {
		readExcl[std::to_string(key)] = static_cast<int>(value);
	}

	auto typingExcl = nlohmann::json::object();
	for (const auto &[key, value] : s._typingExclusions) {
		typingExcl[std::to_string(key)] = static_cast<int>(value);
	}

	auto perChatGhost = nlohmann::json::object();
	for (const auto &[key, policy] : s._perChatGhost) {
		perChatGhost[std::to_string(key)] = nlohmann::json::array({
			static_cast<int>(policy.readMessages),
			static_cast<int>(policy.readStories),
			static_cast<int>(policy.onlinePackets),
			static_cast<int>(policy.uploadProgress),
			static_cast<int>(policy.offlineAfterOnline),
		});
	}

	j = nlohmann::json{
		{"ghostModeSettings", ghostAccounts},
		{"useGlobalGhostMode", s._useGlobalGhostMode.current()},
		{"saveDeletedMessages", s._saveDeletedMessages.current()},
		{"saveMessagesHistory", s._saveMessagesHistory.current()},
		{"saveForBots", s._saveForBots.current()},
		{"shadowBanIds", s._shadowBanIds},
		{"filtersEnabled", s._filtersEnabled.current()},
		{"filtersEnabledInChats", s._filtersEnabledInChats.current()},
		{"hideFromBlocked", s._hideFromBlocked.current()},
		{"semiTransparentDeletedMessages", s._semiTransparentDeletedMessages.current()},
		{"disableAds", s._disableAds.current()},
		{"disableStories", s._disableStories.current()},
		{"disableCustomBackgrounds", s._disableCustomBackgrounds.current()},
		{"hidePremiumStatuses", s._hidePremiumStatuses.current()},
		{"showOnlyAddedEmojisAndStickers", s._showOnlyAddedEmojisAndStickers.current()},
		{"collapseSimilarChannels", s._collapseSimilarChannels.current()},
		{"hideSimilarChannels", s._hideSimilarChannels.current()},
		{"messageBubbleRadius", s._messageBubbleRadius.current()},
		{"disableOpenLinkWarning", s._disableOpenLinkWarning.current()},
		{"wideMultiplier", s._wideMultiplier.current()},
		{"spoofWebviewAsAndroid", s._spoofWebviewAsAndroid.current()},
		{"increaseWebviewHeight", s._increaseWebviewHeight.current()},
		{"increaseWebviewWidth", s._increaseWebviewWidth.current()},
		{"materialSwitches", s._materialSwitches.current()},
		{"removeMessageTail", s._removeMessageTail.current()},
		{"disableNotificationsDelay", s._disableNotificationsDelay.current()},
		{"localPremium", s._localPremium.current()},
		{"showChannelReactions", s._showChannelReactions.current()},
		{"showGroupReactions", s._showGroupReactions.current()},
		{"showPrivateChatReactions", s._showPrivateChatReactions.current()},
		{"appIcon", s._appIcon.current()},
		{"simpleQuotesAndReplies", s._simpleQuotesAndReplies.current()},
		{"hideFastShare", s._hideFastShare.current()},
		{"replaceBottomInfoWithIcons", s._replaceBottomInfoWithIcons.current()},
		{"deletedMark", s._deletedMark.current()},
		{"editedMark", s._editedMark.current()},
		{"unlimitedRecentStickers", s._unlimitedRecentStickers.current()},
		{"showReactionsPanelInContextMenu", s._showReactionsPanelInContextMenu.current()},
		{"showViewsPanelInContextMenu", s._showViewsPanelInContextMenu.current()},
		{"showHideMessageInContextMenu", s._showHideMessageInContextMenu.current()},
		{"showUserMessagesInContextMenu", s._showUserMessagesInContextMenu.current()},
		{"showMessageDetailsInContextMenu", s._showMessageDetailsInContextMenu.current()},
		{"showRepeatMessageInContextMenu", s._showRepeatMessageInContextMenu.current()},
		{"showAddFilterInContextMenu", s._showAddFilterInContextMenu.current()},
		{"showAttachButtonInMessageField", s._showAttachButtonInMessageField.current()},
		{"showCommandsButtonInMessageField", s._showCommandsButtonInMessageField.current()},
		{"showEmojiButtonInMessageField", s._showEmojiButtonInMessageField.current()},
		{"showMicrophoneButtonInMessageField", s._showMicrophoneButtonInMessageField.current()},
		{"showAutoDeleteButtonInMessageField", s._showAutoDeleteButtonInMessageField.current()},
		{"showGiftButtonInMessageField", s._showGiftButtonInMessageField.current()},
		{"showAiEditorButtonInMessageField", s._showAiEditorButtonInMessageField.current()},
		{"showAttachPopup", s._showAttachPopup.current()},
		{"showEmojiPopup", s._showEmojiPopup.current()},
		{"showMyProfileInDrawer", s._showMyProfileInDrawer.current()},
		{"showBotsInDrawer", s._showBotsInDrawer.current()},
		{"showNewGroupInDrawer", s._showNewGroupInDrawer.current()},
		{"showNewChannelInDrawer", s._showNewChannelInDrawer.current()},
		{"showContactsInDrawer", s._showContactsInDrawer.current()},
		{"showCallsInDrawer", s._showCallsInDrawer.current()},
		{"showSavedMessagesInDrawer", s._showSavedMessagesInDrawer.current()},
		{"showLReadToggleInDrawer", s._showLReadToggleInDrawer.current()},
		{"showSReadToggleInDrawer", s._showSReadToggleInDrawer.current()},
		{"showNightModeToggleInDrawer", s._showNightModeToggleInDrawer.current()},
		{"showGhostToggleInDrawer", s._showGhostToggleInDrawer.current()},
		{"showStreamerToggleInDrawer", s._showStreamerToggleInDrawer.current()},
		{"showGhostToggleInTray", s._showGhostToggleInTray.current()},
		{"showStreamerToggleInTray", s._showStreamerToggleInTray.current()},
		{"monoFont", s._monoFont.current()},
		{"hideNotificationCounters", s._hideNotificationCounters.current()},
		{"hideNotificationBadge", s._hideNotificationBadge.current()},
		{"hideAllChatsFolder", s._hideAllChatsFolder.current()},
		{"channelBottomButton", s._channelBottomButton.current()},
		{"quickAdminShortcuts", s._quickAdminShortcuts.current()},
		{"disableGreetingSticker", s._disableGreetingSticker.current()},
		{"showPeerId", s._showPeerId.current()},
		{"showMessageSeconds", s._showMessageSeconds.current()},
		{"showMessageShot", s._showMessageShot.current()},
		{"filterZalgo", s._filterZalgo.current()},
		{"stickerConfirmation", s._stickerConfirmation.current()},
		{"gifConfirmation", s._gifConfirmation.current()},
		{"voiceConfirmation", s._voiceConfirmation.current()},
		{"roundConfirmation", s._roundConfirmation.current()},
		{"translationProvider", s._translationProvider.current()},
		{"adaptiveCoverColor", s._adaptiveCoverColor.current()},
		{"improveLinkPreviews", s._improveLinkPreviews.current()},
		{"crashReporting", s._crashReporting.current()},
		{"avatarCorners", s._avatarCorners.current()},
		{"singleCornerRadius", s._singleCornerRadius.current()},
		{"streamerMode", s._streamerMode.current()},
		{"autoStreamerMode", s._autoStreamerMode.current()},
		{"autoStripTrackingOnSend", s._autoStripTrackingOnSend.current()},
		{"antiCaps", s._antiCaps.current()},
		{"loginCodeLeakGuard", s._loginCodeLeakGuard.current()},
		{"eternalOnline", s._eternalOnline.current()},
		{"forceRelayCalls", s._forceRelayCalls.current()},
		{"callConfirmation", s._callConfirmation.current()},
		{"exactCounters", s._exactCounters.current()},
		{"alwaysOnTop", s._alwaysOnTop.current()},
		{"solarBrainEnabled", s._solarBrainEnabled.current()},
		{"solarBrainEndpoint", s._solarBrainEndpoint.current()},
		{"solarBrainModel", s._solarBrainModel.current()},
		{"deletedSaveMode", s._deletedSaveMode.current()},
		{"saveMediaPrivate", s._saveMediaPrivate.current()},
		{"saveMediaGroups", s._saveMediaGroups.current()},
		{"saveMediaChannels", s._saveMediaChannels.current()},
		{"saveMediaBots", s._saveMediaBots.current()},
		{"saveMediaStickers", s._saveMediaStickers.current()},
		{"attachmentsMaxSizeMb", s._attachmentsMaxSizeMb.current()},
		{"keepOneViewMedia", s._keepOneViewMedia.current()},
		{"spySaveReadMarks", s._spySaveReadMarks.current()},
		{"spySaveLocalOnline", s._spySaveLocalOnline.current()},
		{"displayGhostStatus", s._displayGhostStatus.current()},
		{"ghostModeShortcut", s._ghostModeShortcut.current()},
		{"uploadSpeedBoost", s._uploadSpeedBoost.current()},
		{"copyUsernameAsLink", s._copyUsernameAsLink.current()},
		{"walMode", s._walMode.current()},
		{"mainFont", s._mainFont.current()},
		{"duressPinHash", s._duressPinHash.current()},
		{"duressAction", s._duressAction.current()},
		{"hiddenChatsPinHash", s._hiddenChatsPinHash.current()},
		{"hiddenChatIds", s._hiddenChatIds},
		{"globalHotkeysEnabled", s._globalHotkeysEnabled.current()},
		{"streamerMaskPrivacy", s._streamerMaskPrivacy.current()},
		{"streamerMaskNotifications", s._streamerMaskNotifications.current()},
		{"deleteForAllByDefault", s._deleteForAllByDefault.current()},
		{"showAlwaysOnTopButton", s._showAlwaysOnTopButton.current()},
		{"seekbarForRoundVideos", s._seekbarForRoundVideos.current()},
		{"textReplacementsEnabled", s._textReplacementsEnabled.current()},
		{"localFoldersEnabled", s._localFoldersEnabled.current()},
		{"presenceLoggerEnabled", s._presenceLoggerEnabled.current()},
		{"rememberLastSeen", s._rememberLastSeen.current()},
		{"stripImageExif", s._stripImageExif.current()},
		{"forcedCopy", s._forcedCopy.current()},
		{"deviceSpoofEnabled", s._deviceSpoofEnabled.current()},
		{"deviceSpoofModel", s._deviceSpoofModel.current()},
		{"deviceSpoofSystem", s._deviceSpoofSystem.current()},
		{"deviceSpoofAppVersion", s._deviceSpoofAppVersion.current()},
		{"deviceSpoofLangCode", s._deviceSpoofLangCode.current()},
		{"blockExternalLookups", s._blockExternalLookups.current()},
		{"randomDownloadNames", s._randomDownloadNames.current()},
		{"confirmExternalLinks", s._confirmExternalLinks.current()},
		{"phishingGuard", s._phishingGuard.current()},
		{"presenceOnlineAlerts", s._presenceOnlineAlerts.current()},
		{"downloadSpeedBoost", s._downloadSpeedBoost.current()},
		{"panguSpacing", s._panguSpacing.current()},
		{"stripForeignAgentDisclaimers", s._stripForeignAgentDisclaimers.current()},
		{"hideBirthdays", s._hideBirthdays.current()},
		{"hideGiftsUi", s._hideGiftsUi.current()},
		{"unreadChatsFirst", s._unreadChatsFirst.current()},
		{"chatStreaksEnabled", s._chatStreaksEnabled.current()},
		{"storyAutoHideEnabled", s._storyAutoHideEnabled.current()},
		{"channelAutoBanEnabled", s._channelAutoBanEnabled.current()},
		{"savingExclusions", s._savingExclusions},
		{"watchedPeerIds", s._watchedPeerIds},
		{"storyHiddenUserIds", s._storyHiddenUserIds},
		{"storyFeedHiddenPeerIds", s._storyFeedHiddenPeerIds},
		{"channelBlockedUserIds", s._channelBlockedUserIds},
		{"recentChatIds", s._recentChatIds},
		{"readExclusions", readExcl},
		{"typingExclusions", typingExcl},
		{"perChatGhost", perChatGhost},
		{"messageShotSettings", s._messageShotSettings}
	};
}

void from_json(const nlohmann::json &j, SolarSettings &s) {
	SolarSettings defaults;

	if (j.contains("ghostModeSettings") && j["ghostModeSettings"].is_object()) {
		s._ghostAccounts.clear();
		for (auto &[key, value] : j["ghostModeSettings"].items()) {
			auto account = std::make_unique<GhostModeAccountSettings>();
			value.get_to(*account);
			s._ghostAccounts[std::stoull(key)] = std::move(account);
		}
	}

	s._useGlobalGhostMode = j.value("useGlobalGhostMode", defaults._useGlobalGhostMode.current());
	s._saveDeletedMessages = j.value("saveDeletedMessages", defaults._saveDeletedMessages.current());
	s._saveMessagesHistory = j.value("saveMessagesHistory", defaults._saveMessagesHistory.current());
	s._saveForBots = j.value("saveForBots", defaults._saveForBots.current());
	s._shadowBanIds = j.value("shadowBanIds", defaults._shadowBanIds);
	s._filtersEnabled = j.value("filtersEnabled", defaults._filtersEnabled.current());
	s._filtersEnabledInChats = j.value("filtersEnabledInChats", defaults._filtersEnabledInChats.current());
	s._hideFromBlocked = j.value("hideFromBlocked", defaults._hideFromBlocked.current());
	s._semiTransparentDeletedMessages = j.value("semiTransparentDeletedMessages", defaults._semiTransparentDeletedMessages.current());
	s._disableAds = j.value("disableAds", defaults._disableAds.current());
	s._disableStories = j.value("disableStories", defaults._disableStories.current());
	s._disableCustomBackgrounds = j.value("disableCustomBackgrounds", defaults._disableCustomBackgrounds.current());
	s._hidePremiumStatuses = j.value("hidePremiumStatuses", defaults._hidePremiumStatuses.current());
	s._showOnlyAddedEmojisAndStickers = j.value("showOnlyAddedEmojisAndStickers", defaults._showOnlyAddedEmojisAndStickers.current());
	s._collapseSimilarChannels = j.value("collapseSimilarChannels", defaults._collapseSimilarChannels.current());
	s._hideSimilarChannels = j.value("hideSimilarChannels", defaults._hideSimilarChannels.current());
	s._messageBubbleRadius = j.value("messageBubbleRadius", defaults._messageBubbleRadius.current());
	s._disableOpenLinkWarning = j.value("disableOpenLinkWarning", defaults._disableOpenLinkWarning.current());
	s._wideMultiplier = j.value("wideMultiplier", defaults._wideMultiplier.current());
	s._spoofWebviewAsAndroid = j.value("spoofWebviewAsAndroid", defaults._spoofWebviewAsAndroid.current());
	s._increaseWebviewHeight = j.value("increaseWebviewHeight", defaults._increaseWebviewHeight.current());
	s._increaseWebviewWidth = j.value("increaseWebviewWidth", defaults._increaseWebviewWidth.current());
	s._materialSwitches = j.value("materialSwitches", defaults._materialSwitches.current());
	s._removeMessageTail = j.value("removeMessageTail", defaults._removeMessageTail.current());
	s._disableNotificationsDelay = j.value("disableNotificationsDelay", defaults._disableNotificationsDelay.current());
	s._localPremium = j.value("localPremium", defaults._localPremium.current());
	s._showChannelReactions = j.value("showChannelReactions", defaults._showChannelReactions.current());
	s._showGroupReactions = j.value("showGroupReactions", defaults._showGroupReactions.current());
	s._showPrivateChatReactions = j.value("showPrivateChatReactions", defaults._showPrivateChatReactions.current());
	s._appIcon = j.value("appIcon", defaults._appIcon.current());
	s._simpleQuotesAndReplies = j.value("simpleQuotesAndReplies", defaults._simpleQuotesAndReplies.current());
	s._hideFastShare = j.value("hideFastShare", defaults._hideFastShare.current());
	s._replaceBottomInfoWithIcons = j.value("replaceBottomInfoWithIcons", defaults._replaceBottomInfoWithIcons.current());
	s._deletedMark = j.value("deletedMark", defaults._deletedMark.current());
	s._editedMark = j.value("editedMark", defaults._editedMark.current());
	s._unlimitedRecentStickers = j.value("unlimitedRecentStickers", defaults._unlimitedRecentStickers.current());
	s._showReactionsPanelInContextMenu = j.value("showReactionsPanelInContextMenu", defaults._showReactionsPanelInContextMenu.current());
	s._showViewsPanelInContextMenu = j.value("showViewsPanelInContextMenu", defaults._showViewsPanelInContextMenu.current());
	s._showHideMessageInContextMenu = j.value("showHideMessageInContextMenu", defaults._showHideMessageInContextMenu.current());
	s._showUserMessagesInContextMenu = j.value("showUserMessagesInContextMenu", defaults._showUserMessagesInContextMenu.current());
	s._showMessageDetailsInContextMenu = j.value("showMessageDetailsInContextMenu", defaults._showMessageDetailsInContextMenu.current());
	s._showRepeatMessageInContextMenu = j.value("showRepeatMessageInContextMenu", defaults._showRepeatMessageInContextMenu.current());
	s._showAddFilterInContextMenu = j.value("showAddFilterInContextMenu", defaults._showAddFilterInContextMenu.current());
	s._showAttachButtonInMessageField = j.value("showAttachButtonInMessageField", defaults._showAttachButtonInMessageField.current());
	s._showCommandsButtonInMessageField = j.value("showCommandsButtonInMessageField", defaults._showCommandsButtonInMessageField.current());
	s._showEmojiButtonInMessageField = j.value("showEmojiButtonInMessageField", defaults._showEmojiButtonInMessageField.current());
	s._showMicrophoneButtonInMessageField = j.value("showMicrophoneButtonInMessageField", defaults._showMicrophoneButtonInMessageField.current());
	s._showAutoDeleteButtonInMessageField = j.value("showAutoDeleteButtonInMessageField", defaults._showAutoDeleteButtonInMessageField.current());
	s._showGiftButtonInMessageField = j.value("showGiftButtonInMessageField", defaults._showGiftButtonInMessageField.current());
	s._showAiEditorButtonInMessageField = j.value("showAiEditorButtonInMessageField", defaults._showAiEditorButtonInMessageField.current());
	s._showAttachPopup = j.value("showAttachPopup", defaults._showAttachPopup.current());
	s._showEmojiPopup = j.value("showEmojiPopup", defaults._showEmojiPopup.current());
	s._showMyProfileInDrawer = j.value("showMyProfileInDrawer", defaults._showMyProfileInDrawer.current());
	s._showBotsInDrawer = j.value("showBotsInDrawer", defaults._showBotsInDrawer.current());
	s._showNewGroupInDrawer = j.value("showNewGroupInDrawer", defaults._showNewGroupInDrawer.current());
	s._showNewChannelInDrawer = j.value("showNewChannelInDrawer", defaults._showNewChannelInDrawer.current());
	s._showContactsInDrawer = j.value("showContactsInDrawer", defaults._showContactsInDrawer.current());
	s._showCallsInDrawer = j.value("showCallsInDrawer", defaults._showCallsInDrawer.current());
	s._showSavedMessagesInDrawer = j.value("showSavedMessagesInDrawer", defaults._showSavedMessagesInDrawer.current());
	s._showLReadToggleInDrawer = j.value("showLReadToggleInDrawer", defaults._showLReadToggleInDrawer.current());
	s._showSReadToggleInDrawer = j.value("showSReadToggleInDrawer", defaults._showSReadToggleInDrawer.current());
	s._showNightModeToggleInDrawer = j.value("showNightModeToggleInDrawer", defaults._showNightModeToggleInDrawer.current());
	s._showGhostToggleInDrawer = j.value("showGhostToggleInDrawer", defaults._showGhostToggleInDrawer.current());
	s._showStreamerToggleInDrawer = j.value("showStreamerToggleInDrawer", defaults._showStreamerToggleInDrawer.current());
	s._showGhostToggleInTray = j.value("showGhostToggleInTray", defaults._showGhostToggleInTray.current());
	s._showStreamerToggleInTray = j.value("showStreamerToggleInTray", defaults._showStreamerToggleInTray.current());
	s._monoFont = j.value("monoFont", defaults._monoFont.current());
	s._hideNotificationCounters = j.value("hideNotificationCounters", defaults._hideNotificationCounters.current());
	s._hideNotificationBadge = j.value("hideNotificationBadge", defaults._hideNotificationBadge.current());
	s._hideAllChatsFolder = j.value("hideAllChatsFolder", defaults._hideAllChatsFolder.current());
	s._channelBottomButton = j.value("channelBottomButton", defaults._channelBottomButton.current());
	s._quickAdminShortcuts = j.value("quickAdminShortcuts", defaults._quickAdminShortcuts.current());
	s._disableGreetingSticker = j.value("disableGreetingSticker", defaults._disableGreetingSticker.current());
	s._showPeerId = j.value("showPeerId", defaults._showPeerId.current());
	s._showMessageSeconds = j.value("showMessageSeconds", defaults._showMessageSeconds.current());
	s._showMessageShot = j.value("showMessageShot", defaults._showMessageShot.current());
	s._filterZalgo = j.value("filterZalgo", defaults._filterZalgo.current());
	s._stickerConfirmation = j.value("stickerConfirmation", defaults._stickerConfirmation.current());
	s._gifConfirmation = j.value("gifConfirmation", defaults._gifConfirmation.current());
	s._voiceConfirmation = j.value("voiceConfirmation", defaults._voiceConfirmation.current());
	s._roundConfirmation = j.value("roundConfirmation", defaults._roundConfirmation.current());
	s._translationProvider = j.value("translationProvider", defaults._translationProvider.current());
	s._adaptiveCoverColor = j.value("adaptiveCoverColor", defaults._adaptiveCoverColor.current());
	s._improveLinkPreviews = j.value("improveLinkPreviews", defaults._improveLinkPreviews.current());
	s._crashReporting = j.value("crashReporting", defaults._crashReporting.current());
	s._avatarCorners = j.value("avatarCorners", defaults._avatarCorners.current());
	s._singleCornerRadius = j.value("singleCornerRadius", defaults._singleCornerRadius.current());
	s._streamerMode = j.value("streamerMode", defaults._streamerMode.current());
	s._autoStreamerMode = j.value("autoStreamerMode", defaults._autoStreamerMode.current());
	s._autoStripTrackingOnSend = j.value("autoStripTrackingOnSend", defaults._autoStripTrackingOnSend.current());
	s._antiCaps = j.value("antiCaps", defaults._antiCaps.current());
	s._loginCodeLeakGuard = j.value("loginCodeLeakGuard", defaults._loginCodeLeakGuard.current());
	s._eternalOnline = j.value("eternalOnline", defaults._eternalOnline.current());
	s._forceRelayCalls = j.value("forceRelayCalls", defaults._forceRelayCalls.current());
	s._callConfirmation = j.value("callConfirmation", defaults._callConfirmation.current());
	s._exactCounters = j.value("exactCounters", defaults._exactCounters.current());
	s._alwaysOnTop = j.value("alwaysOnTop", defaults._alwaysOnTop.current());
	s._solarBrainEnabled = j.value("solarBrainEnabled", defaults._solarBrainEnabled.current());
	s._solarBrainEndpoint = j.value("solarBrainEndpoint", defaults._solarBrainEndpoint.current());
	s._solarBrainModel = j.value("solarBrainModel", defaults._solarBrainModel.current());

	if (j.contains("deletedSaveMode")) {
		s._deletedSaveMode = j.value("deletedSaveMode", defaults._deletedSaveMode.current());
		s._saveDeletedMessages = (s._deletedSaveMode.current() != DeletedSaveMode::Off);
	} else {
		s._deletedSaveMode = s._saveDeletedMessages.current()
			? DeletedSaveMode::Persistent
			: DeletedSaveMode::Off;
	}
	s._saveMediaPrivate = j.value("saveMediaPrivate", defaults._saveMediaPrivate.current());
	s._saveMediaGroups = j.value("saveMediaGroups", defaults._saveMediaGroups.current());
	s._saveMediaChannels = j.value("saveMediaChannels", defaults._saveMediaChannels.current());
	s._saveMediaBots = j.value("saveMediaBots", defaults._saveMediaBots.current());
	s._saveMediaStickers = j.value("saveMediaStickers", defaults._saveMediaStickers.current());
	s._attachmentsMaxSizeMb = j.value("attachmentsMaxSizeMb", defaults._attachmentsMaxSizeMb.current());
	s._keepOneViewMedia = j.value("keepOneViewMedia", defaults._keepOneViewMedia.current());
	s._spySaveReadMarks = j.value("spySaveReadMarks", defaults._spySaveReadMarks.current());
	s._spySaveLocalOnline = j.value("spySaveLocalOnline", defaults._spySaveLocalOnline.current());
	s._displayGhostStatus = j.value("displayGhostStatus", defaults._displayGhostStatus.current());
	s._ghostModeShortcut = j.value("ghostModeShortcut", defaults._ghostModeShortcut.current());
	s._uploadSpeedBoost = j.value("uploadSpeedBoost", defaults._uploadSpeedBoost.current());
	s._copyUsernameAsLink = j.value("copyUsernameAsLink", defaults._copyUsernameAsLink.current());
	s._walMode = j.value("walMode", defaults._walMode.current());
	s._mainFont = j.value("mainFont", defaults._mainFont.current());
	s._duressPinHash = j.value("duressPinHash", defaults._duressPinHash.current());
	s._duressAction = j.value("duressAction", defaults._duressAction.current());
	s._hiddenChatsPinHash = j.value("hiddenChatsPinHash", defaults._hiddenChatsPinHash.current());
	s._hiddenChatIds = j.value("hiddenChatIds", defaults._hiddenChatIds);
	s._globalHotkeysEnabled = j.value("globalHotkeysEnabled", defaults._globalHotkeysEnabled.current());
	s._streamerMaskPrivacy = j.value("streamerMaskPrivacy", defaults._streamerMaskPrivacy.current());
	s._streamerMaskNotifications = j.value("streamerMaskNotifications", defaults._streamerMaskNotifications.current());
	s._deleteForAllByDefault = j.value("deleteForAllByDefault", defaults._deleteForAllByDefault.current());
	s._showAlwaysOnTopButton = j.value("showAlwaysOnTopButton", defaults._showAlwaysOnTopButton.current());
	s._seekbarForRoundVideos = j.value("seekbarForRoundVideos", defaults._seekbarForRoundVideos.current());
	s._textReplacementsEnabled = j.value("textReplacementsEnabled", defaults._textReplacementsEnabled.current());
	s._localFoldersEnabled = j.value("localFoldersEnabled", defaults._localFoldersEnabled.current());
	s._presenceLoggerEnabled = j.value("presenceLoggerEnabled", defaults._presenceLoggerEnabled.current());
	s._rememberLastSeen = j.value("rememberLastSeen", defaults._rememberLastSeen.current());
	s._stripImageExif = j.value("stripImageExif", defaults._stripImageExif.current());
	s._forcedCopy = j.value("forcedCopy", defaults._forcedCopy.current());
	s._deviceSpoofEnabled = j.value("deviceSpoofEnabled", defaults._deviceSpoofEnabled.current());
	s._deviceSpoofModel = j.value("deviceSpoofModel", defaults._deviceSpoofModel.current());
	s._deviceSpoofSystem = j.value("deviceSpoofSystem", defaults._deviceSpoofSystem.current());
	s._deviceSpoofAppVersion = j.value("deviceSpoofAppVersion", defaults._deviceSpoofAppVersion.current());
	s._deviceSpoofLangCode = j.value("deviceSpoofLangCode", defaults._deviceSpoofLangCode.current());
	updateSpoofSnapshot(
		s._deviceSpoofEnabled.current(),
		s._deviceSpoofModel.current(),
		s._deviceSpoofSystem.current(),
		s._deviceSpoofAppVersion.current(),
		s._deviceSpoofLangCode.current());
	s._blockExternalLookups = j.value("blockExternalLookups", defaults._blockExternalLookups.current());
	s._randomDownloadNames = j.value("randomDownloadNames", defaults._randomDownloadNames.current());
	s._confirmExternalLinks = j.value("confirmExternalLinks", defaults._confirmExternalLinks.current());
	s._phishingGuard = j.value("phishingGuard", defaults._phishingGuard.current());
	s._presenceOnlineAlerts = j.value("presenceOnlineAlerts", defaults._presenceOnlineAlerts.current());
	s._downloadSpeedBoost = j.value("downloadSpeedBoost", defaults._downloadSpeedBoost.current());
	s._panguSpacing = j.value("panguSpacing", defaults._panguSpacing.current());
	s._stripForeignAgentDisclaimers = j.value("stripForeignAgentDisclaimers", defaults._stripForeignAgentDisclaimers.current());
	s._hideBirthdays = j.value("hideBirthdays", defaults._hideBirthdays.current());
	s._hideGiftsUi = j.value("hideGiftsUi", defaults._hideGiftsUi.current());
	s._unreadChatsFirst = j.value("unreadChatsFirst", defaults._unreadChatsFirst.current());
	s._chatStreaksEnabled = j.value("chatStreaksEnabled", defaults._chatStreaksEnabled.current());
	s._storyAutoHideEnabled = j.value("storyAutoHideEnabled", defaults._storyAutoHideEnabled.current());
	s._channelAutoBanEnabled = j.value("channelAutoBanEnabled", defaults._channelAutoBanEnabled.current());
	s._savingExclusions = j.value("savingExclusions", defaults._savingExclusions);
	s._watchedPeerIds = j.value("watchedPeerIds", defaults._watchedPeerIds);
	s._storyHiddenUserIds = j.value("storyHiddenUserIds", defaults._storyHiddenUserIds);
	s._storyFeedHiddenPeerIds = j.value("storyFeedHiddenPeerIds", defaults._storyFeedHiddenPeerIds);
	s._channelBlockedUserIds = j.value("channelBlockedUserIds", defaults._channelBlockedUserIds);
	s._recentChatIds = j.value("recentChatIds", defaults._recentChatIds);
	if (s._recentChatIds.size() > 50) {
		s._recentChatIds.resize(50);
	}

	s._readExclusions.clear();
	if (j.contains("readExclusions") && j["readExclusions"].is_object()) {
		for (const auto &[key, value] : j["readExclusions"].items()) {
			try {
				const auto id = static_cast<int64>(std::stoll(key));
				const auto modeInt = value.get<int>();
				if (modeInt >= 0 && modeInt <= 2) {
					s._readExclusions[id] = static_cast<ChatExclusionMode>(modeInt);
				}
			} catch (...) {
			}
		}
	}

	s._typingExclusions.clear();
	if (j.contains("typingExclusions") && j["typingExclusions"].is_object()) {
		for (const auto &[key, value] : j["typingExclusions"].items()) {
			try {
				const auto id = static_cast<int64>(std::stoll(key));
				const auto modeInt = value.get<int>();
				if (modeInt >= 0 && modeInt <= 2) {
					s._typingExclusions[id] = static_cast<ChatExclusionMode>(modeInt);
				}
			} catch (...) {
			}
		}
	}

	s._perChatGhost.clear();
	if (j.contains("perChatGhost") && j["perChatGhost"].is_object()) {
		const auto toMode = [](int v) {
			return (v >= 0 && v <= 2)
				? static_cast<ChatExclusionMode>(v)
				: ChatExclusionMode::GlobalDefault;
		};
		for (const auto &[key, value] : j["perChatGhost"].items()) {
			try {
				if (!value.is_array() || value.size() != 5) {
					continue;
				}
				const auto id = static_cast<int64>(std::stoll(key));
				s._perChatGhost[id] = PerChatGhostPolicy{
					.readMessages = toMode(value[0].get<int>()),
					.readStories = toMode(value[1].get<int>()),
					.onlinePackets = toMode(value[2].get<int>()),
					.uploadProgress = toMode(value[3].get<int>()),
					.offlineAfterOnline = toMode(value[4].get<int>()),
				};
			} catch (...) {
			}
		}
	}

	if (j.contains("messageShotSettings") && j["messageShotSettings"].is_object()) {
		j["messageShotSettings"].get_to(s._messageShotSettings);
	}
}
