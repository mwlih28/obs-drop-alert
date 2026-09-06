/*
OBS Drop Alert
Copyright (C) 2026 yazar <mwlih28@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "AlertOverlay.hpp"
#include "Alerter.hpp"
#include "DropMonitor.hpp"
#include "Settings.hpp"
#include "SettingsDialog.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include <QAction>
#include <QMainWindow>
#include <QPointer>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

namespace {

/*
 * Parçaları birbirine bağlayan sahip nesne. Kasten QObject değil: tüm bağlantılar
 * lambda + bağlam nesnesi (overlay/alerter) ile kuruluyor, böylece bu başlıkta
 * Q_OBJECT/moc gerekmeden bağlantılar üyeler yok edildiğinde otomatik kopuyor.
 */
struct DropAlert {
	QWidget *mainWindow = nullptr;
	DropMonitor *monitor = nullptr;
	AlertOverlay *overlay = nullptr;
	Alerter *alerter = nullptr;
	QPointer<SettingsDialog> dialog;

	explicit DropAlert(QWidget *window) : mainWindow(window)
	{
		overlay = new AlertOverlay(window);
		alerter = new Alerter(window);
		monitor = new DropMonitor(window);

		QObject::connect(monitor, &DropMonitor::alarmStarted, overlay, [this](const DropStatus &status) {
			overlay->setStatusText(status.text());
			overlay->startAlarm();
			alerter->startAlarm();
		});

		QObject::connect(monitor, &DropMonitor::alarmUpdated, overlay,
				 [this](const DropStatus &status) { overlay->setStatusText(status.text()); });

		QObject::connect(monitor, &DropMonitor::alarmCleared, overlay, [this]() {
			overlay->stopAlarm();
			alerter->stopAlarm();
		});

		monitor->start();
	}

	~DropAlert()
	{
		if (monitor)
			monitor->stop();
		if (alerter)
			alerter->stopAlarm();
		if (overlay)
			overlay->stopAlarm();

		delete dialog.data();
		delete monitor;
		delete overlay;
		delete alerter;
	}

	void applySettings()
	{
		monitor->applySettings();
		overlay->applySettings();
		alerter->applySettings();
	}

	/* Yayın/kayıt yeniden başladığında sayaçlar sıfırlanır; kayan pencereyi de
	 * boşaltmazsak ilk saniyelerde sahte bir sıçrama görürüz. */
	void resetWindow()
	{
		monitor->stop();
		monitor->start();
	}

	void openSettings()
	{
		if (!dialog) {
			dialog = new SettingsDialog(mainWindow);
			QObject::connect(dialog.data(), &SettingsDialog::settingsApplied, overlay,
					 [this]() { applySettings(); });
			QObject::connect(dialog.data(), &SettingsDialog::soundPreviewRequested, alerter,
					 [this]() { alerter->previewSound(); });
			QObject::connect(dialog.data(), &SettingsDialog::testAlarmRequested, overlay, [this](bool on) {
				if (on)
					monitor->fireTestAlarm();
				else
					monitor->clearTestAlarm();
			});
		}

		dialog->show();
		dialog->raise();
		dialog->activateWindow();
	}
};

DropAlert *g_dropAlert = nullptr;

void createDropAlert()
{
	if (g_dropAlert)
		return;

	auto *window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	if (!window) {
		obs_log(LOG_ERROR, "no frontend main window, plugin disabled");
		return;
	}

	settings().load();
	g_dropAlert = new DropAlert(window);
	obs_log(LOG_INFO, "drop monitor started");
}

void destroyDropAlert()
{
	delete g_dropAlert;
	g_dropAlert = nullptr;
}

void onFrontendEvent(enum obs_frontend_event event, void *)
{
	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		createDropAlert();
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		if (g_dropAlert)
			g_dropAlert->resetWindow();
		break;
	case OBS_FRONTEND_EVENT_EXIT:
		destroyDropAlert();
		break;
	default:
		break;
	}
}

} // namespace

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);

	auto *action = static_cast<QAction *>(obs_frontend_add_tools_menu_qaction(obs_module_text("Menu.Settings")));
	if (action) {
		QObject::connect(action, &QAction::triggered, []() {
			if (g_dropAlert)
				g_dropAlert->openSettings();
		});
	}

	obs_frontend_add_event_callback(onFrontendEvent, nullptr);
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(onFrontendEvent, nullptr);
	destroyDropAlert();
	obs_log(LOG_INFO, "plugin unloaded");
}
