// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "base/basic_types.h"
#include "mtproto/scheme/mtproto_api.h"

class ChannelData;
class HistoryItem;
class PeerData;
class UserData;

namespace Data {
class Story;
} // namespace Data

namespace Main {
class Session;
} // namespace Main

namespace Window {
class SessionController;
} // namespace Window

namespace SolarFeatures::Guards {

struct StreakInfo {
	int current = 0;
	int best = 0;
};

enum class BlacklistKind {
	StoryHide,
	StoryFeedHide,
	ChannelAutoBan,
};

void ObserveChatMessageForStreak(not_null<HistoryItem*> item);
[[nodiscard]] StreakInfo GetStreakInfo(not_null<PeerData*> peer);
[[nodiscard]] QString FormatStreakBadge(not_null<PeerData*> peer);

void ObserveSelfStory(not_null<Data::Story*> story);
int ApplyStoryHideToAllSelfStories(not_null<Main::Session*> session);

void BanUserInChannel(not_null<ChannelData*> channel, not_null<UserData*> user);
void BanUserInAllOwnedChannels(not_null<UserData*> user);
int SyncChannelBlacklists(not_null<Main::Session*> session);
void ObservePendingJoinRequests(
	not_null<ChannelData*> channel,
	const QVector<MTPlong> &requesters);
void ObserveChannelParticipant(
	not_null<ChannelData*> channel,
	UserId userId,
	const MTPChannelParticipant *newParticipant);
void ObserveChannelMessage(
	not_null<ChannelData*> channel,
	not_null<PeerData*> sender);

void ShowActivityRadarBox(
	not_null<Window::SessionController*> controller,
	not_null<PeerData*> peer);

void ShowBlacklistManagerBox(
	not_null<Window::SessionController*> controller,
	BlacklistKind kind);

} // namespace SolarFeatures::Guards
