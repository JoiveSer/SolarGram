// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "solar/data/entities.h"
#include "settings/settings_common.h"
#include "settings/settings_common_session.h"
#include "ui/layers/box_content.h"
#include "ui/widgets/popup_menu.h"

class BoxContent;

namespace Window {
class Controller;
class SessionController;
} // namespace Window

namespace Settings {

class SolarFiltersList : public Section<SolarFiltersList>
{
public:
	SolarFiltersList(QWidget *parent, not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

	void checkBeforeClose(Fn<void()> close) override;

private:
	void setupContent(not_null<Window::SessionController*> controller);
	void initializeSharedFilters(not_null<Ui::VerticalLayout*> container);
	void initializeShadowBan(not_null<Ui::VerticalLayout*> container);

	void addNewFilter(const RegexFilter &filter, bool exclusion = false);

	not_null<Window::SessionController*> _controller;
	not_null<Ui::VerticalLayout*> _content;

	std::vector<RegexFilter> filters;
	std::vector<RegexFilter> exclusions;

	Ui::FlatLabel *filtersTitle = nullptr;
	Ui::FlatLabel *excludedTitle = nullptr;

	std::optional<long long> dialogId;
	bool shadowBan;
};

} // namespace Settings
