// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/features/privacy/solar_guards.h"

#include "apiwrap.h"
#include "lang_auto.h"
#include "base/call_delayed.h"
#include "base/flat_map.h"
#include "base/flat_set.h"
#include "base/unixtime.h"
#include "boxes/peer_list_box.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_chat_participant_status.h"
#include "data/data_peer_id.h"
#include "data/data_session.h"
#include "data/data_stories.h"
#include "data/data_story.h"
#include "data/data_user.h"
#include "dialogs/dialogs_indexed_list.h"
#include "dialogs/dialogs_main_list.h"
#include "dialogs/dialogs_row.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/history_item_components.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "solar/data/entities.h"
#include "solar/data/solar_database.h"
#include "solar/solar_settings.h"
#include "solar/ui/boxes/edit_mark_box.h"
#include "solar/utils/telegram_helpers.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "ui/layers/generic_box.h"
#include "ui/toast/toast.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"

#include <QtCore/QDate>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>

namespace SolarFeatures::Guards {
namespace {

constexpr TimeId kBanCooldownSeconds = 30;

base::flat_map<std::pair<ID, StoryId>, int> &AppliedStoryVersions() {
	static base::flat_map<std::pair<ID, StoryId>, int> map;
	return map;
}

base::flat_map<std::pair<ID, ID>, TimeId> &RecentBanTimestamps() {
	static base::flat_map<std::pair<ID, ID>, TimeId> map;
	return map;
}

[[nodiscard]] ID CurrentUserBareId(not_null<Main::Session*> session) {
	return session->userId().bare & PeerId::kChatTypeMask;
}

[[nodiscard]] int JulianDayFromTimestamp(TimeId ts) {
	return static_cast<int>(
		QDateTime::fromSecsSinceEpoch(std::max<TimeId>(ts, 1)).date().toJulianDay());
}

[[nodiscard]] int TodayJulianDay() {
	return static_cast<int>(QDate::currentDate().toJulianDay());
}

[[nodiscard]] bool IsStreakEligiblePeer(not_null<PeerData*> peer) {
	if (peer->isSelf() || peer->isServiceUser()) {
		return false;
	}
	if (const auto user = peer->asUser()) {
		return !user->isBot();
	}
	return peer->isChat() || peer->isMegagroup();
}

bool SyncStoryPrivacyInternal(not_null<Data::Story*> story, bool force) {
	const auto &settings = SolarSettings::getInstance();
	if (!settings.storyAutoHideEnabled()
		|| settings.storyHiddenUserIds().empty()
		|| story->expired()
		|| !story->peer()->isSelf()) {
		return false;
	}

	const auto version = settings.storyPrivacyVersion();
	const auto key = std::make_pair(
		CurrentUserBareId(&story->session()),
		story->id());
	auto &applied = AppliedStoryVersions();
	if (!force) {
		const auto it = applied.find(key);
		if (it != applied.end() && it->second == version) {
			return false;
		}
	}

	QVector<MTPInputPrivacyRule> rules;
	switch (story->privacy()) {
	case Data::StoryPrivacy::Public:
		rules.push_back(MTP_inputPrivacyValueAllowAll());
		break;
	case Data::StoryPrivacy::Contacts:
		rules.push_back(MTP_inputPrivacyValueAllowContacts());
		break;
	case Data::StoryPrivacy::CloseFriends:
		rules.push_back(MTP_inputPrivacyValueAllowCloseFriends());
		break;
	case Data::StoryPrivacy::SelectedContacts:
	case Data::StoryPrivacy::Other:
		// Preserve custom selected-audience lists without widening visibility.
		return false;
	}

	QVector<MTPInputUser> disallowed;
	auto &data = story->owner();
	for (const auto rawId : settings.storyHiddenUserIds()) {
		if (rawId <= 0) {
			continue;
		}
		if (const auto user = data.userLoaded(UserId(static_cast<uint64>(rawId)))) {
			if (!user->isSelf()) {
				disallowed.push_back(user->inputUser());
			}
		}
	}
	if (disallowed.isEmpty()) {
		return false;
	}

	rules.push_back(
		MTP_inputPrivacyValueDisallowUsers(
			MTP_vector<MTPInputUser>(disallowed)));
	applied[key] = version;

	using Flag = MTPstories_EditStory::Flag;
	story->session().api().request(MTPstories_EditStory(
		MTP_flags(Flag::f_privacy_rules),
		MTP_inputPeerSelf(),
		MTP_int(story->id()),
		MTPInputMedia(),
		MTPVector<MTPMediaArea>(),
		MTPstring(),
		MTPVector<MTPMessageEntity>(),
		MTP_vector<MTPInputPrivacyRule>(std::move(rules)),
		MTPInputDocument()
	)).done([session = &story->session()](const MTPUpdates &result) {
		session->api().applyUpdates(result);
	}).fail([key](const MTP::Error &) {
		AppliedStoryVersions().remove(key);
	}).send();
	return true;
}

[[nodiscard]] PeerData *ResolvePeerFromEntry(
		Data::Session &data,
		BlacklistKind kind,
		int64 id) {
	if (kind == BlacklistKind::StoryFeedHide) {
		if (id > 0) {
			return data.userLoaded(UserId(static_cast<uint64>(id)));
		}
		const auto absId = static_cast<uint64>(-id);
		if (const auto ch = data.channelLoaded(ChannelId(absId))) {
			return ch;
		}
		return data.chatLoaded(ChatId(absId));
	}
	return (id > 0)
		? static_cast<PeerData*>(data.userLoaded(UserId(static_cast<uint64>(id))))
		: nullptr;
}

class BlacklistPeerController final : public PeerListController {
public:
	BlacklistPeerController(
		not_null<Window::SessionController*> controller,
		BlacklistKind kind)
	: _controller(base::make_weak(controller))
	, _session(&controller->session())
	, _kind(kind) {
	}

	void prepare() override {
		reloadRows();
	}

	void reloadRows() {
		while (delegate()->peerListFullRowsCount() > 0) {
			delegate()->peerListRemoveRow(delegate()->peerListRowAt(0));
		}
		auto &settings = SolarSettings::getInstance();
		const auto &ids = (_kind == BlacklistKind::StoryHide)
			? settings.storyHiddenUserIds()
			: (_kind == BlacklistKind::StoryFeedHide)
			? settings.storyFeedHiddenPeerIds()
			: settings.channelBlockedUserIds();

		auto &data = _session->data();
		for (const auto id : ids) {
			if (const auto peer = ResolvePeerFromEntry(data, _kind, id)) {
				auto row = std::make_unique<PeerListRow>(peer);
				row->setCustomStatus(u"Click to remove"_q);
				delegate()->peerListAppendRow(std::move(row));
			}
		}
		if (!delegate()->peerListFullRowsCount()) {
			setDescriptionText(tr::solar_BlacklistEmpty(tr::now));
		} else {
			setDescriptionText(QString());
		}
		delegate()->peerListRefreshRows();
	}

	void rowClicked(not_null<PeerListRow*> row) override {
		const auto peer = row->peer();
		auto &settings = SolarSettings::getInstance();
		const auto targetId = (_kind == BlacklistKind::StoryFeedHide)
			? getDialogIdFromPeer(peer)
			: getBareID(peer);
		switch (_kind) {
		case BlacklistKind::StoryHide:
			settings.setStoryHiddenForUser(targetId, false);
			break;
		case BlacklistKind::StoryFeedHide:
			settings.setStoryFeedHidden(targetId, false);
			_session->data().stories().notifySourcesChanged(
				Data::StorySourcesList::NotHidden);
			break;
		case BlacklistKind::ChannelAutoBan:
			settings.setChannelBlockedUser(targetId, false);
			break;
		}
		reloadRows();
	}

	Main::Session &session() const override {
		return *_session;
	}

private:
	const base::weak_ptr<Window::SessionController> _controller;
	const not_null<Main::Session*> _session;
	const BlacklistKind _kind;
};

} // namespace

void ObserveChatMessageForStreak(not_null<HistoryItem*> item) {
	if (!SolarSettings::getInstance().chatStreaksEnabled()) {
		return;
	}
	const auto peer = item->history()->peer;
	if (item->isService() || !IsStreakEligiblePeer(peer)) {
		return;
	}
	const auto date = item->date();
	if (date <= 0) {
		return;
	}
	const auto msgDay = JulianDayFromTimestamp(date);
	const auto today = TodayJulianDay();
	if (msgDay < today - 1 || msgDay > today + 1) {
		return;
	}

	const auto uid = CurrentUserBareId(&item->history()->session());
	const auto did = getDialogIdFromPeer(peer);
	auto streak = SolarDatabase::getChatStreak(uid, did).value_or(ChatStreak{
		.fakeId = 0,
		.userId = uid,
		.dialogId = did,
		.currentStreak = 0,
		.bestStreak = 0,
		.lastOutDay = 0,
		.lastInDay = 0,
		.lastStreakDay = 0,
	});

	auto changed = false;
	if (item->out()) {
		if (msgDay > streak.lastOutDay) {
			streak.lastOutDay = msgDay;
			changed = true;
		}
	} else {
		if (msgDay > streak.lastInDay) {
			streak.lastInDay = msgDay;
			changed = true;
		}
	}

	if (streak.lastOutDay == msgDay
		&& streak.lastInDay == msgDay
		&& streak.lastStreakDay < msgDay) {
		streak.currentStreak = (streak.lastStreakDay == msgDay - 1)
			? (streak.currentStreak + 1)
			: 1;
		if (streak.currentStreak > streak.bestStreak) {
			streak.bestStreak = streak.currentStreak;
		}
		streak.lastStreakDay = msgDay;
		changed = true;
	}

	if (changed) {
		SolarDatabase::upsertChatStreak(streak);
	}
}

StreakInfo GetStreakInfo(not_null<PeerData*> peer) {
	if (!SolarSettings::getInstance().chatStreaksEnabled()
		|| !IsStreakEligiblePeer(peer)) {
		return {};
	}
	const auto uid = CurrentUserBareId(&peer->session());
	const auto did = getDialogIdFromPeer(peer);
	const auto stored = SolarDatabase::getChatStreak(uid, did);
	if (!stored.has_value()) {
		return {};
	}
	const auto today = TodayJulianDay();
	const auto active = (stored->lastStreakDay >= today - 1)
		? stored->currentStreak
		: 0;
	return { .current = active, .best = stored->bestStreak };
}

QString FormatStreakBadge(not_null<PeerData*> peer) {
	const auto info = GetStreakInfo(peer);
	if (info.current < 2) {
		return QString();
	}
	return QString::fromUtf8("\xF0\x9F\x94\xA5") + QString::number(info.current);
}

void ObserveSelfStory(not_null<Data::Story*> story) {
	SyncStoryPrivacyInternal(story, false);
}

int ApplyStoryHideToAllSelfStories(not_null<Main::Session*> session) {
	auto &stories = session->data().stories();
	const auto selfId = session->userPeerId();
	const auto source = stories.source(selfId);
	if (!source) {
		return 0;
	}
	auto updated = 0;
	for (const auto &entry : source->ids) {
		if (const auto maybe = stories.lookup({ selfId, entry.id })) {
			if (SyncStoryPrivacyInternal(*maybe, true)) {
				++updated;
			}
		}
	}
	return updated;
}

void BanUserInChannel(not_null<ChannelData*> channel, not_null<UserData*> user) {
	if (user->isSelf() || !channel->canBanMembers()) {
		return;
	}
	const auto now = base::unixtime::now();
	const auto key = std::make_pair(getBareID(channel), getBareID(user));
	auto &recent = RecentBanTimestamps();
	if (const auto it = recent.find(key); it != recent.end()) {
		if (now - it->second < kBanCooldownSeconds) {
			return;
		}
	}
	recent[key] = now;

	const auto rights = ChannelData::KickedRestrictedRights(user);
	channel->session().api().request(MTPchannels_EditBanned(
		channel->inputChannel(),
		user->input(),
		RestrictionsToMTP(rights)
	)).done([session = &channel->session()](const MTPUpdates &result) {
		session->api().applyUpdates(result);
	}).send();
}

void BanUserInAllOwnedChannels(not_null<UserData*> user) {
	if (user->isSelf()) {
		return;
	}
	auto delay = 0;
	const auto weakUser = base::make_weak(user);
	for (const auto &row : user->owner().chatsList()->indexed()->all()) {
		const auto history = row->history();
		const auto channel = history ? history->peer->asChannel() : nullptr;
		if (!channel || !channel->canBanMembers()) {
			continue;
		}
		const auto weakChannel = base::make_weak(channel);
		base::call_delayed(delay, &user->session(), [=] {
			if (const auto ch = weakChannel.get()) {
				if (const auto u = weakUser.get()) {
					BanUserInChannel(ch, u);
				}
			}
		});
		delay += 120;
	}
}

int SyncChannelBlacklists(not_null<Main::Session*> session) {
	const auto &blocked = SolarSettings::getInstance().channelBlockedUserIds();
	if (blocked.empty()) {
		return 0;
	}
	auto channelsCount = 0;
	auto delay = 0;
	auto &data = session->data();
	for (const auto &row : data.chatsList()->indexed()->all()) {
		const auto history = row->history();
		const auto channel = history ? history->peer->asChannel() : nullptr;
		if (!channel || !channel->canBanMembers()) {
			continue;
		}
		++channelsCount;
		for (const auto rawUid : blocked) {
			if (rawUid <= 0) {
				continue;
			}
			const auto user = data.userLoaded(UserId(static_cast<uint64>(rawUid)));
			if (!user || user->isSelf()) {
				continue;
			}
			const auto weakChannel = base::make_weak(channel);
			const auto weakUser = base::make_weak(user);
			base::call_delayed(delay, session, [=] {
				if (const auto ch = weakChannel.get()) {
					if (const auto u = weakUser.get()) {
						BanUserInChannel(ch, u);
					}
				}
			});
			delay += 120;
		}
	}
	return channelsCount;
}

void ObservePendingJoinRequests(
		not_null<ChannelData*> channel,
		const QVector<MTPlong> &requesters) {
	const auto &settings = SolarSettings::getInstance();
	if (!settings.channelAutoBanEnabled()
		|| !channel->canBanMembers()
		|| settings.channelBlockedUserIds().empty()) {
		return;
	}
	for (const auto &rawId : requesters) {
		const auto uid = static_cast<int64>(rawId.v);
		if (!settings.isChannelBlockedUser(uid)) {
			continue;
		}
		if (const auto user = channel->owner().userLoaded(UserId(rawId))) {
			channel->session().api().request(MTPmessages_HideChatJoinRequest(
				MTP_flags(0),
				channel->input(),
				user->inputUser()
			)).send();
			BanUserInChannel(channel, user);
		}
	}
}

void ObserveChannelParticipant(
		not_null<ChannelData*> channel,
		UserId userId,
		const MTPChannelParticipant *newParticipant) {
	const auto &settings = SolarSettings::getInstance();
	if (!settings.channelAutoBanEnabled()
		|| !channel->canBanMembers()
		|| !newParticipant) {
		return;
	}
	if (newParticipant->type() != mtpc_channelParticipant) {
		return;
	}
	if (!settings.isChannelBlockedUser(static_cast<int64>(userId.bare))) {
		return;
	}
	if (const auto user = channel->owner().userLoaded(userId)) {
		BanUserInChannel(channel, user);
	}
}

void ObserveChannelMessage(
		not_null<ChannelData*> channel,
		not_null<PeerData*> sender) {
	const auto &settings = SolarSettings::getInstance();
	if (!settings.channelAutoBanEnabled() || !channel->canBanMembers()) {
		return;
	}
	if (const auto user = sender->asUser()) {
		if (settings.isChannelBlockedUser(getBareID(user))) {
			BanUserInChannel(channel, user);
		}
	}
}

void ShowActivityRadarBox(
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::solar_ActivityRadarTitle());
		box->addButton(tr::lng_close(), [=] { box->closeBox(); });

		QStringList sections;
		sections.push_back(u"Target: %1 (ID: %2)"_q.arg(
			peer->name(),
			QString::number(getDialogIdFromPeer(peer))));

		const auto streak = GetStreakInfo(peer);
		if (streak.best > 0 || streak.current > 0) {
			sections.push_back(u"Chat Streak: %1 %2 day(s) · Best: %3 day(s)"_q.arg(
				QString::fromUtf8("\xF0\x9F\x94\xA5"),
				QString::number(streak.current),
				QString::number(streak.best)));
		}

		const auto label = box->addRow(
			object_ptr<Ui::FlatLabel>(box, sections.join(u"\n\n"_q), st::boxLabel),
			st::boxRowPadding);
		label->setSelectable(true);

		if (const auto user = peer->asUser(); user && user->isSelf()) {
			auto &stories = peer->owner().stories();
			const auto source = stories.source(peer->id);
			if (!source || source->ids.empty()) {
				sections.push_back(u"No active stories right now to list viewers from."_q);
				label->setText(sections.join(u"\n\n"_q));
				return;
			}
			const auto storyId = source->ids.back().id;
			const auto baseSections = sections;
			sections.push_back(u"Loading latest active story (#%1) viewers from Telegram..."_q.arg(storyId));
			label->setText(sections.join(u"\n\n"_q));

			const auto weakBox = base::make_weak(box);
			stories.loadViewsSlice(peer, storyId, QString(), [=](Data::StoryViews slice) {
				if (!weakBox) {
					return;
				}
				QStringList out = baseSections;
				out.push_back(u"Latest Story (#%1): %2 views · %3 reactions · %4 forwards"_q.arg(
					QString::number(storyId),
					QString::number(slice.views),
					QString::number(slice.reactions),
					QString::number(slice.forwards)));
				for (const auto &v : slice.list) {
					const auto when = v.date
						? langDateTime(QDateTime::fromSecsSinceEpoch(v.date))
						: u"repost/forward"_q;
					out.push_back(u"• %1  (%2)"_q.arg(v.peer->name(), when));
				}
				label->setText(out.join(u"\n"_q));
			});
			return;
		}

		if (const auto user = peer->asUser()) {
			const auto uid = CurrentUserBareId(&peer->session());
			const auto did = getDialogIdFromPeer(peer);
			const auto logs = SolarDatabase::getSpyPresenceLogs(uid, did, 25);
			const auto remembered = SolarDatabase::getRememberedLastSeen(uid, did);
			if (remembered > 0) {
				sections.push_back(u"Remembered Last Seen: %1"_q.arg(
					langDateTime(QDateTime::fromSecsSinceEpoch(remembered))));
			}
			sections.push_back(u"Recorded Presence Events: %1"_q.arg(logs.size()));
			for (const auto &e : logs) {
				const auto when = langDateTime(QDateTime::fromSecsSinceEpoch(e.timestamp));
				const auto kind = (e.eventType == 0) ? u"Online"_q
					: (e.eventType == 1) ? u"Offline"_q
					: (e.eventType == 2) ? u"Profile"_q
					: (e.eventType == 3) ? u"Phone"_q
					: (e.eventType == 4) ? u"Bio"_q
					: u"Activity"_q;
				sections.push_back(u"• %1 [%2] %3"_q.arg(
					when, kind, QString::fromStdString(e.details)));
			}
			sections.push_back(
				u"Note: Telegram MTProto servers do not record or transmit passive profile visits."_q);
			label->setText(sections.join(u"\n"_q));
			return;
		}

		if (const auto history = peer->owner().historyLoaded(peer)) {
			base::flat_set<PeerData*> activePeers;
			for (auto i = history->blocks.rbegin();
					i != history->blocks.rend() && activePeers.size() < 25;
					++i) {
				for (auto j = (*i)->messages.rbegin();
						j != (*i)->messages.rend() && activePeers.size() < 25;
						++j) {
					const auto item = (*j)->data();
					if (!item->isService() && item->from()) {
						activePeers.emplace(item->from());
					}
					for (const auto &[rid, list] : item->recentReactions()) {
						for (const auto &r : list) {
							activePeers.emplace(r.peer);
						}
					}
				}
			}
			if (!activePeers.empty()) {
				QStringList names;
				for (const auto p : activePeers) {
					names.push_back(p->name());
				}
				sections.push_back(u"Recent active senders / reactors in loaded posts:\n• %1"_q.arg(
					names.join(u", "_q)));
			}
		}

		if (const auto channel = peer->asChannel();
				channel && (channel->amCreator() || channel->hasAdminRights())) {
			const auto baseSections = sections;
			sections.push_back(u"Fetching recent Join/Leave/Invite admin events..."_q);
			label->setText(sections.join(u"\n\n"_q));

			using FilterFlag = MTPDchannelAdminLogEventsFilter::Flag;
			const auto filterFlags = FilterFlag::f_join
				| FilterFlag::f_leave
				| FilterFlag::f_invite
				| FilterFlag::f_ban
				| FilterFlag::f_kick;
			const auto weakBox = base::make_weak(box);
			channel->session().api().request(MTPchannels_GetAdminLog(
				MTP_flags(MTPchannels_GetAdminLog::Flag::f_events_filter),
				channel->inputChannel(),
				MTPstring(),
				MTP_channelAdminLogEventsFilter(MTP_flags(filterFlags)),
				MTPVector<MTPInputUser>(),
				MTP_long(0),
				MTP_long(0),
				MTP_int(30)
			)).done([=](const MTPchannels_AdminLogResults &result) {
				if (!weakBox) {
					return;
				}
				const auto &res = result.c_channels_adminLogResults();
				channel->owner().processUsers(res.vusers());
				channel->owner().processChats(res.vchats());

				QStringList out = baseSections;
				out.push_back(u"Recent Admin Log Events (%1):"_q.arg(res.vevents().v.size()));
				for (const auto &ev : res.vevents().v) {
					const auto &d = ev.data();
					const auto actor = channel->owner().user(UserId(d.vuser_id()));
					const auto when = langDateTime(QDateTime::fromSecsSinceEpoch(d.vdate().v));
					const auto type = d.vaction().type();
					const auto actionName =
						(type == mtpc_channelAdminLogEventActionParticipantJoin) ? u"Joined"_q
						: (type == mtpc_channelAdminLogEventActionParticipantLeave) ? u"Left"_q
						: (type == mtpc_channelAdminLogEventActionParticipantJoinByInvite) ? u"Joined via invite"_q
						: (type == mtpc_channelAdminLogEventActionParticipantJoinByRequest) ? u"Joined by request"_q
						: (type == mtpc_channelAdminLogEventActionParticipantInvite) ? u"Invited"_q
						: (type == mtpc_channelAdminLogEventActionParticipantToggleBan) ? u"Ban toggled"_q
						: u"Participant event"_q;
					out.push_back(u"• %1 — %2: %3 (ID: %4)"_q.arg(
						when,
						actionName,
						actor->name(),
						QString::number(actor->id.value & PeerId::kChatTypeMask)));
				}
				out.push_back(
					u"Note: Passive channel views without join/reaction/comment are not exposed by Telegram."_q);
				label->setText(out.join(u"\n"_q));
			}).fail([=](const MTP::Error &err) {
				if (!weakBox) {
					return;
				}
				QStringList out = baseSections;
				out.push_back(u"Could not load admin log: %1"_q.arg(err.type()));
				label->setText(out.join(u"\n\n"_q));
			}).send();
		} else {
			sections.push_back(
				u"Note: Admin rights are required to view channel Join/Leave events. Passive opens are never sent by Telegram."_q);
			label->setText(sections.join(u"\n\n"_q));
		}
	}));
}

void ShowBlacklistManagerBox(
		not_null<Window::SessionController*> controller,
		BlacklistKind kind) {
	const auto title = (kind == BlacklistKind::StoryHide)
		? tr::solar_StoryBlacklistManage()
		: (kind == BlacklistKind::StoryFeedHide)
		? tr::solar_StoryFeedHiddenManage()
		: tr::solar_ChannelBlacklistManage();

	controller->show(Box<PeerListBox>(
		std::make_unique<BlacklistPeerController>(controller, kind),
		[=](not_null<PeerListBox*> box) {
			box->setTitle(title);
			if (kind == BlacklistKind::StoryHide || kind == BlacklistKind::ChannelAutoBan) {
				const auto weakCtrl = base::make_weak(controller);
				const auto weakBox = base::make_weak(box);
				box->addButton(tr::solar_AddUserById(), [=] {
					const auto c = weakCtrl.get();
					if (!c) {
						return;
					}
					c->show(Box<EditMarkBox>(
						tr::solar_AddUserById(),
						QString(),
						QString(),
						[=](const QString &value) {
							auto ok = false;
							const auto parsed = value.trimmed().toLongLong(&ok);
							if (!ok || parsed <= 0) {
								if (const auto c2 = weakCtrl.get()) {
									c2->showToast(tr::solar_InvalidUserId(tr::now));
								}
								return;
							}
							auto &s = SolarSettings::getInstance();
							if (kind == BlacklistKind::StoryHide) {
								s.setStoryHiddenForUser(parsed, true);
							} else {
								s.setChannelBlockedUser(parsed, true);
							}
							if (const auto b = weakBox.get()) {
								b->closeBox();
							}
							if (const auto c2 = weakCtrl.get()) {
								ShowBlacklistManagerBox(c2, kind);
							}
						}));
				});
			}
			box->addButton(tr::lng_close(), [=] { box->closeBox(); });
			box->addLeftButton(tr::solar_ClearBlacklist(), [=] {
				auto &s = SolarSettings::getInstance();
				switch (kind) {
				case BlacklistKind::StoryHide:
					s.clearStoryHiddenUsers();
					break;
				case BlacklistKind::StoryFeedHide:
					s.clearStoryFeedHidden();
					controller->session().data().stories().notifySourcesChanged(
						Data::StorySourcesList::NotHidden);
					break;
				case BlacklistKind::ChannelAutoBan:
					s.clearChannelBlockedUsers();
					break;
				}
				box->closeBox();
			});
		}));
}

} // namespace SolarFeatures::Guards
