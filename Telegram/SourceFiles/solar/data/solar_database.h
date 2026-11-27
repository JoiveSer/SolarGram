// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "solar/data/entities.h"

#include <QtCore/QString>

#include <functional>
#include <optional>

class SchemaVersion
{
public:
	int id;
	int version;
};

namespace SolarDatabase {

void initialize();

void addEditedMessage(const EditedMessage &message);
std::vector<EditedMessage> getEditedMessages(ID userId, ID dialogId, ID messageId, ID minId, ID maxId, int totalLimit);
bool hasRevisions(ID userId, ID dialogId, ID messageId);

void addDeletedMessage(const DeletedMessage &message);
std::vector<DeletedMessage> getDeletedMessages(ID userId, ID dialogId, ID topicId, ID minId, ID maxId, int totalLimit, const std::string &searchQuery = "");
bool hasDeletedMessages(ID userId, ID dialogId, ID topicId);
void removeDeletedMessage(ID userId, ID dialogId, ID messageId);
void clearDeletedMessages(ID userId, ID dialogId, ID topicId);
void clearAllDeletedMessages();

void setWalMode(bool enabled);

void addSpyMessageRead(const SpyMessageRead &mark);
int getSpyMessageReadDate(ID userId, ID dialogId, int messageId);
void addSpyMessageContentsRead(const SpyMessageContentsRead &mark);
int getSpyMessageContentsReadDate(ID userId, ID dialogId, int messageId);

void addSpyPresenceLog(const SpyPresenceLog &entry);
std::vector<SpyPresenceLog> getSpyPresenceLogs(ID userId, ID peerId, int limitCount = 200);
int getRememberedLastSeen(ID userId, ID peerId);
void clearSpyPresenceLogs(ID userId, ID peerId);

std::optional<ChatStreak> getChatStreak(ID userId, ID dialogId);
std::vector<ChatStreak> getAllChatStreaks(ID userId);
void upsertChatStreak(const ChatStreak &streak);
void clearChatStreaks(ID userId = 0);

bool exportBackupJson(const QString &filePath, const QJsonObject &settingsJson);
std::optional<QJsonObject> importBackupJson(const QString &filePath, bool replaceExisting);

std::vector<RegexFilter> getAllRegexFilters();
RegexFilter getById(std::vector<char> id);
std::vector<RegexFilter> getShared();
std::vector<RegexFilter> getByDialogId(ID dialogId);
std::vector<RegexFilterGlobalExclusion> getAllFiltersExclusions();
std::vector<RegexFilter> getExcludedByDialogId(ID dialogId);

int getCount();


void addRegexFilter(const RegexFilter &filter);
void addRegexExclusion(const RegexFilterGlobalExclusion &exclusion);

void updateRegexFilter(const RegexFilter &filter);

void deleteFilter(const std::vector<char> &id);
void deleteExclusionsByFilterId(const std::vector<char> &id);
void deleteExclusion(ID dialogId, std::vector<char> filterId);

void deleteAllFilters();
void deleteAllExclusions();

bool hasFilters();
bool hasPerDialogFilters();

void moveCurrentDatabase();

}
