// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include <QString>
#include "json.hpp"

inline void to_json(nlohmann::json &j, const QString &q) {
	j = nlohmann::json(q.toStdString());
}

inline void from_json(const nlohmann::json &j, QString &q) {
	q = QString::fromStdString(j.get<std::string>());
}
