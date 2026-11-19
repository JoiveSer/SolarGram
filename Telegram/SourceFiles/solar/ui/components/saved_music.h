// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "ui/widgets/buttons.h"

namespace Ui {
class FlatLabel;
} // namespace Ui

namespace Data {
class DocumentMedia;
}

namespace Info::Profile {

struct MusicButtonData;

struct ResultCover
{
	QPixmap pix;
	QColor bg;
	bool noCover;
};

class SolarMusicButton final : public Ui::RippleButton
{
public:
	SolarMusicButton(QWidget *parent, MusicButtonData data, std::optional<QColor> overrideBg, Fn<void()> handler);
	~SolarMusicButton();

	void updateData(MusicButtonData data);

	rpl::producer<> onReady() const {
		return _onReady.events();
	}

private:
	void downloadAndMakeCover(FullMsgId msgId);
	void makeCover();

	void paintEvent(QPaintEvent *e) override;
	int resizeGetHeight(int newWidth) override;

	std::unique_ptr<Ui::FlatLabel> _performer;
	std::unique_ptr<Ui::FlatLabel> _title;
	std::shared_ptr<Data::DocumentMedia> _mediaView;
	std::optional<ResultCover> _currentCover;
	rpl::event_stream<> _onReady;

	QString _performerText;
	QString _titleText;

	std::optional<QColor> _overrideBg;

};

} // namespace Info::Profile
