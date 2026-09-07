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

#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QPushButton;
class QSpinBox;

/*
 * Araçlar menüsünden açılan ayar penceresi.
 *
 * "Test uyarısı" düğmesi geçmeli (checkable): basılı kaldığı sürece uyarı zinciri
 * gerçek bir drop olmuş gibi çalışır. Böylece kullanıcı eşikleri ve görünümü
 * yayına çıkmadan, drop beklemeden ayarlayabilir.
 */
class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget *parent = nullptr);

	/* Test alarmı menüden de açılıp kapatılabildiği için düğmenin durumu
	 * dışarıdan senkronlanır; sinyal yaymadan ayarlar. */
	void setTestChecked(bool on);

signals:
	/* Ayarlar kaydedildi; izleyici ve katman kendini yenilemeli. */
	void settingsApplied();

	/* true: test alarmını başlat, false: durdur. */
	void testAlarmRequested(bool on);

	void soundPreviewRequested();

protected:
	void done(int result) override;

private slots:
	void onSave();
	void onRestoreDefaults();
	void onBrowseSound();
	void onToggleTest(bool checked);
	void updateEnabledStates();

private:
	void buildUi();
	void loadFromSettings();
	void storeToSettings();

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

	QComboBox *m_visualMode = nullptr;
	QCheckBox *m_pulse = nullptr;
	QDoubleSpinBox *m_pulseHz = nullptr;
	QSpinBox *m_borderWidth = nullptr;
	QSpinBox *m_tintOpacity = nullptr;

	QCheckBox *m_soundEnabled = nullptr;
	QLineEdit *m_soundPath = nullptr;
	QPushButton *m_browse = nullptr;
	QPushButton *m_preview = nullptr;
	QSpinBox *m_soundRepeat = nullptr;
	QCheckBox *m_taskbarFlash = nullptr;

	QSpinBox *m_pollMs = nullptr;
	QSpinBox *m_windowSeconds = nullptr;
	QSpinBox *m_triggerSamples = nullptr;
	QSpinBox *m_clearSeconds = nullptr;
	QCheckBox *m_onlyWhenActive = nullptr;

	QPushButton *m_test = nullptr;
};
