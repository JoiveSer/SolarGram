// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

QString IDString(not_null<PeerData*> peer);
QString IDString(MsgId topicRootId);

rpl::producer<TextWithEntities> IDValue(not_null<PeerData*> peer);
rpl::producer<TextWithEntities> IDValue(MsgId topicRootId);