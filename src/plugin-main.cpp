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
#include <QMenu>
#include <QMenuBar>
#include <QPointer>
#include <QSignalBlocker>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

namespace {

/* OBS'in Yardım menüsünün .ui dosyasındaki nesne adı; kendi menümüzü onun
 * soluna eklemek için kullanılıyor, böylece Yardım her zaman en sağda kalır. */
constexpr const char *kHelpMenuObjectName = "menuBasic_MainMenu_Help";

/*
 * Parçaları birbirine bağlayan sahip nesne. Kasten QObject değil: tüm bağlantılar
 * lambda + bağlam nesnesi ile kuruluyor, böylece burada Q_OBJECT/moc gerekmeden
 * bağlantılar üyeler yok edildiğinde otomatik kopuyor.
 */
struct DropAlert {
	QMainWindow *mainWindow = nullptr;
	DropMonitor *monitor = nullptr;
	AlertOverlay *overlay = nullptr;
	Alerter *alerter = nullptr;
	QPointer<SettingsDialog> dialog;

	QPointer<QMenu> menu;
	QPointer<QAction> actTest;

	bool testActive = false;

	explicit DropAlert(QMainWindow *window) : mainWindow(window)
	{
		overlay = new AlertOverlay(window);
		alerter = new Alerter(window);
		monitor = new DropMonitor(window);

		QObject::connect(monitor, &DropMonitor::alarmStarted, overlay, [this](const DropStatus &status) {
			overlay->setStatusText(status.title(), status.cause(), status.hint());
			overlay->startAlarm();
			alerter->startAlarm();
		});

		QObject::connect(monitor, &DropMonitor::alarmUpdated, overlay,
				 [this](const DropStatus &status) { overlay->setStatusText(status.title(), status.cause(), status.hint()); });

		QObject::connect(monitor, &DropMonitor::alarmCleared, overlay, [this]() {
			overlay->stopAlarm();
			alerter->stopAlarm();
		});

		buildMenu();
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

		/* Menüyü menü çubuğundan sök, yoksa OBS kapanırken sahipsiz kalır. */
		if (menu) {
			if (QMenuBar *bar = mainWindow ? mainWindow->menuBar() : nullptr)
				bar->removeAction(menu->menuAction());
			delete menu.data();
		}

		delete dialog.data();
		delete monitor;
		delete overlay;
		delete alerter;
	}

	/* OBS'in üst menü çubuğuna kendi menümüzü ekler. obs_frontend API'si yalnızca
	 * Araçlar menüsüne ekleme sunduğu için menü çubuğuna doğrudan Qt üzerinden
	 * giriliyor; ana pencereyi zaten obs_frontend_get_main_window()'dan alıyoruz. */
	void buildMenu()
	{
		QMenuBar *bar = mainWindow ? mainWindow->menuBar() : nullptr;
		if (!bar) {
			obs_log(LOG_WARNING, "no menu bar, settings will not be reachable");
			return;
		}

		menu = new QMenu(QString::fromUtf8(obs_module_text("Menu.Title")), bar);
		menu->setObjectName("dropAlertMenu");

		QAction *actSettings = menu->addAction(QString::fromUtf8(obs_module_text("Menu.Settings")));
		menu->addSeparator();
		actTest = menu->addAction(QString::fromUtf8(obs_module_text("Menu.Test")));
		actTest->setCheckable(true);

		QObject::connect(actSettings, &QAction::triggered, menu, [this]() { openSettings(); });
		QObject::connect(actTest.data(), &QAction::toggled, menu, [this](bool on) { setTestAlarm(on); });

		QAction *before = nullptr;
		if (QMenu *help = bar->findChild<QMenu *>(kHelpMenuObjectName))
			before = help->menuAction();
		if (!before && !bar->actions().isEmpty())
			before = bar->actions().last();

		if (before)
			bar->insertMenu(before, menu);
		else
			bar->addMenu(menu);
	}

	void applySettings()
	{
		monitor->applySettings();
		overlay->applySettings();
		alerter->applySettings();
	}

	/* Test alarmının tek doğruluk kaynağı: menüdeki geçmeli öğe ile ayar
	 * penceresindeki düğme buradan senkron tutuluyor. */
	void setTestAlarm(bool on)
	{
		if (testActive == on)
			return;
		testActive = on;

		if (actTest) {
			const QSignalBlocker blocker(actTest.data());
			actTest->setChecked(on);
		}
		if (dialog)
			dialog->setTestChecked(on);

		if (on)
			monitor->fireTestAlarm();
		else
			monitor->clearTestAlarm();
	}

	/* Yayın/kayıt yeniden başladığında sayaçlar sıfırlanır; kayan pencereyi de
	 * boşaltmazsak ilk saniyelerde sahte bir sıçrama görürüz. */
	void resetWindow()
	{
		if (testActive)
			return;
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
			QObject::connect(dialog.data(), &SettingsDialog::testAlarmRequested, overlay,
					 [this](bool on) { setTestAlarm(on); });
		}

		dialog->setTestChecked(testActive);
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
	obs_frontend_add_event_callback(onFrontendEvent, nullptr);
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(onFrontendEvent, nullptr);
	destroyDropAlert();
	obs_log(LOG_INFO, "plugin unloaded");
}
