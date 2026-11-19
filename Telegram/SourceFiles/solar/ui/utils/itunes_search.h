// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include <QtGui/QPixmap>

namespace Solar::Ui::Itunes {

QPixmap FetchCover(const QString &performer,
                   const QString &title,
                   int sizeHintPx = 300,
                   int timeoutMs = 5000);

}
