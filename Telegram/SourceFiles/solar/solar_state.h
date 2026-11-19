// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "history/history.h"
#include "history/history_item.h"

namespace Main {
class Session;
} // namespace Main

namespace SolarState {

void hide(PeerId peerId, MsgId messageId);
void hide(not_null<HistoryItem*> item);
bool isHidden(PeerId peerId, MsgId messageId);
bool isHidden(not_null<HistoryItem*> item);

void setDisableGhostModeOnStoryClose(Main::Session *session);
void disableGhostModeOnStoryClose(Main::Session *session);

}
