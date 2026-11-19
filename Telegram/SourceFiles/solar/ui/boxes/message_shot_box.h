// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "solar/features/message_shot/message_shot.h"
#include "ui/layers/box_content.h"

class MessageShotBox : public Ui::BoxContent
{
public:
	MessageShotBox(QWidget *parent, SolarFeatures::MessageShot::ShotConfig config);

	bool tookShot() const {
		return _tookShot;
	}

protected:
	void prepare() override;

private:
	void setupContent();

	SolarFeatures::MessageShot::ShotConfig _config;
	std::shared_ptr<style::palette> _selectedPalette;

	bool _tookShot = false;
};
