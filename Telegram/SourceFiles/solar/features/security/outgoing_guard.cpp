// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/features/security/outgoing_guard.h"

#include <crl/crl_time.h>
#include "ui/text/text_entity.h"

#include <QRegularExpression>
#include <QStringList>
#include <QUrl>

#include <algorithm>
#include <deque>
#include <mutex>
#include <vector>

namespace SolarFeatures::OutgoingGuard {
namespace {

constexpr auto kCodeTtlMs = crl::time(15 * 60 * 1000); // 15 minutes in RAM only.
constexpr auto kMaxTrackedCodes = size_t(12);

struct TrackedCode {
	QString code;
	crl::time receivedAt = 0;
};

std::mutex &CodesMutex() {
	static std::mutex mutex;
	return mutex;
}

std::deque<TrackedCode> &RecentCodes() {
	static std::deque<TrackedCode> codes;
	return codes;
}

bool &ApprovedOnceFlag() {
	static bool approved = false;
	return approved;
}

void PruneExpiredLocked(crl::time now) {
	auto &codes = RecentCodes();
	while (!codes.empty() && (now - codes.front().receivedAt > kCodeTtlMs)) {
		codes.pop_front();
	}
}

[[nodiscard]] bool IsHan(char32_t ch) {
	return QChar::script(ch) == QChar::Script_Han;
}

[[nodiscard]] bool IsLatinOrDigit(char32_t ch) {
	return (ch >= 'a' && ch <= 'z')
		|| (ch >= 'A' && ch <= 'Z')
		|| (ch >= '0' && ch <= '9');
}

[[nodiscard]] const QRegularExpression &ForeignAgentDisclaimerRegex() {
	static const auto kPattern = QRegularExpression(
		uR"((?:^|\n+)[ \t]*(?:18\+[ \t]+)?ДАННОЕ[ \t]+СООБЩЕНИЕ[ \t]*\([ \t]*МАТЕРИАЛ[ \t]*\)[ \t]+СОЗДАНО[ \t]+И[ \t]*(?:\([ \t]*ИЛИ[ \t]*\)[ \t]*)?РАСПРОСТРАНЕНО[^\n]{0,480}?ИНОСТРАННОГО[ \t]+АГЕНТА\.?[ \t]*(?=\n|$))"_q,
		QRegularExpression::CaseInsensitiveOption
			| QRegularExpression::UseUnicodePropertiesOption);
	return kPattern;
}

[[nodiscard]] int AdjustOffsetForEdits(
		int pos,
		const std::vector<std::tuple<int, int, int>> &edits) {
	// Each edit is (oldStart, oldEnd, delta = newLen - oldLen).
	auto shift = 0;
	for (const auto &[start, end, delta] : edits) {
		if (pos >= end) {
			shift += delta;
		} else if (pos > start) {
			const auto rel = pos - start;
			const auto newLen = (end - start) + delta;
			shift += std::min(rel, newLen) - rel;
		}
	}
	return pos + shift;
}

} // namespace

QString StripTrackingUrl(const QString &original) {
	if (original.size() > 16384
		|| original.trimmed() != original
		|| original.contains(QChar::LineFeed)
		|| original.contains(u' ')) {
		return original;
	}
	const auto url = QUrl(original, QUrl::StrictMode);
	if (!url.isValid()
		|| url.host().isEmpty()
		|| !url.userInfo().isEmpty()
		|| (url.scheme() != u"https"_q && url.scheme() != u"http"_q)
		|| !url.hasQuery()) {
		return original;
	}
	auto kept = QStringList();
	auto changed = false;
	const auto start = original.indexOf(u'?');
	if (start < 0) {
		return original;
	}
	const auto hash = original.indexOf(u'#', start + 1);
	const auto end = (hash < 0) ? original.size() : hash;
	for (const auto &part : QStringView(original).sliced(
			start + 1,
			end - start - 1).split(u'&')) {
		const auto equals = part.indexOf(u'=');
		const auto key = (equals < 0) ? part : part.first(equals);
		const auto name = QUrl::fromPercentEncoding(key.toUtf8()).toLower();
		if (name == u"signature"_q
			|| name == u"sig"_q
			|| name.startsWith(u"x-amz-"_q)
			|| name.startsWith(u"x-goog-"_q)) {
			return original;
		}
		const auto tracking = name.startsWith(u"utm_"_q)
			|| name == u"fbclid"_q
			|| name == u"gclid"_q
			|| name == u"dclid"_q
			|| name == u"msclkid"_q
			|| name == u"twclid"_q
			|| name == u"yclid"_q
			|| name == u"ysclid"_q
			|| name == u"mc_cid"_q
			|| name == u"mc_eid"_q
			|| name == u"igshid"_q
			|| name == u"ref_src"_q
			|| name == u"si"_q
			|| name == u"mkt_tok"_q;
		changed |= tracking;
		if (!tracking && !part.isEmpty()) {
			kept.push_back(part.toString());
		}
	}
	if (!changed) {
		return original;
	}
	return original.first(start)
		+ (kept.isEmpty() ? QString() : (u'?' + kept.join(u'&')))
		+ original.sliced(end);
}

void StripTrackingFromTextWithTags(TextWithTags &textWithTags) {
	// Clean URL values embedded inside markdown/custom link tags first.
	for (auto &tag : textWithTags.tags) {
		if (tag.id.startsWith(u"http://"_q, Qt::CaseInsensitive)
			|| tag.id.startsWith(u"https://"_q, Qt::CaseInsensitive)) {
			tag.id = StripTrackingUrl(tag.id);
		}
	}

	if (textWithTags.text.isEmpty()
		|| (!textWithTags.text.contains(u"http://"_q, Qt::CaseInsensitive)
			&& !textWithTags.text.contains(u"https://"_q, Qt::CaseInsensitive))) {
		return;
	}

	static const auto kUrlRe = QRegularExpression(
		uR"(https?://[^\s<>"'\)\]]+)"_q,
		QRegularExpression::CaseInsensitiveOption);

	auto edits = std::vector<std::tuple<int, int, int>>();
	auto newText = QString();
	newText.reserve(textWithTags.text.size());
	auto cursor = 0;

	auto it = kUrlRe.globalMatch(textWithTags.text);
	while (it.hasNext()) {
		const auto match = it.next();
		auto matchStart = match.capturedStart();
		auto matchEnd = match.capturedEnd();

		// Trim trailing sentence punctuation commonly attached to pasted URLs.
		while (matchEnd > matchStart) {
			const auto ch = textWithTags.text.at(matchEnd - 1);
			if (ch == u'.' || ch == u',' || ch == u';' || ch == u'!' || ch == u'?') {
				--matchEnd;
			} else {
				break;
			}
		}
		if (matchEnd <= matchStart) {
			continue;
		}
		const auto rawUrl = textWithTags.text.mid(matchStart, matchEnd - matchStart);
		const auto cleaned = StripTrackingUrl(rawUrl);
		if (cleaned != rawUrl) {
			newText.append(QStringView(textWithTags.text).sliced(cursor, matchStart - cursor));
			newText.append(cleaned);
			cursor = matchEnd;
			edits.emplace_back(matchStart, matchEnd, int(cleaned.size() - rawUrl.size()));
		}
	}

	if (edits.empty()) {
		return;
	}
	newText.append(QStringView(textWithTags.text).sliced(cursor));

	for (auto &tag : textWithTags.tags) {
		const auto origStart = tag.offset;
		const auto origEnd = tag.offset + tag.length;
		const auto newStart = AdjustOffsetForEdits(origStart, edits);
		const auto newEnd = AdjustOffsetForEdits(origEnd, edits);
		tag.offset = newStart;
		tag.length = std::max(0, newEnd - newStart);
	}
	textWithTags.text = std::move(newText);
}

void ApplyAntiCaps(QString &text) {
	const auto trimmed = text.trimmed();
	if (trimmed.isEmpty() || trimmed.startsWith(u'/')) {
		return;
	}

	auto letters = 0;
	auto uppers = 0;
	auto inCode = false;
	for (auto i = 0; i < text.size(); ++i) {
		const auto ch = text.at(i);
		if (ch == u'`') {
			inCode = !inCode;
			continue;
		}
		if (inCode) {
			continue;
		}
		if (ch.isLetter()) {
			++letters;
			if (ch.isUpper()) {
				++uppers;
			}
		}
	}

	if (letters < 4 || (uppers * 100 < letters * 75)) {
		return;
	}

	// Lowercase letters cleanly outside inline `code` spans and URL tokens.
	auto urlMask = std::vector<bool>(text.size(), false);
	static const auto kUrlRe = QRegularExpression(uR"(https?://\S+)"_q);
	auto it = kUrlRe.globalMatch(text);
	while (it.hasNext()) {
		const auto m = it.next();
		for (auto i = m.capturedStart(); i < m.capturedEnd() && i < text.size(); ++i) {
			urlMask[i] = true;
		}
	}

	inCode = false;
	auto firstSentenceChar = true;
	for (auto i = 0; i < text.size(); ++i) {
		const auto ch = text.at(i);
		if (ch == u'`') {
			inCode = !inCode;
			continue;
		}
		if (inCode || urlMask[i] || !ch.isLetter()) {
			if (ch == u'.' || ch == u'!' || ch == u'?' || ch == u'\n') {
				firstSentenceChar = true;
			}
			continue;
		}
		if (firstSentenceChar) {
			text[i] = ch.toUpper();
			firstSentenceChar = false;
		} else {
			text[i] = ch.toLower();
		}
	}
}

void ApplyTextReplacements(TextWithTags &textWithTags) {
	if (textWithTags.text.isEmpty()) {
		return;
	}
	static const auto kReplacements = std::array<std::pair<QLatin1String, QString>, 12>{{
		{ QLatin1String(":shrug:"), QString::fromUtf8("¯\\_(ツ)_/¯") },
		{ QLatin1String(":lenny:"), QString::fromUtf8("( ͡° ͜ʖ ͡°)") },
		{ QLatin1String(":tableflip:"), QString::fromUtf8("(╯°□°)╯︵ ┻━┻") },
		{ QLatin1String(":unflip:"), QString::fromUtf8("┬─┬ ノ( ゜-゜ノ)") },
		{ QLatin1String(":disapprove:"), QString::fromUtf8("ಠ_ಠ") },
		{ QLatin1String(":check:"), QString::fromUtf8("✓") },
		{ QLatin1String(":cross:"), QString::fromUtf8("✗") },
		{ QLatin1String(":tm:"), QString::fromUtf8("™") },
		{ QLatin1String("<-->"), QString::fromUtf8("↔") },
		{ QLatin1String("-->"), QString::fromUtf8("→") },
		{ QLatin1String("<--"), QString::fromUtf8("←") },
		{ QLatin1String("!="), QString::fromUtf8("≠") },
	}};
	static const auto kTokenRe = QRegularExpression(
		uR"((:shrug:|:lenny:|:tableflip:|:unflip:|:disapprove:|:check:|:cross:|:tm:|<-->|-->|<--|!=))"_q);

	const auto &src = textWithTags.text;
	auto inCode = std::vector<bool>(src.size(), false);
	auto codeActive = false;
	for (auto i = 0; i < src.size(); ++i) {
		if (src.at(i) == u'`') {
			codeActive = !codeActive;
			inCode[i] = true;
		} else {
			inCode[i] = codeActive;
		}
	}

	std::vector<std::tuple<int, int, int>> edits;
	QString out;
	out.reserve(src.size());
	int cursor = 0;

	auto it = kTokenRe.globalMatch(src);
	while (it.hasNext()) {
		const auto m = it.next();
		const auto start = m.capturedStart();
		const auto end = m.capturedEnd();
		if (start < 0 || end <= start || inCode[start]) {
			continue;
		}
		const auto token = m.captured(1);
		for (const auto &[k, rep] : kReplacements) {
			if (token == k) {
				out.append(QStringView(src).sliced(cursor, start - cursor));
				out.append(rep);
				cursor = end;
				edits.emplace_back(start, end, int(rep.size() - token.size()));
				break;
			}
		}
	}
	if (edits.empty()) {
		return;
	}
	out.append(QStringView(src).sliced(cursor));

	for (auto &tag : textWithTags.tags) {
		const auto origStart = tag.offset;
		const auto origEnd = tag.offset + tag.length;
		const auto newStart = AdjustOffsetForEdits(origStart, edits);
		const auto newEnd = AdjustOffsetForEdits(origEnd, edits);
		tag.offset = newStart;
		tag.length = std::max(0, newEnd - newStart);
	}
	textWithTags.text = std::move(out);
}

void ApplyPanguSpacing(TextWithTags &textWithTags) {
	const auto &src = textWithTags.text;
	const auto length = int(src.size());
	if (length < 2) {
		return;
	}

	auto protectedBoundaries = std::vector<int>(length + 1, 0);
	auto openBacktick = -1;
	for (auto i = 0; i < length; ++i) {
		if (src.at(i) == u'`') {
			if (openBacktick < 0) {
				openBacktick = i;
			} else {
				++protectedBoundaries[openBacktick];
				--protectedBoundaries[i + 1];
				openBacktick = -1;
			}
		}
	}
	for (const auto &tag : textWithTags.tags) {
		const auto isProtected = (tag.id == u"code"_q)
			|| tag.id.startsWith(u"pre"_q)
			|| tag.id.contains(u"://"_q)
			|| tag.id.startsWith(u"mention://"_q);
		if (!isProtected) {
			continue;
		}
		const auto start = std::clamp(tag.offset, 0, length);
		const auto end = std::clamp(tag.offset + tag.length, start, length);
		if (start < end) {
			++protectedBoundaries[start];
			--protectedBoundaries[end];
		}
	}

	QString out;
	out.reserve(length + 16);
	auto before = std::vector<int>(length + 1);
	auto after = std::vector<int>(length + 1);
	auto previous = char32_t(0);
	auto depth = 0;

	for (auto i = 0; i != length;) {
		depth += protectedBoundaries[i];
		const auto first = src.at(i);
		const auto surrogate = first.isHighSurrogate()
			&& (i + 1 < length)
			&& src.at(i + 1).isLowSurrogate();
		const auto ch = surrogate
			? QChar::surrogateToUcs4(first, src.at(i + 1))
			: char32_t(first.unicode());
		before[i] = int(out.size());
		if (depth == 0 && ((IsHan(previous) && IsLatinOrDigit(ch))
			|| (IsLatinOrDigit(previous) && IsHan(ch)))) {
			out += u' ';
		}
		after[i] = int(out.size());
		out += first;
		if (surrogate) {
			++i;
			depth += protectedBoundaries[i];
			before[i] = after[i] = int(out.size());
			out += src.at(i);
		}
		++i;
		previous = ch;
	}
	before[length] = after[length] = int(out.size());
	if (out.size() == length) {
		return;
	}

	for (auto &tag : textWithTags.tags) {
		const auto origStart = std::clamp(tag.offset, 0, length);
		const auto origEnd = std::clamp(tag.offset + tag.length, origStart, length);
		const auto newStart = after[origStart];
		const auto newEnd = before[origEnd];
		tag.offset = newStart;
		tag.length = std::max(0, newEnd - newStart);
	}
	textWithTags.text = std::move(out);
}

void StripForeignAgentFromText(TextWithEntities &text) {
	if (text.text.isEmpty() || text.text.size() < 32) {
		return;
	}
	const auto &src = text.text;
	std::vector<std::tuple<int, int, int>> edits;
	QString out;
	out.reserve(src.size());
	auto cursor = 0;

	auto it = ForeignAgentDisclaimerRegex().globalMatch(src);
	while (it.hasNext()) {
		const auto m = it.next();
		const auto start = std::max(cursor, int(m.capturedStart()));
		auto end = int(m.capturedEnd());
		if (end <= start) {
			continue;
		}
		if (start == 0) {
			while (end < src.size()
				&& (src[end] == u'\n' || src[end] == u'\r' || src[end] == u' ')) {
				++end;
			}
		}
		out.append(QStringView(src).sliced(cursor, start - cursor));
		cursor = end;
		edits.emplace_back(start, end, -(end - start));
	}
	if (edits.empty()) {
		return;
	}
	out.append(QStringView(src).sliced(cursor));

	EntitiesInText adjusted;
	adjusted.reserve(text.entities.size());
	const auto maxLen = int(out.size());
	for (const auto &entity : text.entities) {
		const auto origStart = entity.offset();
		const auto origEnd = origStart + entity.length();
		const auto newStart = std::clamp(
			AdjustOffsetForEdits(origStart, edits),
			0,
			maxLen);
		const auto newEnd = std::clamp(
			AdjustOffsetForEdits(origEnd, edits),
			newStart,
			maxLen);
		const auto newLen = newEnd - newStart;
		if (newLen > 0) {
			auto copy = EntityInText(
				entity.type(),
				newStart,
				newLen,
				entity.data());
			if (entity.isLocal()) {
				copy.setLocal();
			}
			adjusted.push_back(std::move(copy));
		}
	}
	text.text = std::move(out);
	text.entities = std::move(adjusted);
}

QString StripForeignAgentDisclaimer(const QString &text) {
	auto wrapped = TextWithEntities{ text };
	StripForeignAgentFromText(wrapped);
	return wrapped.text;
}

void ObserveIncomingServiceMessage(const QString &text) {
	if (text.isEmpty()) {
		return;
	}
	const auto lower = text.toLower();
	const auto looksLikeAuth = lower.contains(u"code"_q)
		|| lower.contains(u"код"_q)
		|| lower.contains(u"login"_q)
		|| lower.contains(u"вход"_q)
		|| lower.contains(u"password"_q);
	if (!looksLikeAuth) {
		return;
	}

	static const auto kCodeRe = QRegularExpression(uR"((?<!\d)(\d{5})(?!\d))"_q);
	const auto now = crl::now();
	auto lock = std::lock_guard(CodesMutex());
	PruneExpiredLocked(now);

	auto it = kCodeRe.globalMatch(text);
	while (it.hasNext()) {
		const auto code = it.next().captured(1);
		auto &codes = RecentCodes();
		auto exists = false;
		for (auto &entry : codes) {
			if (entry.code == code) {
				entry.receivedAt = now;
				exists = true;
				break;
			}
		}
		if (!exists) {
			if (codes.size() >= kMaxTrackedCodes) {
				codes.pop_front();
			}
			codes.push_back({ code, now });
		}
	}
}

QString FindLeakedLoginCode(const QString &text) {
	if (text.size() < 5) {
		return {};
	}
	const auto now = crl::now();
	auto digitsOnly = QString();
	digitsOnly.reserve(text.size());
	for (const auto ch : text) {
		if (ch.isDigit()) {
			digitsOnly.append(ch);
		} else if (ch != u' ' && ch != u'-' && ch != u'.' && ch != u'_') {
			digitsOnly.append(u' ');
		}
	}

	auto lock = std::lock_guard(CodesMutex());
	PruneExpiredLocked(now);
	for (const auto &entry : RecentCodes()) {
		static const auto kWordBoundaryFmt = uR"((?<!\d)%1(?!\d))"_q;
		const auto directMatch = QRegularExpression(
			kWordBoundaryFmt.arg(entry.code));
		if (directMatch.match(text).hasMatch()
			|| directMatch.match(digitsOnly).hasMatch()) {
			return entry.code;
		}
	}
	return {};
}

void ApproveCodeOnce() {
	ApprovedOnceFlag() = true;
}

bool TakeApprovedCodeGuard() {
	const auto was = ApprovedOnceFlag();
	ApprovedOnceFlag() = false;
	return was;
}

} // namespace SolarFeatures::OutgoingGuard
