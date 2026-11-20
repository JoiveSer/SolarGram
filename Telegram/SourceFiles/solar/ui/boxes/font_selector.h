// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "boxes/abstract_box.h"

#include <optional>
#include "base/binary_guard.h"

struct LanguageId;
struct Font;

namespace Ui {

class MultiSelect;
struct ScrollToRequest;
class VerticalLayout;

} // namespace Ui

namespace Window {

class SessionController;

} // namespace Window

namespace SolarUi {

class FontSelectorBox : public Ui::BoxContent
{
public:
	FontSelectorBox(
		QWidget *,
		Window::SessionController *controller,
		Fn<void(QString font)> hook,
		std::optional<QString> initialFont = std::nullopt);

	void setInnerFocus() override;
	static base::binary_guard Show(
		Window::SessionController *controller,
		const Fn<void(QString font)> hook,
		std::optional<QString> initialFont = std::nullopt);

private:
	QString _selectedFont;
	std::optional<QString> _initialFont;

protected:
	void prepare() override;
	void keyPressEvent(QKeyEvent *e) override;

private:
	void setupTop(not_null<Ui::VerticalLayout*> container);
	[[nodiscard]] int rowsInPage() const;

	Window::SessionController *_controller = nullptr;
	rpl::event_stream<bool> _translateChatTurnOff;
	Fn<void()> _setInnerFocus;
	Fn<Ui::ScrollToRequest(int rows)> _jump;
	Fn<void(QString font)> _hook;

};

}
