// This is the source code of SolarGram Desktop.
// Built on Telegram Desktop under GNU GPLv3.
#include "solar/solar_worker.h"

#include "apiwrap.h"
#include "solar_settings.h"
#include "base/timer.h"
#include "base/unixtime.h"
#include "core/application.h"
#include "data/data_user.h"
#include "data/entities.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "solar/utils/telegram_helpers.h"
#include "window/window_controller.h"
#include "window/window_session_controller.h"

namespace SolarWorker {

void runOnce();

std::unordered_map<ID, bool> state;
std::unordered_map<ID, crl::time> blinkUntil;

[[nodiscard]] bool ShouldOfflineForSession(not_null<Main::Session*> session) {
	if (const auto w = Core::App().activeWindow()) {
		if (const auto c = w->sessionController(); c && &c->session() == session) {
			if (const auto peer = c->activeChatCurrent().peer()) {
				return SolarSettings::getInstance().effectiveShouldOfflineAfterOnline(
					session,
					getDialogIdFromPeer(peer));
			}
		}
	}
	return SolarSettings::ghost(session).sendOfflinePacketAfterOnline();
}

base::Timer &workerTimer() {
	static base::Timer timer([] {
		runOnce();
	});
	return timer;
}

void markAsOnline(not_null<Main::Session*> session) {
	const auto id = session->userId().bare;
	state[id] = true;
	const auto &ghost = SolarSettings::ghost(session);
	if (ghost.onlineBlink() && ShouldOfflineForSession(session)) {
		blinkUntil[id] = crl::now() + crl::time(1500);
		session->api().request(MTPaccount_UpdateStatus(
			MTP_bool(false)
		)).send();
	}
	workerTimer().cancel();
	workerTimer().callEach(1500);
}

void lateInit() {
	for (const auto &[index, account] : Core::App().domain().accounts()) {
		if (const auto session = account->maybeSession()) {
			const auto id = session->userId().bare;
			state[id] = true;
		}
	}
}

void runOnce() {
	if (!Core::IsAppLaunched() || !Core::App().domain().started() || Core::Quitting()) {
		return;
	}

	if (state.empty()) {
		lateInit();
	}

	const auto t = base::unixtime::now();

	for (const auto &[index, account] : Core::App().domain().accounts()) {
		if (account) {
			if (const auto session = account->maybeSession()) {
				const auto id = session->userId().bare;
				if (!state.contains(id)) {
					state[id] = true;
				}

				if (!ShouldOfflineForSession(session)) {
					continue;
				}
				if (const auto it = blinkUntil.find(id); it != blinkUntil.end()) {
					if (crl::now() < it->second) {
						continue;
					}
					blinkUntil.erase(it);
				}

				if (state[id] || session->user()->lastseen().isOnline(t)) {
					session->api().request(MTPaccount_UpdateStatus(
						MTP_bool(true)
					)).send();
					state[id] = false;

					DEBUG_LOG(("[SolarGram] Sent offline for account with id %1").arg(id));
				}
			}
		}
	}
}

void initialize() {
	workerTimer().callEach(3000);
}

}
