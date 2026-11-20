// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/features/translator/implementations/extra.h"

#include <memory>

#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtCore/QUuid>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace Solar::Translator {
namespace {

constexpr auto kDeepLxEndpoint = "https://api.deeplx.org/translate";
constexpr auto kTransmartEndpoint = "https://transmart.qq.com/api/imt";
constexpr auto kTimeoutMs = 20000;

[[nodiscard]] QString NormalizeLangCode(
		TranslationProvider kind,
		const QString &toLang) {
	const auto trimmed = toLang.trimmed();
	if (kind == TranslationProvider::DeepLX) {
		if (trimmed.compare(u"zh-cn"_q, Qt::CaseInsensitive) == 0
			|| trimmed.compare(u"zh-hans"_q, Qt::CaseInsensitive) == 0) {
			return u"ZH"_q;
		}
		return trimmed.toUpper();
	}
	return trimmed.toLower();
}

[[nodiscard]] QJsonObject BuildTransmartPayload(
		const QString &text,
		const QString &toLang) {
	// Adapted from SereinGram serein/services/translation_protocol.cpp TransmartBody.
	const auto client = u"browser-chrome-120.0.0-Mac_OS-"_q
		+ QUuid::createUuid().toString(QUuid::WithoutBraces)
		+ u'-'
		+ QString::number(QDateTime::currentMSecsSinceEpoch());
	return QJsonObject{
		{ u"header"_q, QJsonObject{
			{ u"fn"_q, u"auto_translation"_q },
			{ u"client_key"_q, client },
		} },
		{ u"type"_q, u"plain"_q },
		{ u"model_category"_q, u"normal"_q },
		{ u"source"_q, QJsonObject{
			{ u"lang"_q, u"auto"_q },
			{ u"text_list"_q, QJsonArray{ text } },
		} },
		{ u"target"_q, QJsonObject{ { u"lang"_q, toLang } } },
	};
}

[[nodiscard]] QString ParseExtraResponse(
		TranslationProvider kind,
		const QByteArray &body) {
	QJsonParseError err{};
	const auto doc = QJsonDocument::fromJson(body, &err);
	if (err.error != QJsonParseError::NoError || !doc.isObject()) {
		return {};
	}
	const auto root = doc.object();
	switch (kind) {
	case TranslationProvider::DeepLX: {
		if (root.value(u"code"_q).toInt() != 200) {
			return {};
		}
		return root.value(u"data"_q).toString().trimmed();
	}
	case TranslationProvider::TranSmart: {
		const auto header = root.value(u"header"_q).toObject();
		if (header.value(u"ret_code"_q).toString() != u"succ"_q) {
			return {};
		}
		const auto items = root.value(u"auto_translation"_q).toArray();
		return items.isEmpty() ? QString() : items.first().toString().trimmed();
	}
	case TranslationProvider::SolarBrainLLM: {
		if (root.contains(u"message"_q) && root.value(u"message"_q).isObject()) {
			const auto content = root.value(u"message"_q).toObject()
				.value(u"content"_q).toString().trimmed();
			if (!content.isEmpty()) {
				return content;
			}
		}
		if (root.contains(u"response"_q)) {
			const auto content = root.value(u"response"_q).toString().trimmed();
			if (!content.isEmpty()) {
				return content;
			}
		}
		const auto choices = root.value(u"choices"_q).toArray();
		if (!choices.isEmpty() && choices.first().isObject()) {
			return choices.first().toObject()
				.value(u"message"_q).toObject()
				.value(u"content"_q).toString().trimmed();
		}
		return {};
	}
	default:
		return {};
	}
}

} // namespace

ExtraTranslator &ExtraTranslator::deeplx() {
	static ExtraTranslator inst(TranslationProvider::DeepLX);
	return inst;
}

ExtraTranslator &ExtraTranslator::transmart() {
	static ExtraTranslator inst(TranslationProvider::TranSmart);
	return inst;
}

ExtraTranslator &ExtraTranslator::solarBrain() {
	static ExtraTranslator inst(TranslationProvider::SolarBrainLLM);
	return inst;
}

ExtraTranslator::ExtraTranslator(TranslationProvider kind, QObject *parent)
	: MultiThreadTranslator(parent)
	, _kind(kind) {
}

QPointer<QNetworkReply> ExtraTranslator::startSingleTranslation(
		const MultiThreadArgs &args) {
	const auto &text = args.parsedData.text;
	const auto &toLang = args.parsedData.toLang;
	const auto onSuccess = args.onSuccess;
	const auto onFail = args.onFail;

	if (text.empty() || toLang.trimmed().isEmpty()) {
		if (onFail) onFail();
		return nullptr;
	}

	QUrl endpoint;
	QJsonObject payload;
	const auto normalizedTo = NormalizeLangCode(_kind, toLang);

	switch (_kind) {
	case TranslationProvider::DeepLX: {
		endpoint = QUrl(QString::fromLatin1(kDeepLxEndpoint));
		const auto from = args.parsedData.fromLang.trimmed().isEmpty()
			? u"auto"_q
			: args.parsedData.fromLang.trimmed().toUpper();
		payload = QJsonObject{
			{ u"text"_q, text.text },
			{ u"source_lang"_q, from },
			{ u"target_lang"_q, normalizedTo },
		};
		break;
	}
	case TranslationProvider::TranSmart: {
		endpoint = QUrl(QString::fromLatin1(kTransmartEndpoint));
		payload = BuildTransmartPayload(text.text, normalizedTo);
		break;
	}
	case TranslationProvider::SolarBrainLLM: {
		const auto &cfg = SolarSettings::getInstance();
		endpoint = QUrl(cfg.solarBrainEndpoint().trimmed());
		if (!endpoint.isValid()
			|| endpoint.host().isEmpty()
			|| (endpoint.scheme() != u"http"_q && endpoint.scheme() != u"https"_q)) {
			if (onFail) onFail();
			return nullptr;
		}
		const auto systemPrompt = u"You are a concise translator. Translate the user text into language \"%1\". Return ONLY the translation without quotes, notes, or markdown wrappers."_q
			.arg(normalizedTo);
		QJsonArray messages;
		messages.append(QJsonObject{
			{ u"role"_q, u"system"_q },
			{ u"content"_q, systemPrompt },
		});
		messages.append(QJsonObject{
			{ u"role"_q, u"user"_q },
			{ u"content"_q, text.text },
		});
		payload = QJsonObject{
			{ u"model"_q, cfg.solarBrainModel() },
			{ u"messages"_q, messages },
			{ u"stream"_q, false },
		};
		break;
	}
	default:
		if (onFail) onFail();
		return nullptr;
	}

	QNetworkRequest req(endpoint);
	req.setHeader(QNetworkRequest::UserAgentHeader, randomDesktopUserAgent());
	req.setHeader(QNetworkRequest::ContentTypeHeader, u"application/json"_q);
	req.setRawHeader("Accept", "application/json");

	const auto body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
	QPointer<QNetworkReply> reply = _nam.post(req, body);

	auto timer = new QTimer(reply);
	timer->setSingleShot(true);
	timer->setInterval(kTimeoutMs);
	QObject::connect(timer, &QTimer::timeout, reply, [reply] {
		if (reply && reply->isRunning()) {
			reply->abort();
		}
	});
	timer->start();

	const auto kind = _kind;
	QObject::connect(
		reply,
		&QNetworkReply::finished,
		reply,
		[reply, kind, onSuccess, onFail, timer] {
			if (!reply) return;
			timer->stop();
			const auto guard = std::unique_ptr<QNetworkReply, void(*)(QNetworkReply *)>(
				reply,
				[](QNetworkReply *r) { r->deleteLater(); });
			if (reply->error() != QNetworkReply::NoError) {
				if (onFail) onFail();
				return;
			}
			const auto translated = ParseExtraResponse(kind, reply->readAll());
			if (translated.isEmpty()) {
				if (onFail) onFail();
				return;
			}
			if (onSuccess) {
				onSuccess(TextWithEntities{ translated });
			}
		});

	return reply;
}

} // namespace Solar::Translator
