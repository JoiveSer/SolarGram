// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "data/data_document.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_peer_menu.h"
#include "window/window_session_controller.h"

#include "solar/solar_settings.h"

namespace HistoryView {
enum class Context : char;
}

namespace SolarUi {

bool needToShowItem(ContextMenuVisibility state);

void AddSolarGramActions(PeerData *peerData,
							   Data::Thread *thread,
							   not_null<Window::SessionController*> sessionController,
							   const Window::PeerMenuCallback &addCallback);

void AddJumpToBeginningAction(PeerData *peerData,
							  Data::Thread *thread,
							  not_null<Window::SessionController*> sessionController,
							  const Window::PeerMenuCallback &addCallback);

void AddShadowBanAction(PeerData *peerData,
						const Window::PeerMenuCallback &addCallback);
void AddOpenChannelAction(PeerData *peerData,
						  not_null<Window::SessionController*> sessionController,
						  const Window::PeerMenuCallback &addCallback);
void AddDeleteOwnMessagesAction(PeerData *peerData,
								Data::ForumTopic *topic,
								not_null<Window::SessionController*> sessionController,
								const Window::PeerMenuCallback &addCallback);

void AddHistoryAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item);
void AddHideMessageAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item);
void AddUserMessagesAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item);
void AddMessageDetailsAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item);
void AddRepeatMessageAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item, HistoryView::Context context);
void AddReadUntilAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item);
void AddBurnAction(not_null<Ui::PopupMenu*> menu, HistoryItem *item);
void AddCreateFilterAction(not_null<Ui::PopupMenu*> menu,
						   not_null<Window::SessionController*> controller,
						   HistoryItem *item,
						   const QString &selectedText);

}
