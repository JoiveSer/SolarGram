// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/ui/context_menu/context_menu.h"

#include "apiwrap.h"
#include "lang_auto.h"
#include "lang/lang_keys.h"
#include "mainwidget.h"
#include "api/api_sending.h"
#include "solar/solar_settings.h"
#include "solar/solar_state.h"
#include "solar/data/messages_storage.h"
#include "solar/data/solar_database.h"
#include "solar/features/ai/solar_brain.h"
#include "solar/features/filters/filters_controller.h"
#include "solar/features/forward/solar_forward.h"
#include "solar/features/forward/solar_forward_rich.h"
#include "solar/ui/context_menu/menu_item_subtext.h"
#include "solar/ui/message_history/history_section.h"
#include "solar/ui/settings/settings_solar_utils.h"
#include "solar/ui/settings/filters/edit_filter.h"
#include "solar/ui/settings/filters/settings_filters_list.h"
#include "solar/utils/qt_key_modifiers_extended.h"
#include "solar/utils/telegram_helpers.h"
#include "base/call_delayed.h"
#include "base/random.h"
#include "base/unixtime.h"
#include "core/mime_type.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_forum_topic.h"
#include "data/data_media_types.h"
#include "data/data_search_controller.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/history_item_components.h"
#include "history/view/history_view_context_menu.h"
#include "history/view/history_view_element.h"
#include "main/main_session.h"
#include "main/session/send_as_peers.h"
#include "styles/style_solar_icons.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "ui/boxes/confirm_box.h"
#include "ui/widgets/popup_menu.h"
#include "ui/widgets/menu/menu_add_action_callback_factory.h"
#include "window/window_peer_menu.h"
#include "window/window_session_controller.h"
#include "data/data_peer_values.h"
#include "styles/style_boxes.h"
#include "styles/style_widgets.h"
#include "ui/layers/generic_box.h"
#include "ui/toast/toast.h"
#include "ui/widgets/labels.h"

#include <QtCore/QDateTime>
#include <QtCore/QStringList>

namespace SolarUi {

namespace {

Fn<void()> ClearDeletedMessagesHandler(not_null<Window::SessionController*> controller, not_null<PeerData*> peer, ID topicId) {
	return [=] {
		controller->show(Ui::MakeConfirmBox({
			.text = tr::solar_ClearDeletedMessagesText(tr::now),
			.confirmed = [=](Fn<void()> &&close) {
				auto items = std::vector<not_null<HistoryItem*>>();
				for (const auto &block : peer->owner().history(peer)->blocks) {
					for (const auto &view : block->messages) {
						const auto item = view->data();
						if (item->isDeleted() && (!topicId || (item->topicRootId().bare == topicId))) {
							items.push_back(item);
						}
					}
				}
				SolarMessages::clearDeletedMessages(peer, topicId);
				for (const auto item : items) {
					item->destroy();
				}
				close();
			},
			.confirmText = tr::solar_ClearDeletedMessagesActionText(tr::now),
			.cancelText = tr::lng_cancel(),
			.confirmStyle = &st::attentionBoxButton,
		}));
	};
}

void DeleteMyMessagesAfterConfirm(
		not_null<PeerData*> peer,
		int maxCount = 0,
		TimeId minDate = 0) {
	const auto session = &peer->session();

	if (!maxCount && !minDate) {
		if (const auto channel = peer->asChannel()) {
			if (channel->isMegagroup() && channel->canDeleteMessages()) {
				session->api().deleteAllFromParticipant(channel, session->user());
				return;
			}
		}
	}

	auto collected = std::make_shared<std::vector<MsgId>>();

	const auto removeNext = std::make_shared<Fn<void(int)>>();
	const auto requestNext = std::make_shared<Fn<void(MsgId)>>();

	*removeNext = [=](int index)
	{
		if (index >= int(collected->size())) {
			DEBUG_LOG(("Deleted all %1 my messages in this chat").arg(collected->size()));
			return;
		}

		QVector<MTPint> ids;
		ids.reserve(std::min<int>(100, collected->size() - index));
		for (auto i = 0; i < 100 && (index + i) < int(collected->size()); ++i) {
			ids.push_back(MTP_int((*collected)[index + i].bare));
		}

		const auto batch = index / 100 + 1;
		const auto done = [=](const MTPmessages_AffectedMessages &result)
		{
			session->api().applyAffectedMessages(peer, result);
			if (peer->isChannel()) {
				session->data().processMessagesDeleted(peer->id, ids);
			} else {
				session->data().processNonChannelMessagesDeleted(ids);
			}
			const auto deleted = index + ids.size();
			DEBUG_LOG(("Deleted batch %1, total deleted %2/%3").arg(batch).arg(deleted).arg(collected->size()));
			const auto delay = crl::time(500 + base::RandomValue<int>() % 500);
			base::call_delayed(delay, [=] { (*removeNext)(deleted); });
		};
		const auto fail = [=](const MTP::Error &error)
		{
			const auto type = error.type();
			DEBUG_LOG(("Delete batch %1 failed: %2").arg(batch).arg(type));

			if (type.startsWith(u"FLOOD_WAIT_"_q)
				|| type.startsWith(u"FLOOD_PREMIUM_WAIT_"_q)) {
				const auto underscore = type.lastIndexOf('_');
				const auto seconds = (underscore >= 0)
					? type.mid(underscore + 1).toInt()
					: 0;
				const auto delay = crl::time(std::max(seconds, 1) * 1000);
				base::call_delayed(delay, [=] { (*removeNext)(index); });
				return;
			}

			if (type == u"MESSAGE_DELETE_FORBIDDEN"_q
				|| type == u"MSG_ID_INVALID"_q
				|| type == u"MESSAGE_ID_INVALID"_q) {
				DEBUG_LOG(("Skipping batch %1 (%2 ids)").arg(batch).arg(ids.size()));
				const auto delay = crl::time(500 + base::RandomValue<int>() % 500);
				base::call_delayed(delay, [=] { (*removeNext)(index + ids.size()); });
				return;
			}

			DEBUG_LOG(("Stopping deletion, unrecoverable error: %1").arg(type));
		};

		if (const auto channel = peer->asChannel()) {
			session->api()
				.request(MTPchannels_DeleteMessages(channel->inputChannel(), MTP_vector<MTPint>(ids)))
				.done(done)
				.fail(fail)
				.handleFloodErrors()
				.send();
		} else {
			using Flag = MTPmessages_DeleteMessages::Flag;
			session->api()
				.request(MTPmessages_DeleteMessages(MTP_flags(Flag::f_revoke), MTP_vector<MTPint>(ids)))
				.done(done)
				.fail(fail)
				.handleFloodErrors()
				.send();
		}
	};

	*requestNext = [=](MsgId from)
	{
		using Flag = MTPmessages_Search::Flag;
		auto request = MTPmessages_Search(
			MTP_flags(Flag::f_from_id),
			peer->input(),
			MTP_string(),
			MTP_inputPeerSelf(),
			MTPInputPeer(),
			MTPVector<MTPReaction>(),
			MTP_int(0),
			// top_msg_id
			MTP_inputMessagesFilterEmpty(),
			MTP_int(minDate),
			// min_date
			MTP_int(0),
			// max_date
			MTP_int(from.bare),
			MTP_int(0),
			// add_offset
			MTP_int(100),
			MTP_int(0),
			// max_id
			MTP_int(0),
			// min_id
			MTP_long(0)); // hash

		session->api()
			.request(std::move(request))
			.done([=](const Api::HistoryRequestResult &result)
			{
				auto parsed = Api::ParseHistoryResult(peer, from, Data::LoadDirection::Before, result);
				MsgId minId;
				int batchCount = 0;
				for (const auto &id : parsed.messageIds) {
					if (!minId || id < minId) minId = id;
					collected->push_back(id);
					++batchCount;
				}
				DEBUG_LOG(("Batch found %1 my messages, total %2").arg(batchCount).arg(collected->size()));
				if (maxCount > 0 && int(collected->size()) >= maxCount) {
					collected->resize(maxCount);
					(*removeNext)(0);
				} else if (parsed.messageIds.size() == 100 && minId) {
					(*requestNext)(minId - MsgId(1));
				} else {
					DEBUG_LOG(("Found %1 my messages in this chat (SEARCH)").arg(collected->size()));
					(*removeNext)(0);
				}
			})
			.fail([=](const MTP::Error &error) { DEBUG_LOG(("History fetch failed: %1").arg(error.type())); })
			.send();
	};

	(*requestNext)(MsgId(0));
}

Fn<void()> DeleteMyMessagesHandler(
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer,
		int maxCount = 0,
		TimeId minDate = 0) {
	return [=]
	{
		if (!controller->showFrozenError()) {
			controller->show(Ui::MakeConfirmBox({
				.text = tr::solar_DeleteOwnMessagesConfirmation(tr::now),
				.confirmed =
				[=](Fn<void()> &&close)
				{
					DeleteMyMessagesAfterConfirm(peer, maxCount, minDate);
					close();
				},
				.confirmText = tr::lng_box_delete(),
				.cancelText = tr::lng_cancel(),
				.confirmStyle = &st::attentionBoxButton,
			}));
		}
	};
}

}

bool needToShowItem(ContextMenuVisibility state) {
	return state == ContextMenuVisibility::Visible
		|| (state == ContextMenuVisibility::VisibleWithModifier && base::IsExtendedContextMenuModifierPressed());
}

void ShowPresenceHistoryBox(
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer) {
	const auto userId = controller->session().userId().bare & PeerId::kChatTypeMask;
	const auto dialogId = getDialogIdFromPeer(peer);
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::solar_PresenceHistoryTitle());
		const auto logs = SolarDatabase::getSpyPresenceLogs(userId, dialogId, 200);
		if (logs.empty()) {
			box->addRow(object_ptr<Ui::FlatLabel>(
				box,
				tr::solar_PresenceHistoryEmpty(),
				st::boxLabel));
		} else {
			const auto labelForType = [](int eventType) -> QString {
				switch (eventType) {
				case 0: return u"Online"_q;
				case 1: return u"Offline"_q;
				case 2: return u"Profile"_q;
				case 3: return u"Phone"_q;
				case 4: return u"Bio"_q;
				case 5: return u"Last seen before hidden"_q;
				default: return u"Event"_q;
				}
			};
			QStringList lines;
			lines.reserve(int(logs.size()));
			for (const auto &entry : logs) {
				const auto timeText = langDateTime(
					QDateTime::fromSecsSinceEpoch(entry.timestamp));
				const auto details = QString::fromStdString(entry.details);
				lines.push_back(details.isEmpty()
					? u"%1  [%2]"_q.arg(timeText, labelForType(entry.eventType))
					: u"%1  [%2]  %3"_q.arg(
						timeText,
						labelForType(entry.eventType),
						details));
			}
			const auto label = box->addRow(
				object_ptr<Ui::FlatLabel>(box, lines.join(u"\n"_q), st::boxLabel),
				st::boxRowPadding);
			label->setSelectable(true);
			box->addLeftButton(tr::solar_PresenceClearAction(), [=] {
				SolarDatabase::clearSpyPresenceLogs(userId, dialogId);
				box->closeBox();
			});
		}
		box->addButton(tr::lng_close(), [=] { box->closeBox(); });
	}));
}

void AddSolarGramActions(PeerData *peerData,
							   Data::Thread *thread,
							   not_null<Window::SessionController*> sessionController,
							   const Window::PeerMenuCallback &addCallback) {
	if (!peerData) {
		return;
	}

	const auto &settings = SolarSettings::getInstance();
	const auto user = peerData->asUser();
	const auto showFilters = settings.filtersEnabled()
		&& (!user || user->isBot());
	const auto saveDeletedMessages = settings.saveDeletedMessages();

	const auto topic = peerData->isForum() && thread ? thread->asTopic() : nullptr;
	const auto topicId = topic ? topic->rootId().bare : 0;
	const auto dialogId = getDialogIdFromPeer(peerData);

	addCallback(Window::PeerMenuCallback::Args{
		.text = u"SolarGram"_q,
		.handler = nullptr,
		.icon = &st::menuIconGroupReactions,
		.fillSubmenu = [=](not_null<Ui::PopupMenu*> menu) {
			const auto addAction = Ui::Menu::CreateAddActionCallback(menu);
			auto addedPrimary = false;
			if (showFilters) {
				addedPrimary = true;
				addAction(
					tr::solar_ViewFiltersMenuText(tr::now),
					[=]
					{
						sessionController->dialogId = dialogId;
						sessionController->showExclude = true;
						sessionController->shadowBan = false;
						sessionController->showSettings(Settings::SolarFiltersList::Id());
					},
					&st::menuIconAddToFolder);
			}
			const auto filteredToggleShown = FiltersController::filteredMessagesShown(peerData);
			if (filteredToggleShown) {
				addedPrimary = true;
				addAction(
					*filteredToggleShown
						? tr::solar_HideFilteredMessagesMenuText(tr::now)
						: tr::solar_ShowFilteredMessagesMenuText(tr::now),
					[=]
					{
						FiltersController::toggleFilteredMessagesShown(peerData);
					},
					*filteredToggleShown
						? &st::menuIconCaptionHide
						: &st::menuIconCaptionShow);
			}
			if (saveDeletedMessages) {
				addedPrimary = true;
				addAction(
					tr::solar_ViewDeletedMenuText(tr::now),
					[=]
					{
						if (const auto window = sessionController->session().tryResolveWindow()) {
							window->showSection(std::make_shared<MessageHistory::SectionMemento>(
								peerData,
								nullptr,
								topicId));
						}
					},
					&st::menuIconArchive);
				addAction({
					.text = tr::solar_ClearDeletedMenuText(tr::now),
					.handler = ClearDeletedMessagesHandler(sessionController, peerData, topicId),
					.icon = &st::menuIconClearAttention,
					.isAttention = true,
				});
			}

			if (addedPrimary) {
				addAction({ .isSeparator = true });
			}

			// Exclusions (Per-Chat Saving / Read / Typing)
			{
				const auto excluded = SolarSettings::getInstance().isSavingExcluded(dialogId);
				addAction(
					excluded
						? (u"\u2713 "_q + tr::solar_SavingExclusions(tr::now))
						: tr::solar_SavingExclusions(tr::now),
					[=] {
						SolarSettings::getInstance().setSavingExcluded(dialogId, !excluded);
					},
					excluded ? &st::menuIconBlock : &st::menuIconArchive);

				const auto addExclusionModeMenu = [=](
						const QString &label,
						ChatExclusionMode current,
						Fn<void(ChatExclusionMode)> setter,
						const style::icon *icon) {
					addAction({
						.text = label,
						.icon = icon,
						.fillSubmenu = [=](not_null<Ui::PopupMenu*> sub) {
							const auto addSub = Ui::Menu::CreateAddActionCallback(sub);
							const auto addMode = [&](ChatExclusionMode mode, QString text) {
								addSub(
									(current == mode) ? (u"\u2713 "_q + text) : std::move(text),
									[=] { setter(mode); },
									nullptr);
							};
							addMode(ChatExclusionMode::GlobalDefault, tr::solar_ExclusionFollowGlobal(tr::now));
							addMode(ChatExclusionMode::ForceAllow, tr::solar_ExclusionForceAllow(tr::now));
							addMode(ChatExclusionMode::ForceBlock, tr::solar_ExclusionForceBlock(tr::now));
						},
					});
				};
				addExclusionModeMenu(
					tr::solar_ReadExclusion(tr::now),
					SolarSettings::getInstance().readExclusion(dialogId),
					[=](ChatExclusionMode m) { SolarSettings::getInstance().setReadExclusion(dialogId, m); },
					&st::menuIconShowInChat);
				addExclusionModeMenu(
					tr::solar_TypingExclusion(tr::now),
					SolarSettings::getInstance().typingExclusion(dialogId),
					[=](ChatExclusionMode m) { SolarSettings::getInstance().setTypingExclusion(dialogId, m); },
					&st::menuIconEdit);

				// Per-Chat Ghost Mode 5/5 (Read Stories, Online Packets, Auto-Offline + Read/Typing).
				addAction({
					.text = tr::solar_PerChatGhostMenu(tr::now),
					.icon = &st::menuIconStealth,
					.fillSubmenu = [=](not_null<Ui::PopupMenu*> sub) {
						const auto addSub = Ui::Menu::CreateAddActionCallback(sub);
						const auto nextMode = [](ChatExclusionMode m) {
							switch (m) {
							case ChatExclusionMode::GlobalDefault: return ChatExclusionMode::ForceAllow;
							case ChatExclusionMode::ForceAllow: return ChatExclusionMode::ForceBlock;
							case ChatExclusionMode::ForceBlock: return ChatExclusionMode::GlobalDefault;
							}
							return ChatExclusionMode::GlobalDefault;
						};
						const auto modeSuffix = [](ChatExclusionMode m) -> QString {
							switch (m) {
							case ChatExclusionMode::GlobalDefault: return u" [·]"_q;
							case ChatExclusionMode::ForceAllow: return u" [✓]"_q;
							case ChatExclusionMode::ForceBlock: return u" [✕]"_q;
							}
							return {};
						};
						auto policy = SolarSettings::getInstance().perChatGhostPolicy(dialogId);
						addSub(
							tr::solar_GhostReadMessages(tr::now)
								+ modeSuffix(SolarSettings::getInstance().readExclusion(dialogId)),
							[=] {
								auto &s = SolarSettings::getInstance();
								s.setReadExclusion(dialogId, nextMode(s.readExclusion(dialogId)));
							},
							&st::menuIconShowInChat);
						addSub(
							tr::solar_GhostReadStories(tr::now) + modeSuffix(policy.readStories),
							[=] {
								auto &s = SolarSettings::getInstance();
								auto p = s.perChatGhostPolicy(dialogId);
								p.readStories = nextMode(p.readStories);
								s.setPerChatGhostPolicy(dialogId, p);
							},
							&st::menuIconShowAll);
						addSub(
							tr::solar_GhostSendTyping(tr::now)
								+ modeSuffix(SolarSettings::getInstance().typingExclusion(dialogId)),
							[=] {
								auto &s = SolarSettings::getInstance();
								s.setTypingExclusion(dialogId, nextMode(s.typingExclusion(dialogId)));
							},
							&st::menuIconEdit);
						addSub(
							tr::solar_GhostSendOnline(tr::now) + modeSuffix(policy.onlinePackets),
							[=] {
								auto &s = SolarSettings::getInstance();
								auto p = s.perChatGhostPolicy(dialogId);
								p.onlinePackets = nextMode(p.onlinePackets);
								s.setPerChatGhostPolicy(dialogId, p);
							},
							&st::menuIconUnmute);
						addSub(
							tr::solar_GhostOfflineAfterOnline(tr::now)
								+ modeSuffix(policy.offlineAfterOnline),
							[=] {
								auto &s = SolarSettings::getInstance();
								auto p = s.perChatGhostPolicy(dialogId);
								p.offlineAfterOnline = nextMode(p.offlineAfterOnline);
								s.setPerChatGhostPolicy(dialogId, p);
							},
							&st::menuIconTTL);
					},
				});
			}

			// Hidden Chats (#PIN)
			if (!user || !user->isSelf()) {
				const auto isHidden = SolarSettings::getInstance().isChatHidden(dialogId);
				addAction(
					isHidden
						? tr::solar_UnhideChatAction(tr::now)
						: tr::solar_HideChatAction(tr::now),
					[=] {
						auto &s = SolarSettings::getInstance();
						if (isHidden) {
							s.unhideChat(dialogId);
						} else if (s.hasHiddenChatsPin()) {
							s.hideChat(dialogId);
						} else {
							Settings::ShowSolarPinBox(
								sessionController,
								tr::solar_HiddenChatsTitle(),
								tr::solar_DuressPinPlaceholder(),
								tr::solar_HiddenChatsAbout(),
								false,
								[=](const QString &pin) {
									auto &inst = SolarSettings::getInstance();
									inst.setHiddenChatsPin(pin);
									inst.hideChat(dialogId);
								});
						}
					},
					isHidden ? &st::menuIconUserShow : &st::menuIconUserHide);
			}

			// Peek Last Seen & Presence History (private users)
			if (user && !user->isBot() && !user->isSelf()) {
				addAction({ .isSeparator = true });
				addAction(
					tr::solar_PeekOnline(tr::now),
					[=] {
						const auto weak = base::make_weak(sessionController);
						user->session().api().request(MTPusers_GetFullUser(
							user->inputUser()
						)).done([=](const MTPusers_UserFull &result) {
							user->owner().processUsers(result.data().vusers());
							if (const auto s = weak.get()) {
								s->showToast(Data::OnlineTextFull(
									user,
									base::unixtime::now()));
							}
						}).fail([=] {
							if (const auto s = weak.get()) {
								s->showToast(Data::OnlineTextFull(
									user,
									base::unixtime::now()));
							}
						}).send();
					},
					&st::menuIconSearch);
				addAction(
					tr::solar_PresenceHistoryAction(tr::now),
					[=] {
						ShowPresenceHistoryBox(sessionController, peerData);
					},
					&st::menuIconInfo);
				const auto isWatched = SolarSettings::getInstance().isPeerWatched(dialogId);
				addAction(
					isWatched
						? tr::solar_UnwatchOnlineAction(tr::now)
						: tr::solar_WatchOnlineAction(tr::now),
					[=] {
						SolarSettings::getInstance().setPeerWatched(dialogId, !isWatched);
					},
					isWatched ? &st::menuIconMute : &st::menuIconUnmute);
			}

			// Summarize Chat (Last 50 messages via SolarBrain, SereinGram RecentLines pattern)
			if (SolarSettings::getInstance().solarBrainEnabled()) {
				addAction(
					tr::solar_SummarizeChatAction(tr::now),
					crl::guard(sessionController, [=] {
						const auto history = peerData->owner().history(peerData);
						QStringList lines;
						constexpr auto kSummaryLimit = 50;
						for (auto i = history->blocks.rbegin();
								i != history->blocks.rend() && lines.size() < kSummaryLimit;
								++i) {
							const auto &messages = (*i)->messages;
							for (auto j = messages.rbegin();
									j != messages.rend() && lines.size() < kSummaryLimit;
									++j) {
								const auto msg = (*j)->data();
								const auto text = msg->originalText().text.trimmed();
								if (!msg->isService() && !text.isEmpty()) {
									lines.push_back(u"%1: %2"_q.arg(msg->from()->name(), text));
								}
							}
						}
						if (lines.isEmpty()) {
							sessionController->showToast(tr::solar_SummaryEmpty(tr::now));
							return;
						}
						std::reverse(lines.begin(), lines.end());
						SolarFeatures::AI::ShowSolarBrainBox(
							sessionController,
							lines.join(u"\n"_q));
					}),
					&st::menuIconStats);
			}

			// Bulk Delete by Period (private chats, groups, megagroups)
			if ((user && !user->isSelf())
				|| peerData->isChat()
				|| peerData->isMegagroup()) {
				addAction({
					.text = tr::solar_BulkDeleteByPeriod(tr::now),
					.icon = &st::menuIconDelete,
					.fillSubmenu = [=](not_null<Ui::PopupMenu*> sub) {
						const auto addSub = Ui::Menu::CreateAddActionCallback(sub);
						addSub(
							tr::solar_BulkDeleteLast100(tr::now),
							DeleteMyMessagesHandler(sessionController, peerData, 100, 0),
							&st::menuIconDelete);
						addSub(
							tr::solar_BulkDeleteLast24h(tr::now),
							DeleteMyMessagesHandler(
								sessionController,
								peerData,
								0,
								base::unixtime::now() - 86400),
							&st::menuIconTTL);
						addSub(
							tr::solar_BulkDeleteLast7d(tr::now),
							DeleteMyMessagesHandler(
								sessionController,
								peerData,
								0,
								base::unixtime::now() - 7 * 86400),
							&st::menuIconTTL);
					},
				});
			}
		},
	});
}

void AddJumpToBeginningAction(PeerData *peerData,
							  Data::Thread *thread,
							  not_null<Window::SessionController*> sessionController,
							  const Window::PeerMenuCallback &addCallback) {
	const auto user = peerData->asUser();
	const auto group = peerData->isChat() ? peerData->asChat() : nullptr;
	const auto chat = peerData->isMegagroup()
						  ? peerData->asMegagroup()
						  : peerData->isChannel()
								? peerData->asChannel()
								: nullptr;
	const auto topic = peerData->isForum() ? thread->asTopic() : nullptr;
	if (!user && !group && !chat && !topic) {
		return;
	}
	if (topic && topic->creating()) {
		return;
	}

	const auto controller = sessionController;
	const auto jumpToDate = [=](auto history, auto callback)
	{
		const auto weak = base::make_weak(controller);
		controller->session().api().resolveJumpToDate(
			history,
			QDate(2013, 8, 1),
			[=](not_null<PeerData*> peer, MsgId id)
			{
				if (weak.get()) {
					// API returns 0 if message "Channel created" (ID: 1) was deleted, which scrolls to the bottom
					if (id.bare == 0) {
						id = MsgId(2);
					}
					callback(peer, id);
				}
			});
	};

	const auto showPeerHistory = [=](auto peer, MsgId id)
	{
		controller->showPeerHistory(
			peer,
			Window::SectionShow::Way::Forward,
			id);
	};

	const auto showTopic = [=](auto topic, MsgId id)
	{
		controller->showTopic(
			topic,
			id,
			Window::SectionShow::Way::Forward);
	};

	addCallback(
		tr::solar_JumpToBeginning(tr::now),
		[=]
		{
			if (user) {
				jumpToDate(controller->session().data().history(user), showPeerHistory);
			} else if (group && !chat) {
				jumpToDate(controller->session().data().history(group), showPeerHistory);
			} else if (chat && !topic) {
				if (!chat->migrateFrom() && chat->availableMinId() == 1) {
					showPeerHistory(chat, 1);
				} else {
					jumpToDate(controller->session().data().history(chat), showPeerHistory);
				}
			} else if (topic) {
				if (topic->isGeneral()) {
					showTopic(topic, 1);
				} else {
					jumpToDate(
						topic,
						[=](not_null<PeerData*>, MsgId id)
						{
							showTopic(topic, id);
						});
				}
			}
		},
		&st::solarToBeginningMenuIcon);
}

void AddOpenChannelAction(PeerData *peerData,
						  not_null<Window::SessionController*> sessionController,
						  const Window::PeerMenuCallback &addCallback) {
	if (!peerData || !peerData->isMegagroup()) {
		return;
	}

	const auto chat = peerData->asMegagroup()->discussionLink();
	if (!chat) {
		return;
	}

	addCallback(
		tr::lng_context_open_channel(tr::now),
		[=]
		{
			sessionController->showPeerHistory(chat, Window::SectionShow::Way::Forward);
		},
		&st::menuIconChannel);
}

void AddShadowBanAction(PeerData *peerData,
						const Window::PeerMenuCallback &addCallback) {
	const auto &settings = SolarSettings::getInstance();
	if (!peerData || !(peerData->isUser() || peerData->isBroadcast()) || !settings.filtersEnabled()) {
		return;
	}

	if (const auto user = peerData->asUser()) {
		if (user->isSelf()) {
			return;
		}
	}

	const auto realId = getDialogIdFromPeer(peerData);
	const auto shadowBanned = SolarSettings::getInstance().isShadowBanned(realId);
	const auto toggleShadowBan = [=]
	{
		if (shadowBanned) {
			SolarSettings::getInstance().removeShadowBan(realId);
		} else {
			SolarSettings::getInstance().addShadowBan(realId);
		}
	};

	addCallback({
		.text = (shadowBanned
					 ? tr::solar_FiltersQuickUnshadowBan(tr::now)
					 : tr::solar_FiltersQuickShadowBan(tr::now)),
		.handler = toggleShadowBan,
		.icon = shadowBanned ? &st::menuIconShowInChat : &st::menuIconStealth,
	});
}

void AddDeleteOwnMessagesAction(PeerData *peerData,
								Data::ForumTopic *topic,
								not_null<Window::SessionController*> sessionController,
								const Window::PeerMenuCallback &addCallback) {
	if (topic) {
		return;
	}
	if (const auto chat = peerData->asChat()) {
		if (!chat->amIn()) {
			return;
		}
	} else if (const auto channel = peerData->asChannel()) {
		if (!channel->isMegagroup() || !channel->amIn()) {
			return;
		}
	} else {
		return;
	}
	addCallback(
		tr::solar_DeleteOwnMessages(tr::now),
		DeleteMyMessagesHandler(sessionController, peerData),
		&st::menuIconTTL);
}

void AddHistoryAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item) {
	if (item->hideEditedBadge()) {
		return;
	}

	const auto edited = item->Get<HistoryMessageEdited>();
	if (!edited) {
		return;
	}

	const auto has = SolarMessages::hasRevisions(item);
	if (!has) {
		return;
	}

	menu->addAction(
		tr::solar_EditsHistoryMenuText(tr::now),
		[=]
		{
			if (const auto window = item->history()->session().tryResolveWindow()) {
				window->showSection(
					std::make_shared<MessageHistory::SectionMemento>(item->history()->peer, item, 0));
			}
		},
		&st::solarEditsHistoryIcon);
}

void AddHideMessageAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item) {
	const auto &settings = SolarSettings::getInstance();
	if (!needToShowItem(settings.showHideMessageInContextMenu())) {
		return;
	}

	if (item->history()->peer->isSelf()) {
		return;
	}

	const auto history = item->history();
	const auto owner = &history->owner();
	menu->addAction(
		tr::solar_ContextHideMessage(tr::now),
		[=]()
		{
			const auto ids = owner->itemOrItsGroup(item);
			for (const auto &fullId : ids) {
				if (const auto current = owner->message(fullId)) {
					SolarState::hide(current);
					current->destroy();
				}
			}
			history->requestChatListMessage();
		},
		&st::menuIconClear);
}

void AddUserMessagesAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item) {
	const auto &settings = SolarSettings::getInstance();
	if (!needToShowItem(settings.showUserMessagesInContextMenu())) {
		return;
	}

	if (!item->isHistoryEntry()) {
		return;
	}

	if (item->history()->peer->isChat() || item->history()->peer->isMegagroup()) {
		menu->addAction(
			tr::solar_UserMessagesMenuText(tr::now),
			[=]
			{
				if (const auto controller = item->history()->session().tryResolveWindow()) {
					const auto peer = item->history()->peer;
					const auto key = (peer && !peer->isUser())
										 ? item->topic()
											   ? Dialogs::Key{item->topic()}
											   : Dialogs::Key{item->history()}
										 : Dialogs::Key{item->history()};
					controller->searchInChat(key, item->from());
				}
			},
			&st::menuIconTTL);
	}
}

void AddMessageDetailsAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item) {
	const auto &settings = SolarSettings::getInstance();
	if (!needToShowItem(settings.showMessageDetailsInContextMenu())) {
		return;
	}

	if (item->isLocal()) {
		return;
	}

	const auto view = item->mainView();
	const auto forwarded = item->Get<HistoryMessageForwarded>();
	const auto views = item->Get<HistoryMessageViews>();
	const auto media = item->media();

	const auto isSticker = media && media->document() && media->document()->sticker();

	const auto emojiPacks = HistoryView::CollectEmojiPacks(item, HistoryView::EmojiPacksSource::Message);
	auto containsSingleCustomEmojiPack = emojiPacks.size() == 1;
	if (!containsSingleCustomEmojiPack && emojiPacks.size() > 1) {
		const auto author = emojiPacks.front().id >> 32;
		auto sameAuthor = true;
		for (const auto &pack : emojiPacks) {
			if (pack.id >> 32 != author) {
				sameAuthor = false;
				break;
			}
		}

		containsSingleCustomEmojiPack = sameAuthor;
	}

	const auto isForwarded = forwarded && !forwarded->story && forwarded->psaType.isEmpty();

	const auto messageId = QString::number(item->id.bare);
	const auto messageDate = base::unixtime::parse(item->date());
	const auto messageEditDate = base::unixtime::parse(view ? view->displayedEditDate() : TimeId(0));

	const auto messageForwardedDate =
		isForwarded && forwarded
			? base::unixtime::parse(forwarded->originalDate)
			: QDateTime();

	const auto
		messageViews = item->hasViews() && item->viewsCount() > 0 ? QString::number(item->viewsCount()) : QString();
	const auto messageForwards = views && views->forwardsCount > 0 ? QString::number(views->forwardsCount) : QString();

	const auto mediaSize = media ? getMediaSize(item) : QString();
	const auto mediaMime = media ? getMediaMime(item) : QString();
	// todo: bitrate (?)
	const auto mediaName = media ? getMediaName(item) : QString();
	const auto mediaResolution = media ? getMediaResolution(item) : QString();
	const auto mediaDC = media ? getMediaDC(item) : QString();

	const auto hasAnyPostField =
		!messageViews.isEmpty() ||
		!messageForwards.isEmpty();

	const auto hasAnyMediaField =
		!mediaSize.isEmpty() ||
		!mediaMime.isEmpty() ||
		!mediaName.isEmpty() ||
		!mediaResolution.isEmpty() ||
		!mediaDC.isEmpty();

	const auto callback = Ui::Menu::CreateAddActionCallback(menu);

	callback(Window::PeerMenuCallback::Args{
		.text = tr::solar_MessageDetailsPC(tr::now),
		.handler = nullptr,
		.icon = &st::menuIconInfo,
		.fillSubmenu = [&](not_null<Ui::PopupMenu*> menu2)
		{
			if (hasAnyPostField) {
				if (!messageViews.isEmpty()) {
					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::menuIconShowInChat,
						tr::solar_MessageDetailsViewsPC(tr::now),
						messageViews
					));
				}

				if (!messageForwards.isEmpty()) {
					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::menuIconViewReplies,
						tr::solar_MessageDetailsSharesPC(tr::now),
						messageForwards
					));
				}

				menu2->addSeparator();
			}

			menu2->addAction(Ui::ContextActionWithSubText(
				menu2->menu(),
				st::menuIconInfo,
				QString("ID"),
				messageId
			));

			menu2->addAction(Ui::ContextActionWithSubText(
				menu2->menu(),
				st::menuIconSchedule,
				tr::solar_MessageDetailsDatePC(tr::now),
				formatDateTime(messageDate)
			));

			if (view && view->displayedEditDate()) {
				menu2->addAction(Ui::ContextActionWithSubText(
					menu2->menu(),
					st::menuIconEdit,
					tr::solar_MessageDetailsEditedDatePC(tr::now),
					formatDateTime(messageEditDate)
				));
			}

			if (isForwarded) {
				menu2->addAction(Ui::ContextActionWithSubText(
					menu2->menu(),
					st::menuIconTTL,
					tr::solar_MessageDetailsForwardedDatePC(tr::now),
					formatDateTime(messageForwardedDate)
				));
			}

			if (item->out()) {
				const ID uid = item->history()->owner().session().userId().bare & PeerId::kChatTypeMask;
				const ID did = getDialogIdFromPeer(item->history()->peer);
				if (const auto readTs = SolarDatabase::getSpyMessageReadDate(uid, did, item->id.bare); readTs > 0) {
					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::menuIconShowInChat,
						tr::solar_ReadDate(tr::now),
						formatDateTime(base::unixtime::parse(readTs))
					));
				}
				if (const auto contentsTs = SolarDatabase::getSpyMessageContentsReadDate(uid, did, item->id.bare); contentsTs > 0) {
					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::menuIconTTL,
						tr::solar_ContentsReadDate(tr::now),
						formatDateTime(base::unixtime::parse(contentsTs))
					));
				}
			}

			if (media && hasAnyMediaField) {
				menu2->addSeparator();

				if (!mediaSize.isEmpty()) {
					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::menuIconDownload,
						tr::solar_MessageDetailsFileSizePC(tr::now),
						mediaSize
					));
				}

				if (!mediaMime.isEmpty()) {
					const auto mime = Core::MimeTypeForName(mediaMime);

					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::menuIconShowAll,
						tr::solar_MessageDetailsMimeTypePC(tr::now),
						mime.name()
					));
				}

				if (!mediaName.isEmpty()) {
					auto const shortified = mediaName.length() > 20 ? "…" + mediaName.right(20) : mediaName;

					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::solarEditsHistoryIcon,
						tr::solar_MessageDetailsFileNamePC(tr::now),
						shortified,
						[=]
						{
							QGuiApplication::clipboard()->setText(mediaName);
						}
					));
				}

				if (!mediaResolution.isEmpty()) {
					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::menuIconStats,
						tr::solar_MessageDetailsResolutionPC(tr::now),
						mediaResolution
					));
				}

				if (!mediaDC.isEmpty()) {
					menu2->addAction(Ui::ContextActionWithSubText(
						menu2->menu(),
						st::menuIconBoosts,
						tr::solar_MessageDetailsDatacenterPC(tr::now),
						mediaDC
					));
				}

				if (isSticker) {
					const auto authorId = getUserIdFromPackId(media->document()->sticker()->set.id);

					if (authorId != 0) {
						menu2->addAction(Ui::ContextActionStickerAuthor(
							menu2->menu(),
							&item->history()->session(),
							authorId
						));
					}
				}
			}

			if (containsSingleCustomEmojiPack) {
				const auto authorId = getUserIdFromPackId(emojiPacks.front().id);

				if (authorId != 0) {
					menu2->addAction(Ui::ContextActionStickerAuthor(
						menu2->menu(),
						&item->history()->session(),
						authorId
					));
				}
			}
		},
	});
}

void AddRepeatMessageAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item, HistoryView::Context context) {
	const auto &settings = SolarSettings::getInstance();
	if (!needToShowItem(settings.showRepeatMessageInContextMenu())) {
		return;
	}

	if (!item || !item->isHistoryEntry() || item->isService() || item->isLocal() || !item->allowsForward() || item->id <= 0) {
		return;
	}

	const auto history = item->history();
	const auto peer = history->peer;
	if (!peer->isUser() && !peer->isChat() && !peer->isMegagroup() && !peer->isGigagroup()) {
		return;
	}

	const auto itemId = item->fullId();
	const auto session = &history->session();

	menu->addAction(
		tr::solar_RepeatMessage(tr::now),
		[=]
		{
			const auto sendAs = (peer->isUser() || peer->isChat() || history->peer->isMonoforum())
				? nullptr
				: session->sendAsPeers().resolveChosen(peer).get();

			const auto inRepliesView = (context == HistoryView::Context::Replies);
			const auto replyTo = item->replyTo();
			const auto hasReply = replyTo.messageId.msg != 0;
			const auto shiftPressed = base::IsShiftPressed();

			const auto useNoQuote = shiftPressed || (inRepliesView && !history->peer->isForum());
			const auto preserveReply = inRepliesView ? hasReply : (hasReply && shiftPressed);

			const auto currentItem = history->owner().message(itemId);
			if (!currentItem) {
				return;
			}

			auto action = Api::SendAction(
				history,
				Api::SendOptions{ .sendAs = sendAs });
			if (history->peer->amMonoforumAdmin()) {
				action.replyTo.monoforumPeerId = currentItem->sublistPeerId();
			}
			action.clearDraft = false;

			applyGhostScheduling(session, action.options);

			if (currentItem->topic()) {
				action.replyTo.topicRootId = currentItem->topicRootId();
			}

			if (preserveReply) {
				action.replyTo.messageId = replyTo.messageId;
			}

			if (useNoQuote) {
				if (currentItem->richPage() && session->premium()) {
					if (preserveReply) {
						crl::async([=] {
							SolarForward::forwardRichMessage(session, itemId, action);
						});
					} else {
						const auto forwardDraft = Data::ForwardDraft{
							.ids = MessageIdsList{ itemId },
							.options = Data::ForwardOptions::NoSenderNames,
						};
						auto resolvedDraft = history->resolveForwardDraft(forwardDraft);
						session->api().forwardMessages(
							std::move(resolvedDraft),
							action,
							[] {});
					}
				} else {
					auto message = ApiWrap::MessageToSend(action);
					const auto media = currentItem->media();
					if (!currentItem->originalText().text.isEmpty()) {
						message.textWithTags = {
							currentItem->originalText().text,
							TextUtilities::ConvertEntitiesToTextTags(
								currentItem->originalText().entities),
						};
					}
					if (media) {
						if (const auto photo = media->photo()) {
							Api::SendExistingPhoto(std::move(message), photo);
						} else if (const auto document = media->document()) {
							Api::SendExistingDocument(std::move(message), document);
						}
					} else {
						session->api().sendMessage(std::move(message));
					}
				}
			} else {
				const auto forwardDraft = Data::ForwardDraft{
					.ids = MessageIdsList{ itemId },
					.options = Data::ForwardOptions::PreserveInfo,
				};
				auto resolvedDraft = history->resolveForwardDraft(forwardDraft);

				if (SolarForward::isFullSolarForwardNeeded(currentItem)) {
					crl::async([=]
					{
						SolarForward::forwardMessages(session, action, false, resolvedDraft);
					});
				} else if (SolarForward::isSolarForwardNeeded(currentItem)) {
					crl::async([=]
					{
						SolarForward::intelligentForward(session, action, resolvedDraft);
					});
				} else {
					session->api().forwardMessages(std::move(resolvedDraft), action, [] {});
				}
			}
		},
		&st::solarRepeatMenuIcon);
}

void AddReadUntilAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item) {
	const auto group = item->history()->owner().groups().find(item);
	const auto readItem = group ? group->items.back().get() : item;
	if (!readItem->isHistoryEntry()
		|| readItem->isLocal()
		|| readItem->out()
		|| readItem->isDeleted()
		|| readItem->history()->peer->isSelf()) {
		return;
	}

	const auto &ghost = SolarSettings::ghost(&readItem->history()->session());
	if (ghost.sendReadMessages()) {
		return;
	}

	menu->addAction(
		tr::solar_ReadUntilMenuText(tr::now),
		[=]
		{
			readHistory(readItem);
			const auto media = readItem->media();
			if (media
				&& media->ttlSeconds() <= 0
				&& !readItem->out()) {
				const auto ids = MTP_vector<MTPint>(1, MTP_int(readItem->id));
				if (const auto channel = readItem->history()->peer->asChannel()) {
					readItem->history()->session().api().request(
						MTPchannels_ReadMessageContents(
						channel->inputChannel(),
						ids)).send();
				} else {
					readItem->history()->session().api().request(
						MTPmessages_ReadMessageContents(ids)
					).done([=](const MTPmessages_AffectedMessages &result)
					{
						readItem->history()->session().api()
							.applyAffectedMessages(
							readItem->history()->peer,
							result);
					}).send();
				}
				readItem->markContentsRead();
			}
		},
		&st::menuIconShowInChat);
}

void AddBurnAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item) {
	if (!item->media() || item->media()->ttlSeconds() <= 0 || item->out() ||
		!item->hasUnreadMediaFlag()) {
		return;
	}

	menu->addAction(
		tr::solar_ExpireMediaContextMenuText(tr::now),
		[=]
		{
			const auto ids = MTP_vector<MTPint>(1, MTP_int(item->id));

			item->history()->session().api().request(MTPmessages_ReadMessageContents(
					ids
				)).done([=](const MTPmessages_AffectedMessages &result)
				{
					item->history()->session().api().applyAffectedMessages(
						item->history()->peer,
						result);
					item->markContentsRead();
				}).send();
		},
		&st::menuIconTTLAny);
}

void AddCreateFilterAction(not_null<Ui::PopupMenu*> menu,
						   not_null<Window::SessionController*> controller,
						   HistoryItem *item,
						   const QString &selectedText) {
	const auto &settings = SolarSettings::getInstance();
	if (!needToShowItem(settings.showAddFilterInContextMenu()) || !settings.filtersEnabled()) {
		return;
	}

	if (!item || selectedText.isEmpty()) {
		return;
	}

	menu->addAction(
		tr::solar_RegexFilterQuickAdd(tr::now),
		[=]
		{
			RegexFilter filter;
			filter.text = selectedText.toStdString();
			filter.enabled = true;
			filter.caseInsensitive = true;
			filter.reversed = false;

			controller->show(Settings::RegexEditBox(&filter, {}, getDialogIdFromPeer(item->history()->peer), true));
		},
		&st::menuIconAddToFolder);
}

void AddDeletedQuoteAction(
		not_null<Ui::PopupMenu*> menu,
		HistoryItem *item,
		Fn<void(QString)> insertText) {
	if (!item || !item->isDeleted() || !insertText) {
		return;
	}
	const auto authorName = item->from()->name();
	auto deletedText = item->originalText().text.trimmed();
	if (deletedText.isEmpty()) {
		if (const auto media = item->media()) {
			auto label = media->notificationText().text;
			if (const auto document = media->document()) {
				if (!document->isVoiceMessage()
					&& !document->isVideoMessage()
					&& !document->isAnimation()
					&& !document->isVideoFile()
					&& !document->isSong()
					&& !document->sticker()
					&& !document->isImage()) {
					const auto name = document->filename();
					if (!name.isEmpty()) {
						label = name;
					}
				}
			}
			if (!label.isEmpty()) {
				deletedText = u"[%1]"_q.arg(label);
			}
		}
	}
	if (deletedText.isEmpty() && authorName.isEmpty()) {
		return;
	}
	const auto quote = authorName.isEmpty()
		? (deletedText + u"\n"_q)
		: u"%1: «%2»\n"_q.arg(authorName, deletedText);
	menu->addAction(
		tr::solar_QuoteDeletedAction(tr::now),
		[insertText = std::move(insertText), quote] {
			insertText(quote);
		},
		&st::menuIconReply);
}

void AddForwardToSavedCleanAction(
		not_null<Ui::PopupMenu*> menu,
		not_null<Window::SessionController*> controller,
		const MessageIdsList &ids,
		Fn<void()> onDone) {
	if (ids.empty()) {
		return;
	}
	menu->addAction(
		tr::solar_ToSavedClean(tr::now),
		crl::guard(controller, [=, onDone = std::move(onDone)] {
			Window::ForwardToSelf(
				controller->uiShow(),
				Data::ForwardDraft{
					.ids = ids,
					.options = Data::ForwardOptions::NoSenderNames,
				});
			if (onDone) {
				onDone();
			}
		}),
		&st::menuIconSavedMessages);
}

void AddForwardToSavedCleanAction(
		not_null<Ui::PopupMenu*> menu,
		not_null<Window::SessionController*> controller,
		const std::vector<not_null<HistoryItem*>> &items,
		Fn<void()> onDone) {
	MessageIdsList ids;
	ids.reserve(items.size());
	for (const auto &it : items) {
		ids.push_back(it->fullId());
	}
	AddForwardToSavedCleanAction(
		menu,
		controller,
		ids,
		std::move(onDone));
}

} // namespace SolarUi
