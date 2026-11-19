// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/solar_state.h"

#include "solar/solar_settings.h"

namespace SolarState {

std::unordered_map<PeerId, std::unordered_set<MsgId>> hiddenMessages;
Main::Session *disableGhostModeOnStoryCloseSession = nullptr;

void hide(PeerId peerId, MsgId messageId) {
	hiddenMessages[peerId].insert(messageId);
}

void hide(not_null<HistoryItem*> item) {
	hide(item->history()->peer->id, item->id);
}

bool isHidden(PeerId peerId, MsgId messageId) {
	const auto it = hiddenMessages.find(peerId);
	if (it != hiddenMessages.end()) {
		return it->second.contains(messageId);
	}
	return false;
}

bool isHidden(not_null<HistoryItem*> item) {
	return isHidden(item->history()->peer->id, item->id);
}

void setDisableGhostModeOnStoryClose(Main::Session *session) {
	disableGhostModeOnStoryCloseSession = session;
}

void disableGhostModeOnStoryClose(Main::Session *session) {
	if (disableGhostModeOnStoryCloseSession != session) {
		return;
	}
	disableGhostModeOnStoryCloseSession = nullptr;
	if (session) {
		SolarSettings::ghost(session).setGhostModeEnabled(false);
	}
}

}
