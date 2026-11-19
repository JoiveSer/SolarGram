// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/features/ai/solar_brain.h"

#include "lang_auto.h"
#include "solar/solar_settings.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"
#include "styles/style_widgets.h"
#include "ui/layers/generic_box.h"
#include "ui/text/text_utilities.h"
#include "ui/toast/toast.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace SolarFeatures::AI {
namespace {

constexpr auto kRequestTimeoutMs = 45 * 1000;

[[nodiscard]] QString SystemPromptFor(PromptPreset preset) {
	switch (preset) {
	case PromptPreset::Summarize:
		return u"You are SolarBrain, a concise assistant. Summarize the text into crisp bullet points in the original language. Do not invent facts."_q;
	case PromptPreset::ReplyDrafts:
		return u"You are SolarBrain. Generate 3 concise, natural reply drafts (short, neutral, detailed) in the language of the message."_q;
	case PromptPreset::Proofread:
		return u"You are SolarBrain. Rewrite the message clearly, fix grammar/typos, and remove harsh or caps-heavy tone while preserving meaning."_q;
	case PromptPreset::Explain:
		return u"You are SolarBrain. Explain the message clearly, translate key technical or foreign phrases if any, and list any action items."_q;
	}
	return {};
}

[[nodiscard]] bool IsLoopbackEndpoint(const QUrl &url) {
	const auto host = url.host().toLower();
	return host == u"127.0.0.1"_q
		|| host == u"localhost"_q
		|| host == u"::1"_q;
}

[[nodiscard]] QString ExtractResponseContent(const QByteArray &raw) {
	QJsonParseError err{};
	const auto doc = QJsonDocument::fromJson(raw, &err);
	if (err.error != QJsonParseError::NoError || !doc.isObject()) {
		return {};
	}
	const auto root = doc.object();
	// Ollama /api/chat format
	if (root.contains(u"message"_q) && root.value(u"message"_q).isObject()) {
		const auto content = root.value(u"message"_q).toObject().value(u"content"_q).toString().trimmed();
		if (!content.isEmpty()) {
			return content;
		}
	}
	// Ollama /api/generate format
	if (root.contains(u"response"_q)) {
		const auto content = root.value(u"response"_q).toString().trimmed();
		if (!content.isEmpty()) {
			return content;
		}
	}
	// OpenAI-compatible /v1/chat/completions format
	if (root.contains(u"choices"_q) && root.value(u"choices"_q).isArray()) {
		const auto choices = root.value(u"choices"_q).toArray();
		if (!choices.isEmpty() && choices.first().isObject()) {
			const auto first = choices.first().toObject();
			const auto content = first.value(u"message"_q).toObject().value(u"content"_q).toString().trimmed();
			if (!content.isEmpty()) {
				return content;
			}
		}
	}
	return {};
}

} // namespace

void ShowSolarBrainBox(
		not_null<Window::SessionController*> controller,
		const QString &sourceText,
		Fn<void(QString)> onInsert) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::solar_BrainTitle());
		box->setWidth(st::boxWideWidth);
		box->addTopButton(st::boxTitleClose, [=] { box->closeBox(); });

		const auto &settings = SolarSettings::getInstance();
		const auto endpointUrl = QUrl(settings.solarBrainEndpoint().trimmed());
		const auto validScheme = endpointUrl.isValid()
			&& (endpointUrl.scheme() == u"http"_q || endpointUrl.scheme() == u"https"_q)
			&& !endpointUrl.host().isEmpty();

		const auto layout = box->verticalLayout();
		const auto statusLabel = layout->add(
			object_ptr<Ui::FlatLabel>(
				layout,
				rpl::single(validScheme
					? (IsLoopbackEndpoint(endpointUrl)
						? tr::solar_BrainLocalReady(tr::now, lt_model, settings.solarBrainModel())
						: tr::solar_BrainRemoteWarning(tr::now, lt_host, endpointUrl.host()))
					: tr::solar_BrainInvalidEndpoint(tr::now)),
				st::boxDividerLabel),
			st::boxRowPadding);

		Ui::AddSkip(layout);

		const auto promptInput = layout->add(
			object_ptr<Ui::InputField>(
				layout,
				st::defaultInputField,
				Ui::InputField::Mode::MultiLine,
				tr::solar_BrainInputPlaceholder(),
				sourceText.left(4000)),
			st::boxRowPadding);
		promptInput->setMinHeight(72);
		promptInput->setMaxHeight(140);

		Ui::AddSkip(layout);

		const auto outputField = layout->add(
			object_ptr<Ui::InputField>(
				layout,
				st::defaultInputField,
				Ui::InputField::Mode::MultiLine,
				tr::solar_BrainOutputPlaceholder(),
				QString()),
			st::boxRowPadding);
		outputField->setMinHeight(96);
		outputField->setMaxHeight(220);

		struct BrainState {
			std::unique_ptr<QNetworkAccessManager> network;
			QNetworkReply *reply = nullptr;
			PromptPreset currentPreset = PromptPreset::Summarize;
		};
		const auto state = box->lifetime().make_state<BrainState>();
		state->network = std::make_unique<QNetworkAccessManager>();

		const auto runPreset = [=](PromptPreset preset) {
			state->currentPreset = preset;
			const auto &cfg = SolarSettings::getInstance();
			const auto url = QUrl(cfg.solarBrainEndpoint().trimmed());
			if (!url.isValid()
				|| url.host().isEmpty()
				|| (url.scheme() != u"http"_q && url.scheme() != u"https"_q)) {
				statusLabel->setText(tr::solar_BrainInvalidEndpoint(tr::now));
				return;
			}
			const auto text = promptInput->getLastText().trimmed();
			if (text.isEmpty()) {
				promptInput->showError();
				return;
			}
			if (state->reply) {
				state->reply->abort();
				state->reply->deleteLater();
				state->reply = nullptr;
			}

			statusLabel->setText(tr::solar_BrainThinking(tr::now, lt_model, cfg.solarBrainModel()));

			QJsonArray messages;
			messages.append(QJsonObject{
				{ u"role"_q, u"system"_q },
				{ u"content"_q, SystemPromptFor(preset) },
			});
			messages.append(QJsonObject{
				{ u"role"_q, u"user"_q },
				{ u"content"_q, text },
			});
			const auto payload = QJsonObject{
				{ u"model"_q, cfg.solarBrainModel() },
				{ u"messages"_q, messages },
				{ u"stream"_q, false },
			};

			QNetworkRequest req(url);
			req.setHeader(QNetworkRequest::ContentTypeHeader, u"application/json"_q);
			req.setTransferTimeout(kRequestTimeoutMs);

			const auto reply = state->network->post(
				req,
				QJsonDocument(payload).toJson(QJsonDocument::Compact));
			state->reply = reply;

			QObject::connect(reply, &QNetworkReply::finished, box, [=] {
				if (state->reply == reply) {
					state->reply = nullptr;
				}
				const auto raw = reply->readAll();
				const auto err = reply->error();
				const auto errStr = reply->errorString();
				reply->deleteLater();

				if (err != QNetworkReply::NoError) {
					statusLabel->setText(tr::solar_BrainRequestError(tr::now, lt_error, errStr));
					return;
				}
				const auto answer = ExtractResponseContent(raw);
				if (answer.isEmpty()) {
					statusLabel->setText(tr::solar_BrainEmptyResponse(tr::now));
					return;
				}
				outputField->setText(answer);
				statusLabel->setText(tr::solar_BrainDone(tr::now, lt_model, SolarSettings::getInstance().solarBrainModel()));
			});
		};

		Ui::AddSkip(layout);
		layout->add(
			object_ptr<Ui::SettingsButton>(
				layout,
				tr::solar_BrainPresetSummarize(),
				st::settingsButtonNoIcon)
		)->setClickedCallback([=] { runPreset(PromptPreset::Summarize); });

		layout->add(
			object_ptr<Ui::SettingsButton>(
				layout,
				tr::solar_BrainPresetReplies(),
				st::settingsButtonNoIcon)
		)->setClickedCallback([=] { runPreset(PromptPreset::ReplyDrafts); });

		layout->add(
			object_ptr<Ui::SettingsButton>(
				layout,
				tr::solar_BrainPresetProofread(),
				st::settingsButtonNoIcon)
		)->setClickedCallback([=] { runPreset(PromptPreset::Proofread); });

		layout->add(
			object_ptr<Ui::SettingsButton>(
				layout,
				tr::solar_BrainPresetExplain(),
				st::settingsButtonNoIcon)
		)->setClickedCallback([=] { runPreset(PromptPreset::Explain); });

		box->addLeftButton(tr::lng_settings_save(), [=] {
			const auto result = outputField->getLastText().trimmed();
			if (result.isEmpty()) {
				return;
			}
			QGuiApplication::clipboard()->setText(result);
			if (onInsert) {
				onInsert(result);
			}
			controller->showToast(tr::lng_text_copied(tr::now));
			box->closeBox();
		});
		box->addButton(tr::lng_close(), [=] {
			box->closeBox();
		});
	}));
}

void ShowSolarBrainConfigBox(not_null<Window::SessionController*> controller) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::solar_BrainConfigTitle());
		box->setWidth(st::boxWidth);
		const auto &settings = SolarSettings::getInstance();
		const auto layout = box->verticalLayout();

		layout->add(
			object_ptr<Ui::FlatLabel>(
				layout,
				tr::solar_BrainConfigDescription(),
				st::boxDividerLabel),
			st::boxRowPadding);
		Ui::AddSkip(layout);

		const auto endpointField = layout->add(
			object_ptr<Ui::InputField>(
				layout,
				st::defaultInputField,
				rpl::single(u"Endpoint (e.g. http://127.0.0.1:11434/api/chat)"_q),
				settings.solarBrainEndpoint()),
			st::boxRowPadding);

		Ui::AddSkip(layout);

		const auto modelField = layout->add(
			object_ptr<Ui::InputField>(
				layout,
				st::defaultInputField,
				rpl::single(u"Model (e.g. qwen2.5:7b)"_q),
				settings.solarBrainModel()),
			st::boxRowPadding);

		box->addLeftButton(tr::solar_BoxActionReset(), [=] {
			endpointField->setText(u"http://127.0.0.1:11434/api/chat"_q);
			modelField->setText(u"qwen2.5:7b"_q);
		});
		box->addButton(tr::lng_settings_save(), [=] {
			const auto ep = endpointField->getLastText().trimmed();
			const auto model = modelField->getLastText().trimmed();
			const auto parsed = QUrl(ep);
			if (!parsed.isValid()
				|| parsed.host().isEmpty()
				|| (parsed.scheme() != u"http"_q && parsed.scheme() != u"https"_q)) {
				endpointField->showError();
				return;
			}
			if (model.isEmpty()) {
				modelField->showError();
				return;
			}
			auto &s = SolarSettings::getInstance();
			s.setSolarBrainEndpoint(ep);
			s.setSolarBrainModel(model);
			controller->showToast(tr::lng_box_done(tr::now));
			box->closeBox();
		});
		box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
	}));
}

} // namespace SolarFeatures::AI
