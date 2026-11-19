// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "solar/data/entities.h"

#include <QObject>
#include <QString>
#include <unordered_map>
#include <unordered_set>

struct CustomBadge
{
	EmojiStatusId emojiStatusId;
	QString text;
};

class RCManager final : public QObject
{
	Q_OBJECT
public:
	static RCManager &getInstance() {
		static RCManager instance;
		return instance;
	}

	RCManager(const RCManager &) = delete;
	RCManager &operator=(const RCManager &) = delete;
	RCManager(RCManager &&) = delete;
	RCManager &operator=(RCManager &&) = delete;

	void start();

	[[nodiscard]] const std::unordered_set<ID> &developers() const {
		return _developers;
	}

	[[nodiscard]] const std::unordered_set<ID> &channels() const {
		return _officialChannels;
	}

	[[nodiscard]] const std::unordered_set<ID> &supporters() const {
		return _supporters;
	}

	[[nodiscard]] const std::unordered_set<ID> &supporterChannels() const {
		return _supporterChannels;
	}

	[[nodiscard]] const std::unordered_map<ID, CustomBadge> &supporterCustomBadges() const {
		return _customBadges;
	}

	[[nodiscard]] QString donateUsername() const {
		return _donateUsername;
	}

	[[nodiscard]] QString donateAmountUsd() const {
		return _donateAmountUsd;
	}

	[[nodiscard]] QString donateAmountTon() const {
		return _donateAmountTon;
	}

	[[nodiscard]] QString donateAmountRub() const {
		return _donateAmountRub;
	}

private:
	RCManager() = default;
	~RCManager() = default;

	std::unordered_set<ID> _developers = {};
	std::unordered_set<ID> _officialChannels = {};
	std::unordered_set<ID> _supporters = {};
	std::unordered_set<ID> _supporterChannels = {};
	std::unordered_map<ID, CustomBadge> _customBadges = {};

	QString _donateUsername = QString("JoiveSer");
	QString _donateAmountUsd = QString("0");
	QString _donateAmountTon = QString("0");
	QString _donateAmountRub = QString("0");
};
