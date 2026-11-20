// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "solar/solar_settings.h"
#include "solar/features/translator/implementations/base.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtNetwork/QNetworkAccessManager>

namespace Solar::Translator {

// Unified translator backend for DeepLX, TranSmart, and local SolarBrain LLM endpoints,
// reusing SereinGram's DeepLX / TranSmart wire formats and SolarBrain's Ollama/OpenAI APIs.
class ExtraTranslator final : public MultiThreadTranslator {
	Q_OBJECT

public:
	static ExtraTranslator &deeplx();
	static ExtraTranslator &transmart();
	static ExtraTranslator &solarBrain();

	[[nodiscard]] QSet<QString> supportedLanguages() const override { return {}; }

	[[nodiscard]] QPointer<QNetworkReply> startSingleTranslation(
		const MultiThreadArgs &args
	) override;

private:
	ExtraTranslator(TranslationProvider kind, QObject *parent = nullptr);

	const TranslationProvider _kind;
	QNetworkAccessManager _nam;
};

} // namespace Solar::Translator
