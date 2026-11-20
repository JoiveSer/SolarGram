// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/data/messages_storage.h"

#include "solar/solar_settings.h"
#include "solar/data/solar_database.h"
#include "solar/utils/solar_mapper.h"
#include "solar/utils/telegram_helpers.h"
#include "base/random.h"
#include "base/unixtime.h"
#include "core/application.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_document.h"
#include "data/data_document_media.h"
#include "data/data_forum_topic.h"
#include "data/data_media_types.h"
#include "data/data_peer.h"
#include "data/data_photo.h"
#include "data/data_photo_media.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "data/stickers/data_stickers_set.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/history_item_components.h"
#include "main/main_session.h"
#include "storage/serialize_document.h"
#include "ui/image/image.h"

#include <QtCore/QBuffer>
#include <QtCore/QDataStream>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QIODevice>

#include <algorithm>
#include <deque>
#include <mutex>
#include <unordered_map>

namespace SolarMessages {
namespace {

constexpr qint64 kMaxSingleAttachmentBytes = 256LL * 1024LL * 1024LL;

struct SessionVaultState {
	std::mutex mutex;
	std::vector<DeletedMessage> messages;
	std::deque<std::string> mediaOrder;
	std::unordered_map<std::string, QByteArray> mediaBytes;
	qint64 totalMediaBytes = 0;
};

SessionVaultState &sessionVault() {
	static SessionVaultState state;
	return state;
}

bool containsCaseInsensitive(const std::string &haystack, const std::string &needle) {
	if (needle.empty()) {
		return true;
	}
	return QString::fromStdString(haystack).contains(
		QString::fromStdString(needle),
		Qt::CaseInsensitive);
}

QString sanitizeExtension(const QString &filename, const QString &mime) {
	auto ext = QFileInfo(filename).suffix().toLower();
	QString clean;
	clean.reserve(ext.size());
	for (const auto ch : ext) {
		if ((ch >= u'a' && ch <= u'z') || (ch >= u'0' && ch <= u'9')) {
			clean.append(ch);
		}
		if (clean.size() >= 8) {
			break;
		}
	}
	if (!clean.isEmpty()) {
		return clean;
	}
	const auto m = mime.toLower();
	if (m == u"image/jpeg"_q || m == u"image/jpg"_q) return u"jpg"_q;
	if (m == u"image/png"_q) return u"png"_q;
	if (m == u"image/webp"_q) return u"webp"_q;
	if (m == u"video/mp4"_q) return u"mp4"_q;
	if (m == u"video/webm"_q) return u"webm"_q;
	if (m == u"audio/ogg"_q || m == u"audio/opus"_q) return u"ogg"_q;
	if (m == u"audio/mpeg"_q) return u"mp3"_q;
	if (m == u"application/x-tgsticker"_q) return u"tgs"_q;
	return u"bin"_q;
}

bool shouldSaveMediaForPeer(not_null<PeerData*> peer, bool isSticker) {
	const auto &settings = SolarSettings::getInstance();
	if (isSticker && !settings.saveMediaStickers()) {
		return false;
	}
	if (const auto user = peer->asUser()) {
		return user->isBot()
			? settings.saveMediaBots()
			: settings.saveMediaPrivate();
	}
	if (peer->isChat() || (peer->isChannel() && peer->isMegagroup())) {
		return settings.saveMediaGroups();
	}
	if (peer->isChannel()) {
		return settings.saveMediaChannels();
	}
	return settings.saveMediaPrivate();
}

void pruneAttachmentsFolder(qint64 maxBytes) {
	const auto dirPath = getAttachmentsFolderPath();
	QDir dir(dirPath);
	if (!dir.exists()) {
		return;
	}
	auto entries = dir.entryInfoList(
		QDir::Files | QDir::NoSymLinks,
		QDir::Time | QDir::Reversed); // Oldest first.
	qint64 total = 0;
	for (const auto &info : entries) {
		total += info.size();
	}
	for (const auto &info : entries) {
		if (total <= maxBytes) {
			break;
		}
		const auto sz = info.size();
		if (QFile::remove(info.absoluteFilePath())) {
			total -= sz;
		}
	}
}

void storeSessionMediaBytes(const std::string &ramKey, const QByteArray &bytes, qint64 maxVaultBytes) {
	auto &vault = sessionVault();
	std::lock_guard lock(vault.mutex);
	if (bytes.size() > maxVaultBytes) {
		return;
	}
	if (const auto it = vault.mediaBytes.find(ramKey); it != vault.mediaBytes.end()) {
		vault.totalMediaBytes -= it->second.size();
		vault.mediaBytes.erase(it);
	}
	while (vault.totalMediaBytes + bytes.size() > maxVaultBytes && !vault.mediaOrder.empty()) {
		const auto oldKey = std::move(vault.mediaOrder.front());
		vault.mediaOrder.pop_front();
		if (const auto it = vault.mediaBytes.find(oldKey); it != vault.mediaBytes.end()) {
			vault.totalMediaBytes -= it->second.size();
			vault.mediaBytes.erase(it);
		}
	}
	vault.totalMediaBytes += bytes.size();
	vault.mediaOrder.push_back(ramKey);
	vault.mediaBytes.emplace(ramKey, bytes);
}

std::string persistAttachmentBytes(
		ID userId,
		ID dialogId,
		int messageId,
		const QString &ext,
		const QByteArray &bytes,
		bool persistentToDisk) {
	if (bytes.isEmpty() || bytes.size() > kMaxSingleAttachmentBytes) {
		return {};
	}
	const auto &settings = SolarSettings::getInstance();
	const auto maxBytes = std::max<qint64>(128LL, qint64(settings.attachmentsMaxSizeMb()))
		* 1024LL * 1024LL;

	const auto uniqueId = base::RandomValue<quint32>();
	if (!persistentToDisk) {
		const auto key = u"ram://%1_%2_%3_%4.%5"_q
			.arg(userId)
			.arg(dialogId)
			.arg(messageId)
			.arg(uniqueId)
			.arg(ext)
			.toStdString();
		storeSessionMediaBytes(key, bytes, maxBytes);
		return key;
	}

	const auto dirPath = getAttachmentsFolderPath();
	QDir().mkpath(dirPath);
	const auto fileName = u"%1_%2_%3_%4.%5"_q
		.arg(userId)
		.arg(dialogId)
		.arg(messageId)
		.arg(uniqueId)
		.arg(ext);
	const auto fullPath = dirPath + u'/' + fileName;
	QFile out(fullPath);
	if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return {};
	}
	if (out.write(bytes) != bytes.size()) {
		out.close();
		QFile::remove(fullPath);
		return {};
	}
	out.close();
	pruneAttachmentsFolder(maxBytes);
	return fullPath.toStdString();
}

QByteArray extractPhotoBytes(not_null<PhotoData*> photo) {
	const auto view = photo->createMediaView();
	constexpr Data::PhotoSize kOrder[] = {
		Data::PhotoSize::Large,
		Data::PhotoSize::Thumbnail,
		Data::PhotoSize::Small,
	};
	for (const auto sz : kOrder) {
		const auto raw = view->imageBytes(sz);
		if (!raw.isEmpty()) {
			return raw;
		}
	}
	for (const auto sz : kOrder) {
		if (const auto img = view->image(sz)) {
			const auto original = img->original();
			if (!original.isNull()) {
				QByteArray encoded;
				QBuffer buf(&encoded);
				buf.open(QIODevice::WriteOnly);
				if (original.save(&buf, "JPG", 92) && !encoded.isEmpty()) {
					return encoded;
				}
			}
		}
	}
	return QByteArray();
}

QByteArray extractDocumentBytes(not_null<DocumentData*> doc) {
	const auto path = doc->filepath(true);
	if (!path.isEmpty() && QFile::exists(path)) {
		QFileInfo info(path);
		if (info.size() > 0 && info.size() <= kMaxSingleAttachmentBytes) {
			QFile f(path);
			if (f.open(QIODevice::ReadOnly)) {
				return f.readAll();
			}
		}
	}
	if (const auto view = doc->createMediaView()) {
		const auto bytes = view->bytes();
		if (!bytes.isEmpty() && bytes.size() <= kMaxSingleAttachmentBytes) {
			return bytes;
		}
	}
	return QByteArray();
}

MTPVector<MTPDocumentAttribute> composeDocumentAttributes(not_null<DocumentData*> document) {
	QVector<MTPDocumentAttribute> attributes;
	if (!document->filename().isEmpty()) {
		attributes.push_back(MTP_documentAttributeFilename(
			MTP_string(document->filename())));
	}
	const auto dimensions = document->dimensions;
	const auto animated = (document->type == AnimatedDocument);
	if (dimensions.width() > 0 && dimensions.height() > 0) {
		if (document->hasDuration() && !document->hasMimeType(u"image/gif"_q)) {
			auto flags = MTPDdocumentAttributeVideo::Flags(0);
			using VideoFlag = MTPDdocumentAttributeVideo::Flag;
			if (document->isVideoMessage()) {
				flags |= VideoFlag::f_round_message;
			}
			if (document->supportsStreaming()) {
				flags |= VideoFlag::f_supports_streaming;
			}
			if (!animated && document->isSilentVideo()) {
				flags |= VideoFlag::f_nosound;
			}
			const auto video = document->video();
			const auto startTs = video ? video->startTs : crl::time(0);
			if (startTs > 0) {
				flags |= VideoFlag::f_video_start_ts;
			}
			attributes.push_back(MTP_documentAttributeVideo(
				MTP_flags(flags),
				MTP_double(document->duration() / 1000.),
				MTP_int(dimensions.width()),
				MTP_int(dimensions.height()),
				MTPint(),
				MTP_double(startTs / 1000.),
				MTPstring()));
		} else {
			attributes.push_back(MTP_documentAttributeImageSize(
				MTP_int(dimensions.width()),
				MTP_int(dimensions.height())));
		}
	}
	if (animated) {
		attributes.push_back(MTP_documentAttributeAnimated());
	} else if (document->type == StickerDocument && document->sticker()) {
		attributes.push_back(MTP_documentAttributeSticker(
			MTP_flags(0),
			MTP_string(document->sticker()->alt),
			Data::InputStickerSet(document->sticker()->set),
			MTPMaskCoords()));
	} else if (const auto song = document->song()) {
		const auto flags = MTPDdocumentAttributeAudio::Flag::f_title
			| MTPDdocumentAttributeAudio::Flag::f_performer;
		attributes.push_back(MTP_documentAttributeAudio(
			MTP_flags(flags),
			MTP_int(document->duration() / 1000),
			MTP_string(song->title),
			MTP_string(song->performer),
			MTPstring()));
	} else if (const auto voice = document->voice()) {
		const auto flags = MTPDdocumentAttributeAudio::Flag::f_voice
			| MTPDdocumentAttributeAudio::Flag::f_waveform;
		attributes.push_back(MTP_documentAttributeAudio(
			MTP_flags(flags),
			MTP_int(document->duration() / 1000),
			MTPstring(),
			MTPstring(),
			MTP_bytes(documentWaveformEncode5bit(voice->waveform))));
	}
	return MTP_vector<MTPDocumentAttribute>(attributes);
}

} // namespace

template<typename DerivedMessage>
std::vector<SolarMessageBase> convertToBase(const std::vector<DerivedMessage> &messages) {
	std::vector<SolarMessageBase> based;
	based.reserve(messages.size());
	for (const auto &msg : messages) {
		based.push_back(static_cast<SolarMessageBase>(msg));
	}
	return based;
}

void map(not_null<HistoryItem*> item, SolarMessageBase &message, bool persistentMedia) {
	const ID userId = item->history()->owner().session().userId().bare & PeerId::kChatTypeMask;

	message.userId = userId;
	message.dialogId = getDialogIdFromPeer(item->history()->peer);
	message.groupedId = item->groupId().raw();
	message.peerId = item->history()->peer->id.value & PeerId::kChatTypeMask;
	message.fromId = item->from() ? (item->from()->id.value & PeerId::kChatTypeMask) : 0;
	if (item->topic()) {
		message.topicId = item->topicRootId().bare;
	} else {
		message.topicId = 0;
	}
	message.messageId = item->id.bare;
	message.date = item->date();
	message.flags = SolarMapper::mapItemFlagsToMTPFlags(item);

	if (const auto edited = item->Get<HistoryMessageEdited>()) {
		message.editDate = edited->date;
	} else {
		message.editDate = base::unixtime::now();
	}

	message.views = item->viewsCount();
	message.fwdFlags = 0;
	message.fwdFromId = 0;
	message.fwdDate = 0;
	if (const auto msgsigned = item->Get<HistoryMessageSigned>()) {
		message.postAuthor = msgsigned->author.toStdString();
	}
	message.replyFlags = 0;
	message.replyMessageId = 0;
	message.replyPeerId = 0;
	message.replyTopId = 0;
	message.replyForumTopic = false;
	message.entityCreateDate = base::unixtime::now();

	auto serializedText = SolarMapper::serializeTextWithEntities(item);
	message.text = serializedText.first;
	message.textEntities = serializedText.second;

	message.mediaPath.clear();
	message.hqThumbPath.clear();
	message.documentType = 0;

	if (const auto media = item->media()) {
		const auto peer = item->history()->peer;
		if (const auto photo = media->photo()) {
			if (shouldSaveMediaForPeer(peer, false)) {
				message.documentType = 1;
				message.mimeType = "image/jpeg";
				const auto inlineThumb = photo->inlineThumbnailBytes();
				if (!inlineThumb.isEmpty()) {
					message.thumbsSerialized.assign(inlineThumb.begin(), inlineThumb.end());
				}
				QByteArray metaBuf;
				{
					QDataStream stream(&metaBuf, QIODevice::WriteOnly);
					stream << quint64(photo->id)
						<< qint32(photo->width())
						<< qint32(photo->height());
				}
				message.documentSerialized.assign(metaBuf.begin(), metaBuf.end());
				const auto bytes = extractPhotoBytes(photo);
				if (!bytes.isEmpty()) {
					message.mediaPath = persistAttachmentBytes(
						message.userId,
						message.dialogId,
						message.messageId,
						u"jpg"_q,
						bytes,
						persistentMedia);
				}
			}
		} else if (const auto doc = media->document()) {
			const bool isSticker = (doc->sticker() != nullptr);
			if (shouldSaveMediaForPeer(peer, isSticker)) {
				message.documentType = isSticker ? 2 : 3;
				message.mimeType = doc->mimeString().toStdString();
				const auto inlineThumb = doc->inlineThumbnailBytes();
				if (!inlineThumb.isEmpty()) {
					message.thumbsSerialized.assign(inlineThumb.begin(), inlineThumb.end());
				}
				QByteArray docBuf;
				{
					QDataStream stream(&docBuf, QIODevice::WriteOnly);
					const bool hasStickerInfo = isSticker && doc->sticker();
					stream << quint8(0x53) << quint8(1) << hasStickerInfo;
					if (hasStickerInfo) {
						const auto &s = doc->sticker()->set;
						stream << quint64(s.id) << quint64(s.accessHash) << s.shortName;
					}
					Serialize::Document::writeToStream(stream, doc);
				}
				message.documentSerialized.assign(docBuf.begin(), docBuf.end());
				message.documentAttributesSerialized = SolarMapper::serializeDocumentAttributes(
					composeDocumentAttributes(doc));

				const auto bytes = extractDocumentBytes(doc);
				if (!bytes.isEmpty()) {
					const auto ext = sanitizeExtension(doc->filename(), doc->mimeString());
					message.mediaPath = persistAttachmentBytes(
						message.userId,
						message.dialogId,
						message.messageId,
						ext,
						bytes,
						persistentMedia);
				}
			}
		}
	}
}

void addEditedMessage(not_null<HistoryItem *> item) {
	EditedMessage message;
	map(item, message, true);

	if (message.text.empty() && message.documentType == 0) {
		return;
	}

	SolarDatabase::addEditedMessage(message);
}

std::vector<SolarMessageBase> getEditedMessages(not_null<HistoryItem*> item, ID minId, ID maxId, int totalLimit) {
	const ID userId = item->history()->owner().session().userId().bare & PeerId::kChatTypeMask;
	const auto dialogId = getDialogIdFromPeer(item->history()->peer);
	const auto msgId = item->id.bare;

	return convertToBase(SolarDatabase::getEditedMessages(userId, dialogId, msgId, minId, maxId, totalLimit));
}

bool hasRevisions(not_null<HistoryItem*> item) {
	const ID userId = item->history()->owner().session().userId().bare & PeerId::kChatTypeMask;
	const auto dialogId = getDialogIdFromPeer(item->history()->peer);
	const auto msgId = item->id.bare;

	return SolarDatabase::hasRevisions(userId, dialogId, msgId);
}

void addDeletedMessage(not_null<HistoryItem*> item) {
	const auto mode = SolarSettings::getInstance().deletedSaveMode();
	if (mode == DeletedSaveMode::Off) {
		return;
	}
	const bool persistent = (mode == DeletedSaveMode::Persistent);

	DeletedMessage message;
	map(item, message, persistent);

	if (message.text.empty() && message.documentType == 0) {
		return;
	}

	if (persistent) {
		SolarDatabase::addDeletedMessage(message);
		return;
	}

	auto &vault = sessionVault();
	std::lock_guard lock(vault.mutex);
	const auto dup = ranges::any_of(vault.messages, [&](const DeletedMessage &existing) {
		return existing.userId == message.userId
			&& existing.dialogId == message.dialogId
			&& existing.messageId == message.messageId;
	});
	if (!dup) {
		message.fakeId = static_cast<ID>(vault.messages.size() + 1);
		vault.messages.push_back(std::move(message));
	}
}

std::vector<SolarMessageBase>
getDeletedMessages(not_null<PeerData*> peer, ID topicId, ID minId, ID maxId, int totalLimit, const QString &searchQuery) {
	const ID userId = peer->session().userId().bare & PeerId::kChatTypeMask;
	const ID dialogId = getDialogIdFromPeer(peer);
	if (SolarSettings::getInstance().deletedSaveMode() == DeletedSaveMode::SessionOnly) {
		const auto query = searchQuery.toStdString();
		auto &vault = sessionVault();
		std::lock_guard lock(vault.mutex);
		std::vector<SolarMessageBase> filtered;
		for (const auto &msg : vault.messages) {
			if (msg.userId == userId
				&& msg.dialogId == dialogId
				&& (topicId == 0 || msg.topicId == topicId)
				&& (minId == 0 || msg.messageId > minId)
				&& (maxId == 0 || msg.messageId < maxId)
				&& containsCaseInsensitive(msg.text, query)) {
				filtered.push_back(static_cast<SolarMessageBase>(msg));
			}
		}
		std::sort(filtered.begin(), filtered.end(), [](const SolarMessageBase &a, const SolarMessageBase &b) {
			return a.messageId > b.messageId;
		});
		if (totalLimit > 0 && static_cast<int>(filtered.size()) > totalLimit) {
			filtered.resize(totalLimit);
		}
		return filtered;
	}
	return convertToBase(
		SolarDatabase::getDeletedMessages(userId, dialogId, topicId, minId, maxId, totalLimit, searchQuery.toStdString()));
}

bool hasDeletedMessages(not_null<PeerData*> peer, ID topicId) {
	const ID userId = peer->session().userId().bare & PeerId::kChatTypeMask;
	const ID dialogId = getDialogIdFromPeer(peer);
	if (SolarSettings::getInstance().deletedSaveMode() == DeletedSaveMode::SessionOnly) {
		auto &vault = sessionVault();
		std::lock_guard lock(vault.mutex);
		return ranges::any_of(vault.messages, [&](const DeletedMessage &msg) {
			return msg.userId == userId
				&& msg.dialogId == dialogId
				&& (topicId == 0 || msg.topicId == topicId);
		});
	}
	return SolarDatabase::hasDeletedMessages(userId, dialogId, topicId);
}

void removeDeletedMessage(not_null<HistoryItem*> item) {
	const auto peer = item->history()->peer;
	const ID userId = peer->session().userId().bare & PeerId::kChatTypeMask;
	const ID dialogId = getDialogIdFromPeer(peer);
	{
		auto &vault = sessionVault();
		std::lock_guard lock(vault.mutex);
		vault.messages.erase(
			std::remove_if(vault.messages.begin(), vault.messages.end(), [&](const DeletedMessage &msg) {
				if (msg.userId == userId
					&& msg.dialogId == dialogId
					&& msg.messageId == item->id.bare) {
					if (!msg.mediaPath.empty()) {
						if (const auto it = vault.mediaBytes.find(msg.mediaPath); it != vault.mediaBytes.end()) {
							vault.totalMediaBytes -= it->second.size();
							vault.mediaBytes.erase(it);
						}
					}
					return true;
				}
				return false;
			}),
			vault.messages.end());
	}
	SolarDatabase::removeDeletedMessage(userId, dialogId, item->id.bare);
}

void clearDeletedMessages(not_null<PeerData*> peer, ID topicId) {
	const ID userId = peer->session().userId().bare & PeerId::kChatTypeMask;
	const ID dialogId = getDialogIdFromPeer(peer);
	{
		auto &vault = sessionVault();
		std::lock_guard lock(vault.mutex);
		vault.messages.erase(
			std::remove_if(vault.messages.begin(), vault.messages.end(), [&](const DeletedMessage &msg) {
				if (msg.userId == userId
					&& msg.dialogId == dialogId
					&& (topicId == 0 || msg.topicId == topicId)) {
					if (!msg.mediaPath.empty()) {
						if (const auto it = vault.mediaBytes.find(msg.mediaPath); it != vault.mediaBytes.end()) {
							vault.totalMediaBytes -= it->second.size();
							vault.mediaBytes.erase(it);
						}
					}
					return true;
				}
				return false;
			}),
			vault.messages.end());
	}
	SolarDatabase::clearDeletedMessages(userId, dialogId, topicId);
}

void clearAllDeletedMessages() {
	clearSessionVault();
	SolarDatabase::clearAllDeletedMessages();
	clearAttachmentsFolder();
}

void clearSessionVault() {
	auto &vault = sessionVault();
	std::lock_guard lock(vault.mutex);
	vault.messages.clear();
	vault.mediaOrder.clear();
	vault.mediaBytes.clear();
	vault.totalMediaBytes = 0;
}

QByteArray getSessionMediaBytes(const std::string &ramKey) {
	auto &vault = sessionVault();
	std::lock_guard lock(vault.mutex);
	const auto it = vault.mediaBytes.find(ramKey);
	return (it != vault.mediaBytes.end()) ? it->second : QByteArray();
}

QString getAttachmentsFolderPath() {
	return cWorkingDir() + u"tdata/SolarAttachments"_q;
}

void clearAttachmentsFolder() {
	QDir dir(getAttachmentsFolderPath());
	if (dir.exists()) {
		dir.removeRecursively();
	}
}

}
