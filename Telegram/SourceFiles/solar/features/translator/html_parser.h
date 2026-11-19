// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "ui/text/text_entity.h"

#include <QtCore/QString>

namespace Solar::Translator::Html {

[[nodiscard]] QString entitiesToHtml(const TextWithEntities &text);
[[nodiscard]] TextWithEntities htmlToEntities(const QString &text);

}