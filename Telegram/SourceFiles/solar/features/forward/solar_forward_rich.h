// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "api/api_common.h"
#include "history/history_item.h"
#include "main/main_session.h"

namespace SolarForward {

bool forwardRichMessage(
	not_null<Main::Session*> session,
	FullMsgId itemId,
	const Api::SendAction &action,
	Fn<bool()> cancelled = nullptr);

} // namespace SolarForward
