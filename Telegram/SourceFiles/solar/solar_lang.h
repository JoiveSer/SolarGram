// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include <QtNetwork/QNetworkReply>
#include <QtXml/QDomDocument>

class SolarLanguage : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY(SolarLanguage)

public:
	static SolarLanguage *currentInstance();
	static void init();
	static SolarLanguage *instance;

	void fetchLanguage(const QString &id, const QString &baseId);
	void applyLanguageJson(QJsonDocument doc);

public Q_SLOTS:
	void fetchFinished();
	void fetchError(QNetworkReply::NetworkError e);

private:
	SolarLanguage();
	~SolarLanguage() override = default;

	void loadCachedLanguage();
	void saveCachedLanguage(const QByteArray &json, const QString &langId);
	[[nodiscard]] QString getCacheDir() const;
	[[nodiscard]] QString getCachePath(const QString &langId) const;

	QNetworkAccessManager networkManager;
	QNetworkReply *_chkReply = nullptr;
	bool needFallback = false;
	QString _currentLangId;
};
