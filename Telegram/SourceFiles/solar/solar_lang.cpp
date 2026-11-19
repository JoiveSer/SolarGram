// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/solar_lang.h"

#include "qjsondocument.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "lang/lang_instance.h"
#include "storage/localstorage.h"

#include <QDir>
#include <QFile>

// hard-coded languages
std::map<QString, QString> langMapping = {
	{"pt-br", "pt"},
	{"zh-hans-beta", "zh-hans"},
	{"zh-hant-beta", "zh-hant"},
	{"zh-hans-raw", "zh-hans"},
	{"zh-hant-raw", "zh-hant"},
};

constexpr auto postfixes = {
	"zero",
	"one",
	"two",
	"few",
	"many",
	"other"
};

SolarLanguage *SolarLanguage::instance = nullptr;

SolarLanguage::SolarLanguage() = default;

void SolarLanguage::init() {
	if (!instance) instance = new SolarLanguage;
	instance->loadCachedLanguage();
}

SolarLanguage *SolarLanguage::currentInstance() {
	return instance;
}

QString SolarLanguage::getCacheDir() const {
	return cWorkingDir() + u"tdata/solar/languages/"_q;
}

QString SolarLanguage::getCachePath(const QString &langId) const {
	return getCacheDir() + langId + u".json"_q;
}

void SolarLanguage::loadCachedLanguage() {
	const auto langPackId = Lang::GetInstance().id();
	const auto langPackBaseId = Lang::GetInstance().baseId();
	auto finalLangPackId = langMapping.contains(langPackId) ? langMapping[langPackId] : langPackId;

	if (finalLangPackId.isEmpty()) {
		finalLangPackId = langPackBaseId;
	}
	if (finalLangPackId.isEmpty()) {
		return;
	}

	const auto cachePath = getCachePath(finalLangPackId);
	QFile file(cachePath);
	if (!file.exists()) {
		const auto basePath = getCachePath(langPackBaseId);
		if (!QFile::exists(basePath)) {
			return;
		}
		file.setFileName(basePath);
	}

	if (file.open(QIODevice::ReadOnly)) {
		const auto data = file.readAll();
		file.close();

		QJsonParseError error{};
		const auto doc = QJsonDocument::fromJson(data, &error);
		if (error.error == QJsonParseError::NoError) {
			LOG(("Loading cached SolarGram language: %1").arg(finalLangPackId));
			applyLanguageJson(doc);
		}
	}
}

void SolarLanguage::saveCachedLanguage(const QByteArray &json, const QString &langId) {
	const auto cacheDir = getCacheDir();
	QDir().mkpath(cacheDir);

	const auto cachePath = getCachePath(langId);
	QFile file(cachePath);
	if (file.open(QIODevice::WriteOnly)) {
		file.write(json);
		file.close();
		LOG(("Cached SolarGram language: %1").arg(langId));
	}
}

void SolarLanguage::fetchLanguage(const QString &id, const QString &baseId) {
	auto finalLangPackId = langMapping.contains(id) ? langMapping[id] : id;
	_currentLangId = finalLangPackId.isEmpty() ? baseId : finalLangPackId;
	// SolarGram bundles localization keys locally in lang.strings and only reads local overrides from tdata/solar/languages/.
	loadCachedLanguage();
}

void SolarLanguage::fetchFinished() {
	if (!_chkReply) return;

	QString langPackBaseId = Lang::GetInstance().baseId();
	QString langPackId = Lang::GetInstance().id();
	auto statusCode = _chkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (statusCode == 404 && !langPackId.isEmpty() && !langPackBaseId.isEmpty() && !needFallback) {
		LOG(("SolarGram Language not found! Fallback to main language: %1...").arg(langPackBaseId));
		needFallback = true;
		_chkReply->disconnect();
		fetchLanguage("", langPackBaseId);
	} else {
		const auto result = _chkReply->readAll().trimmed();
		QJsonParseError error{};
		const auto doc = QJsonDocument::fromJson(result, &error);
		if (error.error == QJsonParseError::NoError) {
			saveCachedLanguage(result, _currentLangId);
			applyLanguageJson(doc);
		} else {
			LOG(("Incorrect language JSON File."));
		}

		_chkReply = nullptr;
	}
}

void SolarLanguage::fetchError(QNetworkReply::NetworkError e) {
	LOG(("Network error: %1").arg(e));

	if (e == QNetworkReply::NetworkError::ContentNotFoundError) {
		const auto baseId = Lang::GetInstance().baseId();
		const auto id = Lang::GetInstance().id();

		if (!id.isEmpty() && !baseId.isEmpty() && !needFallback) {
			LOG(("SolarGram Language not found! Fallback to main language: %1...").arg(baseId));
			needFallback = true;
			_chkReply->disconnect();
			fetchLanguage("", baseId);
		} else {
			LOG(("SolarGram Language not found!"));
			_chkReply = nullptr;
		}
	}
}

void SolarLanguage::applyLanguageJson(QJsonDocument doc) {
	const auto json = doc.object();
	for (const QString &brokenKey : json.keys()) {
		auto key = qsl("solar_") + brokenKey;
		auto val = json.value(brokenKey).toString().replace(qsl("&amp;"), qsl("&"));

		if (key.endsWith("_Android")) {
			continue;
		}

		for (const auto &postfix : postfixes) {
			if (key.endsWith(qsl("_") + postfix)) {
				key = key.replace(qsl("_") + postfix, qsl("#") + postfix);
				break;
			}
		}

		if (key.endsWith("_PC")) {
			key = key.replace("_PC", "");
		}

		if (val.contains(qsl("%1$d")) && !val.contains(qsl("%2$d"))) {
			val = val.replace(qsl("%1$d"), qsl("{count}"));
		} else if (val.contains(qsl("%1$d")) && val.contains(qsl("%2$d"))) {
			val = val.replace(qsl("%1$d"), qsl("{count1}")).replace(qsl("%2$d"), qsl("{count2}"));
		} else if (val.contains(qsl("%1$s")) && !val.contains(qsl("%2$s"))) {
			val = val.replace(qsl("%1$s"), qsl("{item}"));
		} else if (val.contains(qsl("%1$s")) && val.contains(qsl("%2$s"))) {
			val = val.replace(qsl("%1$s"), qsl("{item1}")).replace(qsl("%2$s"), qsl("{item2}"));
		}

		Lang::GetInstance().resetValue(key.toUtf8());
		Lang::GetInstance().applyValue(key.toUtf8(), val.toUtf8());
	}
	Lang::GetInstance().updatePluralRules();
}
