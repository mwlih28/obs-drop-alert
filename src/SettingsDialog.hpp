/*
OBS Drop Alert
Copyright (C) 2026 yazar <mwlih28@users.noreply.github.com>

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

#pragma once

#include <QDialog>
#include <QTimer>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QSpinBox;
class QTabWidget;

class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget *parent = nullptr);

	void setTestChecked(bool on);

signals:
	void settingsApplied();
	void testAlarmRequested(bool on);
	void soundPreviewRequested();
	void warnSoundPreviewRequested();

protected:
	void done(int result) override;
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;

private slots:
	void onSave();
	void onRestoreDefaults();
	void onBrowseSound();
	void onBrowseWarnSound();
	void onToggleTest(bool checked);
	void onRunOptimizer();
	void onDiagTick();
	void updateEnabledStates();

private:
	void buildUi();
	void fitToScreen();

	QWidget *buildMonitorTab();
	QWidget *buildVisualTab();
	QWidget *buildSoundTab();
	QWidget *buildOptimizerTab();
	QWidget *buildDiagnosticsTab();

	void loadFromSettings();
	void storeToSettings();

	QTabWidget *m_tabs = nullptr;

	// İzleme
	QCheckBox *m_network = nullptr;
	QCheckBox *m_render = nullptr;
	QCheckBox *m_encoder = nullptr;
	QCheckBox *m_disk = nullptr;
	QCheckBox *m_outputError = nullptr;
	QCheckBox *m_streamDrop = nullptr;
	QCheckBox *m_stall = nullptr;
	QSpinBox *m_stallMs = nullptr;
	QSpinBox *m_eventHold = nullptr;
	QDoubleSpinBox *m_networkPct = nullptr;
	QDoubleSpinBox *m_renderPct = nullptr;
	QDoubleSpinBox *m_encoderPct = nullptr;
	QDoubleSpinBox *m_recordPct = nullptr;
	QDoubleSpinBox *m_diskGb = nullptr;
	QSpinBox *m_pollMs = nullptr;
	QSpinBox *m_windowSeconds = nullptr;
	QSpinBox *m_triggerSamples = nullptr;
	QSpinBox *m_clearSeconds = nullptr;
	QCheckBox *m_onlyWhenActive = nullptr;

	// Görsel
	QComboBox *m_visualMode = nullptr;
	QComboBox *m_theme = nullptr;
	QCheckBox *m_pulse = nullptr;
	QDoubleSpinBox *m_pulseHz = nullptr;
	QSpinBox *m_borderWidth = nullptr;
	QSpinBox *m_tintOpacity = nullptr;
	QCheckBox *m_modernGlow = nullptr;
	QCheckBox *m_hudCorners = nullptr;
	QCheckBox *m_showSparkline = nullptr;

	// Ses & Bildirim
	QCheckBox *m_soundEnabled = nullptr;
	QLineEdit *m_soundPath = nullptr;
	QPushButton *m_browse = nullptr;
	QPushButton *m_preview = nullptr;
	QSpinBox *m_soundRepeat = nullptr;

	QCheckBox *m_warnSoundEnabled = nullptr;
	QLineEdit *m_warnSoundPath = nullptr;
	QPushButton *m_warnBrowse = nullptr;
	QPushButton *m_warnPreview = nullptr;

	QCheckBox *m_builtInSynth = nullptr;
	QCheckBox *m_taskbarFlash = nullptr;

	// Optimizasyon & Otopilot
	QCheckBox *m_autoHighPriority = nullptr;
	QCheckBox *m_autoMultimediaTimer = nullptr;
	QCheckBox *m_enablePreWarning = nullptr;
	QCheckBox *m_autoPausePreview = nullptr;
	QPushButton *m_optimizeNow = nullptr;

	// Canlı Teşhis
	QLabel *m_diagFps = nullptr;
	QLabel *m_diagDropped = nullptr;
	QLabel *m_diagLagged = nullptr;
	QLabel *m_diagSkipped = nullptr;
	QLabel *m_diagDisk = nullptr;
	QLabel *m_diagOptimizerStatus = nullptr;
	QTimer m_diagTimer;

	QPushButton *m_test = nullptr;
};
