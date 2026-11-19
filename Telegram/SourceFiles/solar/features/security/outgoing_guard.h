// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include <QString>

class PeerData;
struct TextWithTags;

namespace SolarFeatures::OutgoingGuard {

// Cleans common tracking query parameters (utm_*, fbclid, gclid, si, etc.)
// from a standalone URL while preserving signature parameters (x-amz-*, sig, signature).
[[nodiscard]] QString StripTrackingUrl(const QString &original);

// Strips tracking query parameters from all URLs inside textWithTags (and link tags),
// keeping tag offsets and lengths aligned.
void StripTrackingFromTextWithTags(TextWithTags &textWithTags);

// Normalizes accidental ALL-CAPS outgoing messages while preserving /commands, URLs, and `code`.
void ApplyAntiCaps(QString &text);

// Records 5-digit verification codes from recent incoming service notifications (777000) in RAM.
void ObserveIncomingServiceMessage(const QString &text);

// Returns a matching recently received 5-digit login code if present in outgoing text, or empty.
[[nodiscard]] QString FindLeakedLoginCode(const QString &text);

// Allows one explicit user-confirmed send pass to bypass the login-code warning dialog.
void ApproveCodeOnce();
[[nodiscard]] bool TakeApprovedCodeGuard();

} // namespace SolarFeatures::OutgoingGuard
