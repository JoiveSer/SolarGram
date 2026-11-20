// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/data/solar_database.h"

#include "solar/data/entities.h"
#include "solar/libs/sqlite/sqlite_orm.h"
#include "base/unixtime.h"

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <mutex>
#include <unordered_map>

using namespace sqlite_orm;

std::mutex databaseMutex;

namespace {

struct LastSeenCacheKey {
	ID userId;
	ID peerId;
	bool operator==(const LastSeenCacheKey &other) const = default;
};

struct LastSeenCacheKeyHash {
	size_t operator()(const LastSeenCacheKey &k) const noexcept {
		return std::hash<ID>{}(k.userId) ^ (std::hash<ID>{}(k.peerId) << 1);
	}
};

std::unordered_map<LastSeenCacheKey, int, LastSeenCacheKeyHash> rememberedLastSeenCache;

} // namespace

auto storage = make_storage(
	"./tdata/solardata.db",
	make_table<SchemaVersion>(
		"SchemaVersion",
		make_column("id", &SchemaVersion::id, primary_key()),
		make_column("version", &SchemaVersion::version)
	),
	make_index("idx_deleted_message_userId_dialogId_topicId_messageId",
			   column<DeletedMessage>(&DeletedMessage::userId),
			   column<DeletedMessage>(&DeletedMessage::dialogId),
			   column<DeletedMessage>(&DeletedMessage::topicId),
			   column<DeletedMessage>(&DeletedMessage::messageId)),
	make_index("idx_edited_message_userId_dialogId_messageId",
			   column<EditedMessage>(&EditedMessage::userId),
			   column<EditedMessage>(&EditedMessage::dialogId),
			   column<EditedMessage>(&EditedMessage::messageId)),
	make_table<DeletedMessage>(
		"DeletedMessage",
		make_column("fakeId", &DeletedMessage::fakeId, primary_key().autoincrement()),
		make_column("userId", &DeletedMessage::userId),
		make_column("dialogId", &DeletedMessage::dialogId),
		make_column("groupedId", &DeletedMessage::groupedId),
		make_column("peerId", &DeletedMessage::peerId),
		make_column("fromId", &DeletedMessage::fromId),
		make_column("topicId", &DeletedMessage::topicId),
		make_column("messageId", &DeletedMessage::messageId),
		make_column("date", &DeletedMessage::date),
		make_column("flags", &DeletedMessage::flags),
		make_column("editDate", &DeletedMessage::editDate),
		make_column("views", &DeletedMessage::views),
		make_column("fwdFlags", &DeletedMessage::fwdFlags),
		make_column("fwdFromId", &DeletedMessage::fwdFromId),
		make_column("fwdName", &DeletedMessage::fwdName),
		make_column("fwdDate", &DeletedMessage::fwdDate),
		make_column("fwdPostAuthor", &DeletedMessage::fwdPostAuthor),
		make_column("replyFlags", &DeletedMessage::replyFlags),
		make_column("replyMessageId", &DeletedMessage::replyMessageId),
		make_column("replyPeerId", &DeletedMessage::replyPeerId),
		make_column("replyTopId", &DeletedMessage::replyTopId),
		make_column("replyForumTopic", &DeletedMessage::replyForumTopic),
		make_column("replySerialized", &DeletedMessage::replySerialized),
		make_column("entityCreateDate", &DeletedMessage::entityCreateDate),
		make_column("text", &DeletedMessage::text),
		make_column("textEntities", &DeletedMessage::textEntities),
		make_column("mediaPath", &DeletedMessage::mediaPath),
		make_column("hqThumbPath", &DeletedMessage::hqThumbPath),
		make_column("documentType", &DeletedMessage::documentType),
		make_column("documentSerialized", &DeletedMessage::documentSerialized),
		make_column("thumbsSerialized", &DeletedMessage::thumbsSerialized),
		make_column("documentAttributesSerialized", &DeletedMessage::documentAttributesSerialized),
		make_column("mimeType", &DeletedMessage::mimeType)
	),
	make_table<EditedMessage>(
		"EditedMessage",
		make_column("fakeId", &EditedMessage::fakeId, primary_key().autoincrement()),
		make_column("userId", &EditedMessage::userId),
		make_column("dialogId", &EditedMessage::dialogId),
		make_column("groupedId", &EditedMessage::groupedId),
		make_column("peerId", &EditedMessage::peerId),
		make_column("fromId", &EditedMessage::fromId),
		make_column("topicId", &EditedMessage::topicId),
		make_column("messageId", &EditedMessage::messageId),
		make_column("date", &EditedMessage::date),
		make_column("flags", &EditedMessage::flags),
		make_column("editDate", &EditedMessage::editDate),
		make_column("views", &EditedMessage::views),
		make_column("fwdFlags", &EditedMessage::fwdFlags),
		make_column("fwdFromId", &EditedMessage::fwdFromId),
		make_column("fwdName", &EditedMessage::fwdName),
		make_column("fwdDate", &EditedMessage::fwdDate),
		make_column("fwdPostAuthor", &EditedMessage::fwdPostAuthor),
		make_column("replyFlags", &EditedMessage::replyFlags),
		make_column("replyMessageId", &EditedMessage::replyMessageId),
		make_column("replyPeerId", &EditedMessage::replyPeerId),
		make_column("replyTopId", &EditedMessage::replyTopId),
		make_column("replyForumTopic", &EditedMessage::replyForumTopic),
		make_column("replySerialized", &EditedMessage::replySerialized),
		make_column("entityCreateDate", &EditedMessage::entityCreateDate),
		make_column("text", &EditedMessage::text),
		make_column("textEntities", &EditedMessage::textEntities),
		make_column("mediaPath", &EditedMessage::mediaPath),
		make_column("hqThumbPath", &EditedMessage::hqThumbPath),
		make_column("documentType", &EditedMessage::documentType),
		make_column("documentSerialized", &EditedMessage::documentSerialized),
		make_column("thumbsSerialized", &EditedMessage::thumbsSerialized),
		make_column("documentAttributesSerialized", &EditedMessage::documentAttributesSerialized),
		make_column("mimeType", &EditedMessage::mimeType)
	),
	make_table<DeletedDialog>(
		"DeletedDialog",
		make_column("fakeId", &DeletedDialog::fakeId, primary_key().autoincrement()),
		make_column("userId", &DeletedDialog::userId),
		make_column("dialogId", &DeletedDialog::dialogId),
		make_column("peerId", &DeletedDialog::peerId),
		make_column("folderId", &DeletedDialog::folderId),
		make_column("topMessage", &DeletedDialog::topMessage),
		make_column("lastMessageDate", &DeletedDialog::lastMessageDate),
		make_column("flags", &DeletedDialog::flags),
		make_column("entityCreateDate", &DeletedDialog::entityCreateDate)
	),
	make_table<RegexFilter>(
		"RegexFilter",
		make_column("id", &RegexFilter::id, primary_key()),
		make_column("text", &RegexFilter::text),
		make_column("enabled", &RegexFilter::enabled),
		make_column("reversed", &RegexFilter::reversed),
		make_column("caseInsensitive", &RegexFilter::caseInsensitive),
		make_column("dialogId", &RegexFilter::dialogId)
	),
	make_table<RegexFilterGlobalExclusion>(
		"RegexFilterGlobalExclusion",
		make_column("fakeId", &RegexFilterGlobalExclusion::fakeId, primary_key().autoincrement()),
		make_column("dialogId", &RegexFilterGlobalExclusion::dialogId),
		make_column("filterId", &RegexFilterGlobalExclusion::filterId)
	),
	make_table<SpyMessageRead>(
		"SpyMessageRead",
		make_column("fakeId", &SpyMessageRead::fakeId, primary_key().autoincrement()),
		make_column("userId", &SpyMessageRead::userId),
		make_column("dialogId", &SpyMessageRead::dialogId),
		make_column("messageId", &SpyMessageRead::messageId),
		make_column("entityCreateDate", &SpyMessageRead::entityCreateDate)
	),
	make_table<SpyMessageContentsRead>(
		"SpyMessageContentsRead",
		make_column("fakeId", &SpyMessageContentsRead::fakeId, primary_key().autoincrement()),
		make_column("userId", &SpyMessageContentsRead::userId),
		make_column("dialogId", &SpyMessageContentsRead::dialogId),
		make_column("messageId", &SpyMessageContentsRead::messageId),
		make_column("entityCreateDate", &SpyMessageContentsRead::entityCreateDate)
	),
	make_index("idx_spy_presence_userId_peerId",
			   column<SpyPresenceLog>(&SpyPresenceLog::userId),
			   column<SpyPresenceLog>(&SpyPresenceLog::peerId)),
	make_table<SpyPresenceLog>(
		"SpyPresenceLog",
		make_column("fakeId", &SpyPresenceLog::fakeId, primary_key().autoincrement()),
		make_column("userId", &SpyPresenceLog::userId),
		make_column("peerId", &SpyPresenceLog::peerId),
		make_column("eventType", &SpyPresenceLog::eventType),
		make_column("timestamp", &SpyPresenceLog::timestamp),
		make_column("details", &SpyPresenceLog::details)
	)
);

namespace SolarMigrations {

void migrateToV1(decltype(storage) &storage) {
	// drop RegexFilter table as we've added primary_key()
	try {
		storage.drop_table_if_exists("RegexFilter");
		LOG(("Migration to V1 successful."));
	} catch (const std::exception &ex) {
		LOG(("Migration to V1 failed: %1").arg(ex.what()));
	}
}

}

void runMigrations(decltype(storage) &storage) {
	constexpr int kLatestVersion = 1;

	const std::map<int, Fn<void(decltype(storage) &)>> migrations = {
		{1, SolarMigrations::migrateToV1},
	};

	int currentVersion = 0;
	try {
		if (auto versionRow = storage.get_pointer<SchemaVersion>(1)) {
			currentVersion = versionRow->version;
		} else {
			storage.insert(SchemaVersion{1, 0});
		}
	} catch (...) {
		LOG(("No SchemaVersion, assuming 0"));
		storage.insert(SchemaVersion{1, 0});
	}

	if (currentVersion >= kLatestVersion) {
		LOG(("Database is ok"));
		return;
	}

	LOG(("Database version: %1. Latest version: %2.").arg(currentVersion).arg(kLatestVersion));

	for (int v = currentVersion + 1; v <= kLatestVersion; ++v) {
		if (migrations.contains(v)) {
			try {
				LOG(("Migration for version: %1").arg(v));
				storage.begin_transaction();

				migrations.at(v)(storage);

				storage.update_all(set(c(&SchemaVersion::version) = v), where(c(&SchemaVersion::id) == 1));
				storage.commit();
				LOG(("Applied migration for version: %1.").arg(v));
			} catch (...) {
				storage.rollback();
				LOG(("Failed to apply migration for version: %1.").arg(v));
				SolarDatabase::moveCurrentDatabase();

				return;
			}
		}
	}
}

namespace SolarDatabase {

void moveCurrentDatabase() {
	const auto time = base::unixtime::now();

	if (QFile::exists("./tdata/solardata.db")) {
		QFile::rename("./tdata/solardata.db", QString("./tdata/solardata_%1.db").arg(time));
	}

	if (QFile::exists("./tdata/solardata.db-shm")) {
		QFile::rename("./tdata/solardata.db-shm", QString("./tdata/solardata_%1.db-shm").arg(time));
	}

	if (QFile::exists("./tdata/solardata.db-wal")) {
		QFile::rename("./tdata/solardata.db-wal", QString("./tdata/solardata_%1.db-wal").arg(time));
	}
}

void initialize() {
	std::lock_guard lock(databaseMutex);
	try {
		storage.sync_schema(true);

		runMigrations(storage);

		storage.sync_schema(true);
	} catch (const std::exception &ex) {
		LOG(("Database initialization failed: %1").arg(ex.what()));
		moveCurrentDatabase();

		storage.sync_schema(true);
		if (!storage.get_pointer<SchemaVersion>(1)) {
			storage.insert(SchemaVersion{1, 0});
		}
	}

	try {
		storage.open_forever();
	} catch (const std::exception &ex) {
		LOG(("Failed to keep database open: %1").arg(ex.what()));
	}
}

void addEditedMessage(const EditedMessage &message) {
	std::lock_guard lock(databaseMutex);
	try {
		storage.begin_transaction();
		storage.insert(message);
		storage.commit();
	} catch (std::exception &ex) {
		try {
			storage.rollback();
		} catch (...) {
		}
		LOG(("Failed to save edited message for some reason: %1").arg(ex.what()));
	}
}

std::vector<EditedMessage> getEditedMessages(ID userId, ID dialogId, ID messageId, ID minId, ID maxId, int totalLimit) {
	std::lock_guard lock(databaseMutex);
	try {
		return storage.get_all<EditedMessage>(
			where(
				column<EditedMessage>(&EditedMessage::userId) == userId and
				column<EditedMessage>(&EditedMessage::dialogId) == dialogId and
				column<EditedMessage>(&EditedMessage::messageId) == messageId and
				(column<EditedMessage>(&EditedMessage::fakeId) > minId or minId == 0) and
				(column<EditedMessage>(&EditedMessage::fakeId) < maxId or maxId == 0)
			),
			order_by(column<EditedMessage>(&EditedMessage::fakeId)).desc(),
			limit(totalLimit)
		);
	} catch (std::exception &ex) {
		LOG(("Failed to get edited messages: %1").arg(ex.what()));
		return {};
	}
}

bool hasRevisions(ID userId, ID dialogId, ID messageId) {
	std::lock_guard lock(databaseMutex);
	try {
		return !storage.select(
			columns(column<EditedMessage>(&EditedMessage::messageId)),
			where(
				column<EditedMessage>(&EditedMessage::userId) == userId and
				column<EditedMessage>(&EditedMessage::dialogId) == dialogId and
				column<EditedMessage>(&EditedMessage::messageId) == messageId
			),
			limit(1)
		).empty();
	} catch (std::exception &ex) {
		LOG(("Failed to check if message has revisions: %1").arg(ex.what()));
		return false;
	}
}

void addDeletedMessage(const DeletedMessage &message) {
	std::lock_guard lock(databaseMutex);
	try {
		storage.begin_transaction();
		storage.insert(message);
		storage.commit();
	} catch (std::exception &ex) {
		try {
			storage.rollback();
		} catch (...) {
		}
		LOG(("Failed to save edited message for some reason: %1").arg(ex.what()));
	}
}

std::vector<DeletedMessage> getDeletedMessages(ID userId, ID dialogId, ID topicId, ID minId, ID maxId, int totalLimit, const std::string &searchQuery) {
	std::lock_guard lock(databaseMutex);
	try {
		if (searchQuery.empty()) {
			return storage.get_all<DeletedMessage>(
				where(
					column<DeletedMessage>(&DeletedMessage::userId) == userId and
					column<DeletedMessage>(&DeletedMessage::dialogId) == dialogId and
					(column<DeletedMessage>(&DeletedMessage::topicId) == topicId or topicId == 0) and
					(column<DeletedMessage>(&DeletedMessage::messageId) > minId or minId == 0) and
					(column<DeletedMessage>(&DeletedMessage::messageId) < maxId or maxId == 0)
				),
				order_by(column<DeletedMessage>(&DeletedMessage::messageId)).desc(),
				limit(totalLimit)
			);
		}

		std::string escaped;
		escaped.reserve(searchQuery.size());
		for (const auto c : searchQuery) {
			if (c == '%' || c == '_' || c == '\\') {
				escaped += '\\';
			}
			escaped += c;
		}
		const auto pattern = "%" + escaped + "%";
		return storage.get_all<DeletedMessage>(
			where(
				column<DeletedMessage>(&DeletedMessage::userId) == userId and
				column<DeletedMessage>(&DeletedMessage::dialogId) == dialogId and
				(column<DeletedMessage>(&DeletedMessage::topicId) == topicId or topicId == 0) and
				(column<DeletedMessage>(&DeletedMessage::messageId) > minId or minId == 0) and
				(column<DeletedMessage>(&DeletedMessage::messageId) < maxId or maxId == 0) and
				like(column<DeletedMessage>(&DeletedMessage::text), pattern, "\\")
			),
			order_by(column<DeletedMessage>(&DeletedMessage::messageId)).desc(),
			limit(totalLimit)
		);
	} catch (std::exception &ex) {
		LOG(("Failed to get deleted messages: %1").arg(ex.what()));
		return {};
	}
}

bool hasDeletedMessages(ID userId, ID dialogId, ID topicId) {
	std::lock_guard lock(databaseMutex);
	try {
		return !storage.select(
			columns(column<DeletedMessage>(&DeletedMessage::dialogId)),
			where(
				column<DeletedMessage>(&DeletedMessage::userId) == userId and
				column<DeletedMessage>(&DeletedMessage::dialogId) == dialogId and
				(column<DeletedMessage>(&DeletedMessage::topicId) == topicId or topicId == 0)
			),
			limit(1)
		).empty();
	} catch (std::exception &ex) {
		LOG(("Failed to check if dialog has deleted message: %1").arg(ex.what()));
		return false;
	}
}

void removeDeletedMessage(ID userId, ID dialogId, ID messageId) {
	std::lock_guard lock(databaseMutex);
	try {
		storage.remove_all<DeletedMessage>(
			where(
				column<DeletedMessage>(&DeletedMessage::userId) == userId and
				column<DeletedMessage>(&DeletedMessage::dialogId) == dialogId and
				column<DeletedMessage>(&DeletedMessage::messageId) == messageId
			)
		);
	} catch (std::exception &ex) {
		LOG(("Failed to remove deleted message: %1").arg(ex.what()));
	}
}

void clearDeletedMessages(ID userId, ID dialogId, ID topicId) {
	std::lock_guard lock(databaseMutex);
	try {
		storage.remove_all<DeletedMessage>(
			where(
				column<DeletedMessage>(&DeletedMessage::userId) == userId and
				column<DeletedMessage>(&DeletedMessage::dialogId) == dialogId and
				(column<DeletedMessage>(&DeletedMessage::topicId) == topicId or topicId == 0)
			)
		);
	} catch (std::exception &) {
	}
}

void clearAllDeletedMessages() {
	std::lock_guard lock(databaseMutex);
	try {
		storage.remove_all<DeletedMessage>();
	} catch (const std::exception &ex) {
		LOG(("Failed to clear all deleted messages: %1").arg(ex.what()));
	}
}

void setWalMode(bool enabled) {
	std::lock_guard lock(databaseMutex);
	try {
		storage.pragma.journal_mode(
			enabled
				? sqlite_orm::journal_mode::WAL
				: sqlite_orm::journal_mode::DELETE_);
	} catch (const std::exception &ex) {
		LOG(("Failed to update SQLite journal_mode: %1").arg(ex.what()));
	}
}

void addSpyMessageRead(const SpyMessageRead &mark) {
	std::lock_guard lock(databaseMutex);
	try {
		const auto existing = storage.select(
			columns(&SpyMessageRead::fakeId),
			where(
				column<SpyMessageRead>(&SpyMessageRead::userId) == mark.userId and
				column<SpyMessageRead>(&SpyMessageRead::dialogId) == mark.dialogId and
				column<SpyMessageRead>(&SpyMessageRead::messageId) == mark.messageId
			),
			limit(1)
		);
		if (existing.empty()) {
			storage.insert(mark);
		}
	} catch (const std::exception &ex) {
		LOG(("Failed to save spy read mark: %1").arg(ex.what()));
	}
}

int getSpyMessageReadDate(ID userId, ID dialogId, int messageId) {
	std::lock_guard lock(databaseMutex);
	try {
		const auto rows = storage.select(
			columns(column<SpyMessageRead>(&SpyMessageRead::entityCreateDate)),
			where(
				column<SpyMessageRead>(&SpyMessageRead::userId) == userId and
				column<SpyMessageRead>(&SpyMessageRead::dialogId) == dialogId and
				column<SpyMessageRead>(&SpyMessageRead::messageId) >= messageId
			),
			order_by(column<SpyMessageRead>(&SpyMessageRead::messageId)).asc(),
			limit(1)
		);
		return rows.empty() ? 0 : std::get<0>(rows.front());
	} catch (const std::exception &ex) {
		LOG(("Failed to query spy read mark: %1").arg(ex.what()));
		return 0;
	}
}

void addSpyMessageContentsRead(const SpyMessageContentsRead &mark) {
	std::lock_guard lock(databaseMutex);
	try {
		const auto existing = storage.select(
			columns(column<SpyMessageContentsRead>(&SpyMessageContentsRead::fakeId)),
			where(
				column<SpyMessageContentsRead>(&SpyMessageContentsRead::userId) == mark.userId and
				column<SpyMessageContentsRead>(&SpyMessageContentsRead::dialogId) == mark.dialogId and
				column<SpyMessageContentsRead>(&SpyMessageContentsRead::messageId) == mark.messageId
			),
			limit(1)
		);
		if (existing.empty()) {
			storage.insert(mark);
		}
	} catch (const std::exception &ex) {
		LOG(("Failed to save spy contents read mark: %1").arg(ex.what()));
	}
}

int getSpyMessageContentsReadDate(ID userId, ID dialogId, int messageId) {
	std::lock_guard lock(databaseMutex);
	try {
		const auto rows = storage.select(
			columns(column<SpyMessageContentsRead>(&SpyMessageContentsRead::entityCreateDate)),
			where(
				column<SpyMessageContentsRead>(&SpyMessageContentsRead::userId) == userId and
				column<SpyMessageContentsRead>(&SpyMessageContentsRead::dialogId) == dialogId and
				column<SpyMessageContentsRead>(&SpyMessageContentsRead::messageId) == messageId
			),
			order_by(column<SpyMessageContentsRead>(&SpyMessageContentsRead::fakeId)).desc(),
			limit(1)
		);
		return rows.empty() ? 0 : std::get<0>(rows.front());
	} catch (const std::exception &ex) {
		LOG(("Failed to query spy contents read mark: %1").arg(ex.what()));
		return 0;
	}
}

void addSpyPresenceLog(const SpyPresenceLog &entry) {
	std::lock_guard lock(databaseMutex);
	try {
		storage.insert(entry);
		if ((entry.eventType == 0 || entry.eventType == 1 || entry.eventType == 5)
			&& entry.timestamp > 0) {
			const LastSeenCacheKey key{ entry.userId, entry.peerId };
			auto &cached = rememberedLastSeenCache[key];
			if (entry.timestamp > cached) {
				cached = entry.timestamp;
			}
		}
	} catch (const std::exception &ex) {
		LOG(("Failed to save spy presence log: %1").arg(ex.what()));
	}
}

std::vector<SpyPresenceLog> getSpyPresenceLogs(ID userId, ID peerId, int limitCount) {
	std::lock_guard lock(databaseMutex);
	try {
		return storage.get_all<SpyPresenceLog>(
			where(
				column<SpyPresenceLog>(&SpyPresenceLog::userId) == userId and
				column<SpyPresenceLog>(&SpyPresenceLog::peerId) == peerId
			),
			order_by(column<SpyPresenceLog>(&SpyPresenceLog::fakeId)).desc(),
			limit(limitCount)
		);
	} catch (const std::exception &ex) {
		LOG(("Failed to fetch spy presence logs: %1").arg(ex.what()));
		return {};
	}
}

int getRememberedLastSeen(ID userId, ID peerId) {
	std::lock_guard lock(databaseMutex);
	const LastSeenCacheKey key{ userId, peerId };
	if (const auto it = rememberedLastSeenCache.find(key); it != rememberedLastSeenCache.end()) {
		return it->second;
	}
	try {
		const auto rows = storage.select(
			columns(column<SpyPresenceLog>(&SpyPresenceLog::timestamp)),
			where(
				column<SpyPresenceLog>(&SpyPresenceLog::userId) == userId and
				column<SpyPresenceLog>(&SpyPresenceLog::peerId) == peerId and
				(column<SpyPresenceLog>(&SpyPresenceLog::eventType) == 0 or
				 column<SpyPresenceLog>(&SpyPresenceLog::eventType) == 1 or
				 column<SpyPresenceLog>(&SpyPresenceLog::eventType) == 5)
			),
			order_by(column<SpyPresenceLog>(&SpyPresenceLog::timestamp)).desc(),
			limit(1)
		);
		const auto ts = rows.empty() ? 0 : std::get<0>(rows.front());
		rememberedLastSeenCache.emplace(key, ts);
		return ts;
	} catch (const std::exception &ex) {
		LOG(("Failed to query remembered last seen: %1").arg(ex.what()));
		return 0;
	}
}

void clearSpyPresenceLogs(ID userId, ID peerId) {
	std::lock_guard lock(databaseMutex);
	try {
		if (userId == 0 && peerId == 0) {
			storage.remove_all<SpyPresenceLog>();
			rememberedLastSeenCache.clear();
		} else if (peerId == 0) {
			storage.remove_all<SpyPresenceLog>(
				where(column<SpyPresenceLog>(&SpyPresenceLog::userId) == userId)
			);
			rememberedLastSeenCache.clear();
		} else {
			storage.remove_all<SpyPresenceLog>(
				where(
					column<SpyPresenceLog>(&SpyPresenceLog::userId) == userId and
					column<SpyPresenceLog>(&SpyPresenceLog::peerId) == peerId
				)
			);
			rememberedLastSeenCache.erase(LastSeenCacheKey{ userId, peerId });
		}
	} catch (const std::exception &ex) {
		LOG(("Failed to clear spy presence logs: %1").arg(ex.what()));
	}
}

bool exportBackupJson(const QString &filePath, const QJsonObject &settingsJson) {
	std::lock_guard lock(databaseMutex);
	try {
		QJsonObject root;
		root["format"] = QStringLiteral("SolarGramBackup");
		root["version"] = 1;
		root["exportedAt"] = static_cast<qint64>(base::unixtime::now());
		root["settings"] = settingsJson;

		QJsonArray filtersArr;
		for (const auto &f : storage.get_all<RegexFilter>()) {
			QJsonObject obj;
			obj["idBase64"] = QString::fromLatin1(
				QByteArray(f.id.data(), static_cast<int>(f.id.size())).toBase64());
			obj["text"] = QString::fromStdString(f.text);
			obj["enabled"] = f.enabled;
			obj["reversed"] = f.reversed;
			obj["caseInsensitive"] = f.caseInsensitive;
			if (f.dialogId.has_value()) {
				obj["dialogId"] = static_cast<qint64>(*f.dialogId);
			}
			filtersArr.append(obj);
		}
		root["regexFilters"] = filtersArr;

		QJsonArray exclArr;
		for (const auto &ex : storage.get_all<RegexFilterGlobalExclusion>()) {
			QJsonObject obj;
			obj["dialogId"] = static_cast<qint64>(ex.dialogId);
			obj["filterIdBase64"] = QString::fromLatin1(
				QByteArray(ex.filterId.data(), static_cast<int>(ex.filterId.size())).toBase64());
			exclArr.append(obj);
		}
		root["regexExclusions"] = exclArr;

		QFile out(filePath);
		if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			return false;
		}
		out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
		return true;
	} catch (const std::exception &ex) {
		LOG(("Failed to export SolarGram backup: %1").arg(ex.what()));
		return false;
	}
}

std::optional<QJsonObject> importBackupJson(const QString &filePath, bool replaceExisting) {
	QFile in(filePath);
	if (!in.open(QIODevice::ReadOnly)) {
		return std::nullopt;
	}
	constexpr qint64 kMaxBackupBytes = 32 * 1024 * 1024;
	if (in.size() <= 0 || in.size() > kMaxBackupBytes) {
		return std::nullopt;
	}
	QJsonParseError err;
	const auto doc = QJsonDocument::fromJson(in.readAll(), &err);
	if (err.error != QJsonParseError::NoError || !doc.isObject()) {
		return std::nullopt;
	}
	const auto root = doc.object();
	if (root.value("format").toString() != QStringLiteral("SolarGramBackup")) {
		return std::nullopt;
	}

	std::lock_guard lock(databaseMutex);
	try {
		storage.begin_transaction();
		if (replaceExisting) {
			storage.remove_all<RegexFilter>();
			storage.remove_all<RegexFilterGlobalExclusion>();
		}
		const auto filtersArr = root.value("regexFilters").toArray();
		for (const auto &val : filtersArr) {
			if (!val.isObject()) continue;
			const auto obj = val.toObject();
			const auto rawId = QByteArray::fromBase64(obj.value("idBase64").toString().toLatin1());
			if (rawId.isEmpty()) continue;
			RegexFilter filter;
			filter.id.assign(rawId.begin(), rawId.end());
			filter.text = obj.value("text").toString().toStdString();
			filter.enabled = obj.value("enabled").toBool(true);
			filter.reversed = obj.value("reversed").toBool(false);
			filter.caseInsensitive = obj.value("caseInsensitive").toBool(false);
			if (obj.contains("dialogId") && !obj.value("dialogId").isNull()) {
				filter.dialogId = static_cast<ID>(obj.value("dialogId").toInteger());
			}
			storage.replace(filter);
		}
		const auto exclArr = root.value("regexExclusions").toArray();
		for (const auto &val : exclArr) {
			if (!val.isObject()) continue;
			const auto obj = val.toObject();
			const auto rawId = QByteArray::fromBase64(obj.value("filterIdBase64").toString().toLatin1());
			if (rawId.isEmpty()) continue;
			const auto did = static_cast<ID>(obj.value("dialogId").toInteger());
			std::vector<char> fid(rawId.begin(), rawId.end());
			const auto exists = !storage.select(
				columns(column<RegexFilterGlobalExclusion>(&RegexFilterGlobalExclusion::fakeId)),
				where(
					column<RegexFilterGlobalExclusion>(&RegexFilterGlobalExclusion::dialogId) == did and
					column<RegexFilterGlobalExclusion>(&RegexFilterGlobalExclusion::filterId) == fid
				),
				limit(1)
			).empty();
			if (!exists) {
				RegexFilterGlobalExclusion ex;
				ex.fakeId = 0;
				ex.dialogId = did;
				ex.filterId = std::move(fid);
				storage.insert(ex);
			}
		}
		storage.commit();
	} catch (const std::exception &ex) {
		try {
			storage.rollback();
		} catch (...) {
		}
		LOG(("Failed to import SolarGram backup: %1").arg(ex.what()));
		return std::nullopt;
	}

	if (root.value("settings").isObject()) {
		return root.value("settings").toObject();
	}
	return QJsonObject{};
}

template<typename T>
std::vector<T> getAllT() {
	std::lock_guard lock(databaseMutex);
	try {
		return storage.get_all<T>();
	} catch (std::exception &ex) {
		LOG(("Failed to get all: %1").arg(ex.what()));
		return {};
	}
}

std::vector<RegexFilter> getAllRegexFilters() {
	return getAllT<RegexFilter>();
}

std::vector<RegexFilterGlobalExclusion> getAllFiltersExclusions() {
	return getAllT<RegexFilterGlobalExclusion>();
}

std::vector<RegexFilter> getExcludedByDialogId(ID dialogId) {
	std::lock_guard lock(databaseMutex);
	try {
		return storage.get_all<RegexFilter>(
			where(in(&RegexFilter::id,
					 storage.select(columns(&RegexFilterGlobalExclusion::filterId),
									where(is_equal(&RegexFilterGlobalExclusion::dialogId, dialogId))
					 )
			))
		);
	} catch (std::exception &ex) {
		LOG(("Failed to get excluded by dialog id: %1").arg(ex.what()));
		return {};
	}
}

int getCount() {
	std::lock_guard lock(databaseMutex);
	try {
		return storage.count<RegexFilter>();
	} catch (std::exception &ex) {
		LOG(("Failed to get count: %1").arg(ex.what()));
		return 0;
	}
}

RegexFilter getById(std::vector<char> id) {
	std::lock_guard lock(databaseMutex);
	try {
		return storage.get<RegexFilter>(
			where(column<RegexFilter>(&RegexFilter::id) == std::move(id))
		);
	} catch (std::exception &ex) {
		LOG(("Failed to get filters by id: %1").arg(ex.what()));
		return {};
	}
}

std::vector<RegexFilter> getShared() {
	std::lock_guard lock(databaseMutex);
	try {
		return storage.get_all<RegexFilter>(
			where(is_null(column<RegexFilter>(&RegexFilter::dialogId)))
		);
	} catch (std::exception &ex) {
		LOG(("Failed to get shared filters: %1").arg(ex.what()));
		return {};
	}
}

std::vector<RegexFilter> getByDialogId(ID dialogId) {
	std::lock_guard lock(databaseMutex);
	try {
		return storage.get_all<RegexFilter>(
			where(column<RegexFilter>(&RegexFilter::dialogId) == dialogId)
		);
	} catch (std::exception &ex) {
		LOG(("Failed to get filters by dialog id: %1").arg(ex.what()));
		return {};
	}
}

void addRegexFilter(const RegexFilter &filter) {
	std::lock_guard lock(databaseMutex);
	try {
		storage.begin_transaction();
		storage.replace(filter); // we're using replace as we set std::vector<char> as primary key
		storage.commit();
	} catch (std::exception &ex) {
		try {
			storage.rollback();
		} catch (...) {
		}
		LOG(("Failed to save regex filter for some reason: %1").arg(ex.what()));
	}
}

void addRegexExclusion(const RegexFilterGlobalExclusion &exclusion) {
	std::lock_guard lock(databaseMutex);
	try {
		storage.begin_transaction();
		storage.insert(exclusion);
		storage.commit();
	} catch (std::exception &ex) {
		try {
			storage.rollback();
		} catch (...) {
		}
		LOG(("Failed to save regex filter exclusion for some reason: %1").arg(ex.what()));
	}
}

void updateRegexFilter(const RegexFilter &filter) {
	std::lock_guard lock(databaseMutex);
	try {
		storage.update_all(
			set(
				c(&RegexFilter::text) = filter.text,
				c(&RegexFilter::enabled) = filter.enabled,
				c(&RegexFilter::reversed) = filter.reversed,
				c(&RegexFilter::caseInsensitive) = filter.caseInsensitive,
				c(&RegexFilter::dialogId) = filter.dialogId
			),
			where(c(&RegexFilter::id) == filter.id)
		);
	} catch (std::exception &ex) {
		LOG(("Failed to update regex filter for some reason: %1").arg(ex.what()));
	}
}

void deleteFilter(const std::vector<char> &id) {
	std::lock_guard lock(databaseMutex);
	try {
		storage.remove_all<RegexFilter>(
			where(column<RegexFilter>(&RegexFilter::id) == id)
		);
	} catch (std::exception &ex) {
		LOG(("Failed to delete regex filter for some reason: %1").arg(ex.what()));
	}
}

void deleteExclusionsByFilterId(const std::vector<char> &id) {
	std::lock_guard lock(databaseMutex);
	try {
		storage.remove_all<RegexFilterGlobalExclusion>(
			where(column<RegexFilterGlobalExclusion>(&RegexFilterGlobalExclusion::filterId) == id)
		);
	} catch (std::exception &ex) {
		LOG(("Failed to delete regex filter exclusion by filter id for some reason: %1").arg(ex.what()));
	}
}

void deleteExclusion(ID dialogId, std::vector<char> filterId) {
	std::lock_guard lock(databaseMutex);
	try {
		storage.remove_all<RegexFilterGlobalExclusion>(
			where(column<RegexFilterGlobalExclusion>(&RegexFilterGlobalExclusion::filterId) == filterId and
				column<RegexFilterGlobalExclusion>(&RegexFilterGlobalExclusion::dialogId) == dialogId
			)
		);
	} catch (std::exception &ex) {
		LOG(("Failed to delete regex filter exclusion for some reason: %1").arg(ex.what()));
	}
}

void deleteAllFilters() {
	std::lock_guard lock(databaseMutex);
	try {
		storage.remove_all<RegexFilter>();
	} catch (std::exception &ex) {
		LOG(("Failed to delete all regex filter for some reason: %1").arg(ex.what()));
	}
}

void deleteAllExclusions() {
	std::lock_guard lock(databaseMutex);
	try {
		storage.remove_all<RegexFilterGlobalExclusion>();
	} catch (std::exception &ex) {
		LOG(("Failed to delete all regex filter exclusions for some reason: %1").arg(ex.what()));
	}
}

bool hasFilters() {
	std::lock_guard lock(databaseMutex);
	try {
		return !storage.select(
			columns(column<RegexFilter>(&RegexFilter::id)),
			limit(1)
		).empty();
	} catch (std::exception &ex) {
		LOG(("Failed to check if there's any filters: %1").arg(ex.what()));
		return false;
	}
}

bool hasPerDialogFilters() {
	std::lock_guard lock(databaseMutex);
	try {
		return
			!storage.select(
				columns(column<RegexFilter>(&RegexFilter::id)),
				where(is_not_null(column<RegexFilter>(&RegexFilter::dialogId))),
				limit(1)
			).empty() ||
			!storage.select(
				columns(column<RegexFilterGlobalExclusion>(&RegexFilterGlobalExclusion::fakeId)),
				limit(1)
			).empty();
	} catch (std::exception &ex) {
		LOG(("Failed to check if there's any filters: %1").arg(ex.what()));
		return false;
	}
}

}
