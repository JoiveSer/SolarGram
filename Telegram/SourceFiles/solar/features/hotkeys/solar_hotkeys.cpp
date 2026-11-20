// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/features/hotkeys/solar_hotkeys.h"

#include "core/application.h"
#include "core/shortcuts.h"
#include "lang_auto.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "solar/data/messages_storage.h"
#include "solar/data/solar_database.h"
#include "solar/solar_settings.h"
#include "solar/ui/settings/settings_solar_utils.h"
#include "storage/storage_domain.h"
#include "window/main_window.h"
#include "window/window_controller.h"
#include "window/window_session_controller.h"

#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCoreApplication>
#include <QtGui/QKeySequence>
#include <QtGui/QShortcut>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif // Q_OS_WIN

namespace SolarHotkeys {
namespace {

constexpr int kHotkeyAlwaysOnTop = 0x5301;
constexpr int kHotkeyStreamerMode = 0x5302;
constexpr int kHotkeyPanicWipe = 0x5303;

#ifdef Q_OS_WIN
class GlobalHotkeyFilter final : public QAbstractNativeEventFilter {
public:
	GlobalHotkeyFilter() {
		SolarSettings::getInstance().globalHotkeysEnabledValue(
		) | rpl::on_next([=](bool enabled) {
			if (enabled) {
				registerHotkeys();
			} else {
				unregisterHotkeys();
			}
		}, _lifetime);
	}

	~GlobalHotkeyFilter() override {
		unregisterHotkeys();
	}

	void registerHotkeys() {
		if (_aotRegistered || _streamerRegistered || _wipeRegistered) {
			return;
		}
		_aotRegistered = RegisterHotKey(
			nullptr,
			kHotkeyAlwaysOnTop,
			MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT,
			VK_F10) != FALSE;
		_streamerRegistered = RegisterHotKey(
			nullptr,
			kHotkeyStreamerMode,
			MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT,
			VK_F11) != FALSE;
		_wipeRegistered = RegisterHotKey(
			nullptr,
			kHotkeyPanicWipe,
			MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT,
			VK_F12) != FALSE;
		if (!_aotRegistered || !_streamerRegistered || !_wipeRegistered) {
			LOG(("SolarHotkeys: RegisterHotKey partial failure (%1/%2/%3)")
				.arg(_aotRegistered ? 1 : 0)
				.arg(_streamerRegistered ? 1 : 0)
				.arg(_wipeRegistered ? 1 : 0));
		}
	}

	void unregisterHotkeys() {
		if (_aotRegistered) {
			UnregisterHotKey(nullptr, kHotkeyAlwaysOnTop);
			_aotRegistered = false;
		}
		if (_streamerRegistered) {
			UnregisterHotKey(nullptr, kHotkeyStreamerMode);
			_streamerRegistered = false;
		}
		if (_wipeRegistered) {
			UnregisterHotKey(nullptr, kHotkeyPanicWipe);
			_wipeRegistered = false;
		}
	}

	bool nativeEventFilter(
			const QByteArray &eventType,
			void *message,
			native_event_filter_result *result) override {
		Q_UNUSED(result);
		if (eventType != "windows_generic_MSG" || !message) {
			return false;
		}
		const auto msg = static_cast<MSG*>(message);
		if (msg->message != WM_HOTKEY) {
			return false;
		}
		switch (int(msg->wParam)) {
		case kHotkeyAlwaysOnTop:
			crl::on_main([] {
				auto &s = SolarSettings::getInstance();
				s.setAlwaysOnTop(!s.alwaysOnTop());
			});
			return true;
		case kHotkeyStreamerMode:
			crl::on_main([] {
				auto &s = SolarSettings::getInstance();
				s.setStreamerMode(!s.streamerMode());
			});
			return true;
		case kHotkeyPanicWipe:
			crl::on_main([] {
				TriggerPanicWipe();
			});
			return true;
		default:
			return false;
		}
	}

private:
	bool _aotRegistered = false;
	bool _streamerRegistered = false;
	bool _wipeRegistered = false;
	rpl::lifetime _lifetime;
};

void EnsureGlobalHotkeysInstalled() {
	static bool installed = false;
	if (installed) {
		return;
	}
	const auto app = QCoreApplication::instance();
	if (!app) {
		return;
	}
	installed = true;
	static auto filter = std::make_unique<GlobalHotkeyFilter>();
	app->installNativeEventFilter(filter.get());
	QObject::connect(app, &QCoreApplication::aboutToQuit, app, [app] {
		if (filter) {
			app->removeNativeEventFilter(filter.get());
			filter.reset();
		}
	});
}
#else // Q_OS_WIN
void EnsureGlobalHotkeysInstalled() {
}
#endif // Q_OS_WIN

} // namespace

void TriggerPanicWipe() {
	SolarMessages::clearAllDeletedMessages();
	SolarDatabase::clearSpyPresenceLogs(0, 0);

	auto &settings = SolarSettings::getInstance();
	settings.setHiddenChatsRevealed(false);
	settings.setDecoyModeActive(false);

	if (Core::App().domain().local().hasLocalPasscode()) {
		Core::App().lockByPasscode();
	} else {
		Core::Quit(Core::QuitReason::Default);
	}
}

void AttachWindowShortcuts(not_null<Window::Controller*> controller) {
	EnsureGlobalHotkeysInstalled();

	const auto widget = controller->widget();
	const auto weak = base::make_weak(controller);

	const auto ghostShortcut = new QShortcut(widget);
	ghostShortcut->setContext(Qt::WindowShortcut);
	const auto applyGhostKey = [=](const QString &seq) {
		const auto parsed = QKeySequence(seq.trimmed(), QKeySequence::PortableText);
		ghostShortcut->setKey(parsed);
		ghostShortcut->setEnabled(!parsed.isEmpty());
	};
	SolarSettings::getInstance().ghostModeShortcutValue(
	) | rpl::on_next(applyGhostKey, widget->lifetime());

	QObject::connect(ghostShortcut, &QShortcut::activated, widget, [weak] {
		const auto ctrl = weak.get();
		if (!ctrl) {
			return;
		}
		const auto sessionController = ctrl->sessionController();
		if (!sessionController) {
			return;
		}
		auto &ghost = SolarSettings::ghost(&sessionController->session());
		const auto next = !ghost.isGhostModeActive();
		ghost.setGhostModeEnabled(next);
		ctrl->showToast(next
			? tr::solar_GhostModeEnabled(tr::now)
			: tr::solar_GhostModeDisabled(tr::now));
	});

	const auto searchShortcut = new QShortcut(
		QKeySequence(u"Alt+F"_q),
		widget);
	searchShortcut->setContext(Qt::WindowShortcut);
	QObject::connect(searchShortcut, &QShortcut::activated, widget, [] {
		Shortcuts::Launch(Shortcuts::Command::Search);
	});

	const auto savedShortcut = new QShortcut(
		QKeySequence(u"Alt+C"_q),
		widget);
	savedShortcut->setContext(Qt::WindowShortcut);
	QObject::connect(savedShortcut, &QShortcut::activated, widget, [] {
		Shortcuts::Launch(Shortcuts::Command::ChatSelf);
	});

	const auto recentShortcut = new QShortcut(
		QKeySequence(u"Alt+R"_q),
		widget);
	recentShortcut->setContext(Qt::WindowShortcut);
	QObject::connect(recentShortcut, &QShortcut::activated, widget, [weak] {
		if (const auto ctrl = weak.get()) {
			if (const auto sessionController = ctrl->sessionController()) {
				Settings::ShowRecentChatsBox(sessionController);
			}
		}
	});
}

} // namespace SolarHotkeys
