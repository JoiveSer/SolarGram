// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/ui/utils/solar_profile_values.h"

#include "solar/solar_settings.h"
#include "solar/utils/telegram_helpers.h"
#include "data/data_peer.h"
#include "lang/lang_text_entity.h"

constexpr auto kMaxChannelId = -1000000000000;

QString IDString(const not_null<PeerData*> peer) {
	auto resultId = QString::number(getBareID(peer));

	const auto &settings = SolarSettings::getInstance();
	if (settings.showPeerId() == PeerIdDisplay::BotApi) {
		if (peer->isChannel()) {
			resultId = QString::number(peerToChannel(peer->id).bare - kMaxChannelId).prepend("-");
		} else if (peer->isChat()) {
			resultId = resultId.prepend("-");
		}
	}

	return resultId;
}

QString IDString(MsgId topicRootId) {
	return QString::number(topicRootId.bare);
}

rpl::producer<TextWithEntities> IDValue(not_null<PeerData*> peer) {
	return SolarSettings::getInstance().showPeerIdValue(
	) | rpl::map([=](PeerIdDisplay display) {
		return (display == PeerIdDisplay::Hidden)
			? TextWithEntities()
			: tr::marked(IDString(peer));
	});
}

rpl::producer<TextWithEntities> IDValue(MsgId topicRootId) {
	return SolarSettings::getInstance().showPeerIdValue(
	) | rpl::map([=](PeerIdDisplay display) {
		return (display == PeerIdDisplay::Hidden)
			? TextWithEntities()
			: tr::marked(IDString(topicRootId));
	});
}
