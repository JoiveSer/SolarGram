// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#pragma once

#include "base/qthelp_regex.h"
#include "window/window_session_controller.h"

#include <QtCore/QUrl>

namespace SolarUrlHandlers {

using Match = qthelp::RegularExpressionMatch;

bool ResolveUser(
	Window::SessionController *controller,
	const Match &match,
	const QVariant &context);

bool ResolveChat(
	Window::SessionController *controller,
	const Match &match,
	const QVariant &context);

bool HandleSolar(
	Window::SessionController *controller,
	const Match &match,
	const QVariant &context);

bool HandleSupport(
	Window::SessionController *controller,
	const Match &match,
	const QVariant &context);

bool HandleSolarSettings(
	Window::SessionController *controller,
	const Match &match,
	const QVariant &context);

bool TryHandleSpotify(const QString &url);

[[nodiscard]] bool IsWebUrl(const QUrl &url);
[[nodiscard]] bool IsUnsafeExternalUrl(const QString &url);

}
