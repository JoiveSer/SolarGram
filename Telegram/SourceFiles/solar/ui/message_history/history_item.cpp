// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/ui/message_history/history_item.h"

#include "history/history_item.h"
#include "api/api_text_entities.h"
#include "solar/data/entities.h"
#include "solar/data/messages_storage.h"
#include "solar/ui/message_history/history_inner.h"
#include "solar/utils/solar_mapper.h"
#include "base/random.h"
#include "base/unixtime.h"
#include "core/application.h"
#include "core/click_handler_types.h"
#include "core/file_location.h"
#include "core/version.h"
#include "data/data_channel.h"
#include "data/data_document.h"
#include "data/data_document_media.h"
#include "data/data_file_origin.h"
#include "data/data_forum_topic.h"
#include "data/data_photo.h"
#include "data/data_photo_media.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "history/history.h"
#include "history/view/history_view_element.h"
#include "storage/serialize_document.h"
#include "ui/basic_click_handlers.h"
#include "ui/image/image.h"
#include "ui/image/image_location.h"
#include "ui/text/text_utilities.h"

#include <QtCore/QDataStream>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtGui/QImage>

namespace MessageHistory {

OwnedItem::OwnedItem(std::nullptr_t) {
}

OwnedItem::OwnedItem(
	not_null<HistoryView::ElementDelegate*> delegate,
	not_null<HistoryItem*> data)
	: _data(data), _view(_data->createView(delegate)) {
}

OwnedItem::OwnedItem(OwnedItem &&other)
	: _data(base::take(other._data)), _view(base::take(other._view)) {
}

OwnedItem &OwnedItem::operator=(OwnedItem &&other) {
	_data = base::take(other._data);
	_view = base::take(other._view);
	return *this;
}

OwnedItem::~OwnedItem() {
	clearView();
	if (_data) {
		_data->destroy();
	}
}

void OwnedItem::refreshView(
	not_null<HistoryView::ElementDelegate*> delegate) {
	_view = _data->createView(delegate);
}

void OwnedItem::clearView() {
	_view = nullptr;
}

void GenerateItems(
	not_null<HistoryView::ElementDelegate*> delegate,
	not_null<History*> history,
	SolarMessageBase message,
	Fn<void(OwnedItem item, TimeId sentDate, MsgId)> callback) {
	PeerData *from = history->owner().userLoaded(message.fromId);
	if (!from) {
		from = history->owner().channelLoaded(message.fromId);
	}
	if (!from) {
		from = reinterpret_cast<PeerData*>(history->owner().chatLoaded(message.fromId));
	}
	const auto date = message.entityCreateDate;
	const auto addPart = [&](
		not_null<HistoryItem*> item,
		TimeId sentDate = 0,
		MsgId realId = MsgId())
	{
		return callback(OwnedItem(delegate, item), sentDate, realId);
	};

	const auto makeFields = [&]
	{
		base::flags<MessageFlag> flags = MessageFlag::AdminLogEntry;
		if (from) {
			flags |= MessageFlag::HasFromId;
		} else {
			flags |= MessageFlag::HasPostAuthor;
		}
		if (!message.postAuthor.empty()) {
			flags |= MessageFlag::HasPostAuthor;
		}

		return HistoryItemCommonFields{
			.id = history->nextNonHistoryEntryId(),
			.flags = flags,
			.from = from ? from->id : 0,
			.date = date,
			.postAuthor = !message.postAuthor.empty()
				? QString::fromStdString(message.postAuthor)
				: from
					? QString()
					: QString("unknown user: %1").arg(message.fromId),
		};
	};

	const auto makeSimpleTextMessage = [&](TextWithEntities &&text)
	{
		return history->makeMessage(
			makeFields(),
			std::move(text),
			MTP_messageMediaEmpty());
	};

	const auto addSimpleTextMessage = [&](TextWithEntities &&text)
	{
		addPart(makeSimpleTextMessage(std::move(text)));
	};

	const auto loadAttachmentBytes = [&](const std::string &mediaPath) -> QByteArray {
		if (mediaPath.empty()) {
			return QByteArray();
		}
		if (mediaPath.rfind("ram://", 0) == 0) {
			return SolarMessages::getSessionMediaBytes(mediaPath);
		}
		const auto path = QString::fromStdString(mediaPath);
		if (!QFile::exists(path)) {
			return QByteArray();
		}
		QFile f(path);
		if (!f.open(QIODevice::ReadOnly)) {
			return QByteArray();
		}
		return f.readAll();
	};

	const auto text = QString::fromStdString(message.text);
	auto textAndEntities = Ui::Text::WithEntities(text);
	const auto entities = SolarMapper::deserializeTextWithEntities(message.textEntities);
	textAndEntities.entities = Api::EntitiesFromMTP(&history->session(), entities.v);

	if (message.documentType == 1) {
		const auto bytes = loadAttachmentBytes(message.mediaPath);
		QImage img;
		if (!bytes.isEmpty()) {
			img.loadFromData(bytes);
		}
		if (!img.isNull()) {
			const auto inlineThumb = message.thumbsSerialized.empty()
				? QByteArray()
				: QByteArray(message.thumbsSerialized.data(), int(message.thumbsSerialized.size()));
			const auto memLoc = ImageLocation(
				DownloadLocation{ InMemoryLocation{ bytes } },
				img.width(),
				img.height());
			ImageWithLocation large{
				.location = memLoc,
				.bytes = bytes,
				.preloaded = img,
				.bytesCount = int(bytes.size()),
			};
			ImageWithLocation emptyLoc;
			const auto localPhotoId = base::RandomValue<PhotoId>() | (1ULL << 62);
			const auto photo = history->owner().photo(
				localPhotoId,
				0,
				QByteArray(),
				date,
				0,
				false,
				inlineThumb,
				emptyLoc,
				emptyLoc,
				large,
				emptyLoc,
				emptyLoc,
				0);
			const auto view = photo->createMediaView();
			view->set(Data::PhotoSize::Large, Data::PhotoSize::Large, img, bytes);
			history->owner().keepAlive(view);
			addPart(history->makeMessage(makeFields(), photo, textAndEntities));
			return;
		}
		if (textAndEntities.text.isEmpty()) {
			textAndEntities.text = u"[Photo]"_q;
		}
	} else if ((message.documentType == 2 || message.documentType == 3)
		&& !message.documentSerialized.empty()) {
		const auto serialized = QByteArray(
			message.documentSerialized.data(),
			int(message.documentSerialized.size()));
		QDataStream stream(serialized);
		quint8 magic = 0;
		quint8 version = 0;
		bool hasStickerInfo = false;
		stream >> magic >> version >> hasStickerInfo;
		DocumentData *doc = nullptr;
		if (stream.status() == QDataStream::Ok && magic == 0x53 && version == 1) {
			if (hasStickerInfo) {
				quint64 setId = 0;
				quint64 accessHash = 0;
				QString shortName;
				stream >> setId >> accessHash >> shortName;
				if (stream.status() == QDataStream::Ok) {
					doc = Serialize::Document::readStickerFromStream(
						&history->session(),
						AppVersion,
						stream,
						Serialize::Document::StickerSetInfo(setId, accessHash, shortName));
				}
			} else {
				doc = Serialize::Document::readFromStream(
					&history->session(),
					AppVersion,
					stream);
			}
		}
		if (doc) {
			if (!message.documentAttributesSerialized.empty()) {
				const auto attrs = SolarMapper::deserializeDocumentAttributes(
					message.documentAttributesSerialized);
				if (!attrs.v.isEmpty()) {
					doc->setattributes(attrs.v);
				}
			}
			if (message.mediaPath.rfind("ram://", 0) == 0) {
				const auto bytes = SolarMessages::getSessionMediaBytes(message.mediaPath);
				if (!bytes.isEmpty()) {
					const auto view = doc->createMediaView();
					view->setBytes(bytes);
					history->owner().keepAlive(view);
				}
			} else if (!message.mediaPath.empty()) {
				const auto path = QString::fromStdString(message.mediaPath);
				if (QFile::exists(path)) {
					doc->setLocation(Core::FileLocation(path));
					QFile f(path);
					constexpr qint64 kPreloadBytesLimit = 16 * 1024 * 1024;
					if (f.size() > 0 && f.size() <= kPreloadBytesLimit && f.open(QIODevice::ReadOnly)) {
						const auto view = doc->createMediaView();
						view->setBytes(f.readAll());
						history->owner().keepAlive(view);
					}
				}
			}
			addPart(history->makeMessage(makeFields(), doc, textAndEntities));
			return;
		}
		if (textAndEntities.text.isEmpty()) {
			textAndEntities.text = (message.documentType == 2) ? u"[Sticker]"_q : u"[Media]"_q;
		}
	}

	addSimpleTextMessage(std::move(textAndEntities));
}

} // namespace MessageHistory
