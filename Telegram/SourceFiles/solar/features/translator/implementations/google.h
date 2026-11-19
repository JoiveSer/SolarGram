// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "solar/features/translator/implementations/base.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QString>

namespace Solar::Translator {

class GoogleTranslator final : public MultiThreadTranslator
{
	Q_OBJECT

public:
	static GoogleTranslator &instance();

	// all languages
	[[nodiscard]] QSet<QString> supportedLanguages() const override { return {}; }

	[[nodiscard]] QPointer<QNetworkReply> startSingleTranslation(
		const MultiThreadArgs &args
	) override;

private:
	explicit GoogleTranslator(QObject *parent = nullptr);

	QNetworkAccessManager _nam;
};

}