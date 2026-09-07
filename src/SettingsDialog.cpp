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

#include "SettingsDialog.hpp"
#include "Settings.hpp"
#include "Optimizer.hpp"

#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>

#include <algorithm>

#include <QGuiApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {

QString T(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

QDoubleSpinBox *makePercentBox(double min = 0.1, double max = 100.0)
{
	auto *box = new QDoubleSpinBox();
	box->setRange(min, max);
	box->setDecimals(1);
	box->setSingleStep(0.5);
	box->setSuffix(" %");
	return box;
}

}

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(T("Dialog.Title"));
	setModal(false);
	buildUi();
	loadFromSettings();
	updateEnabledStates();
	fitToScreen();

	connect(&m_diagTimer, &QTimer::timeout, this, &SettingsDialog::onDiagTick);
}

void SettingsDialog::fitToScreen()
{
	const QScreen *screen = parentWidget() ? parentWidget()->screen() : QGuiApplication::primaryScreen();
	if (!screen)
		return;

	const int maxHeight = (int)(screen->availableGeometry().height() * 0.85);
	resize(620, std::min(540, maxHeight));
	setMaximumHeight(screen->availableGeometry().height());
}

void SettingsDialog::buildUi()
{
	auto *outer = new QVBoxLayout(this);

	m_tabs = new QTabWidget(this);
	m_tabs->addTab(buildMonitorTab(), T("Tab.Monitor"));
	m_tabs->addTab(buildVisualTab(), T("Tab.Visual"));
	m_tabs->addTab(buildSoundTab(), T("Tab.Sound"));
	m_tabs->addTab(buildOptimizerTab(), T("Tab.Optimizer"));
	m_tabs->addTab(buildDiagnosticsTab(), T("Tab.Diagnostics"));

	outer->addWidget(m_tabs, 1);

	auto *buttonRow = new QHBoxLayout();
	m_test = new QPushButton(T("Button.Test"));
	m_test->setCheckable(true);
	m_test->setToolTip(T("Button.Test.Tip"));
	buttonRow->addWidget(m_test);
	buttonRow->addStretch(1);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel |
					     QDialogButtonBox::RestoreDefaults);
	buttonRow->addWidget(buttons);
	outer->addLayout(buttonRow);

	connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::onSave);
	connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
	connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this,
		&SettingsDialog::onRestoreDefaults);
	connect(m_test, &QPushButton::toggled, this, &SettingsDialog::onToggleTest);
}

QWidget *SettingsDialog::buildMonitorTab()
{
	auto *page = new QWidget();
	auto *layout = new QVBoxLayout(page);

	auto *monitorGroup = new QGroupBox(T("Group.Monitor"), page);
	auto *monitorGrid = new QGridLayout(monitorGroup);
	int row = 0;

	m_network = new QCheckBox(T("Monitor.Network"));
	m_network->setToolTip(T("Monitor.Network.Tip"));
	m_networkPct = makePercentBox();
	monitorGrid->addWidget(m_network, row, 0);
	monitorGrid->addWidget(new QLabel(T("Label.Threshold")), row, 1);
	monitorGrid->addWidget(m_networkPct, row++, 2);

	m_render = new QCheckBox(T("Monitor.Render"));
	m_render->setToolTip(T("Monitor.Render.Tip"));
	m_renderPct = makePercentBox();
	monitorGrid->addWidget(m_render, row, 0);
	monitorGrid->addWidget(new QLabel(T("Label.Threshold")), row, 1);
	monitorGrid->addWidget(m_renderPct, row++, 2);

	m_encoder = new QCheckBox(T("Monitor.Encoder"));
	m_encoder->setToolTip(T("Monitor.Encoder.Tip"));
	m_encoderPct = makePercentBox();
	monitorGrid->addWidget(m_encoder, row, 0);
	monitorGrid->addWidget(new QLabel(T("Label.Threshold")), row, 1);
	monitorGrid->addWidget(m_encoderPct, row++, 2);

	m_disk = new QCheckBox(T("Monitor.Disk"));
	m_disk->setToolTip(T("Monitor.Disk.Tip"));
	m_recordPct = makePercentBox();
	monitorGrid->addWidget(m_disk, row, 0);
	monitorGrid->addWidget(new QLabel(T("Label.RecordThreshold")), row, 1);
	monitorGrid->addWidget(m_recordPct, row++, 2);

	m_diskGb = new QDoubleSpinBox();
	m_diskGb->setRange(0.1, 1000.0);
	m_diskGb->setDecimals(1);
	m_diskGb->setSingleStep(0.5);
	m_diskGb->setSuffix(" GB");
	m_diskGb->setToolTip(T("Monitor.DiskFree.Tip"));
	monitorGrid->addWidget(new QLabel(T("Label.DiskThreshold")), row, 1);
	monitorGrid->addWidget(m_diskGb, row++, 2);

	m_outputError = new QCheckBox(T("Monitor.OutputError"));
	monitorGrid->addWidget(m_outputError, row++, 0, 1, 3);

	m_streamDrop = new QCheckBox(T("Monitor.StreamDrop"));
	monitorGrid->addWidget(m_streamDrop, row++, 0, 1, 3);

	m_stall = new QCheckBox(T("Monitor.Stall"));
	m_stallMs = new QSpinBox();
	m_stallMs->setRange(300, 30000);
	m_stallMs->setSingleStep(100);
	m_stallMs->setSuffix(" ms");
	monitorGrid->addWidget(m_stall, row, 0);
	monitorGrid->addWidget(new QLabel(T("Tuning.StallMs")), row, 1);
	monitorGrid->addWidget(m_stallMs, row++, 2);

	monitorGrid->setColumnStretch(0, 1);
	layout->addWidget(monitorGroup);

	auto *tuningGroup = new QGroupBox(T("Group.Tuning"), page);
	auto *tuningForm = new QFormLayout(tuningGroup);

	m_windowSeconds = new QSpinBox();
	m_windowSeconds->setRange(1, 120);
	m_windowSeconds->setSuffix(" s");
	m_windowSeconds->setToolTip(T("Tuning.Window.Tip"));
	tuningForm->addRow(T("Tuning.Window"), m_windowSeconds);

	m_pollMs = new QSpinBox();
	m_pollMs->setRange(50, 5000);
	m_pollMs->setSingleStep(50);
	m_pollMs->setSuffix(" ms");
	tuningForm->addRow(T("Tuning.Poll"), m_pollMs);

	m_triggerSamples = new QSpinBox();
	m_triggerSamples->setRange(1, 100);
	m_triggerSamples->setToolTip(T("Tuning.Trigger.Tip"));
	tuningForm->addRow(T("Tuning.Trigger"), m_triggerSamples);

	m_clearSeconds = new QSpinBox();
	m_clearSeconds->setRange(1, 120);
	m_clearSeconds->setSuffix(" s");
	m_clearSeconds->setToolTip(T("Tuning.Clear.Tip"));
	tuningForm->addRow(T("Tuning.Clear"), m_clearSeconds);

	m_eventHold = new QSpinBox();
	m_eventHold->setRange(1, 120);
	m_eventHold->setSuffix(" s");
	tuningForm->addRow(T("Tuning.EventHold"), m_eventHold);

	m_onlyWhenActive = new QCheckBox(T("Tuning.OnlyWhenActive"));
	tuningForm->addRow(m_onlyWhenActive);

	layout->addWidget(tuningGroup);
	layout->addStretch(1);

	connect(m_network, &QCheckBox::toggled, this, &SettingsDialog::updateEnabledStates);
	connect(m_render, &QCheckBox::toggled, this, &SettingsDialog::updateEnabledStates);
	connect(m_encoder, &QCheckBox::toggled, this, &SettingsDialog::updateEnabledStates);
	connect(m_disk, &QCheckBox::toggled, this, &SettingsDialog::updateEnabledStates);

	return page;
}

QWidget *SettingsDialog::buildVisualTab()
{
	auto *page = new QWidget();
	auto *layout = new QVBoxLayout(page);

	auto *visualGroup = new QGroupBox(T("Group.Visual"), page);
	auto *visualForm = new QFormLayout(visualGroup);

	m_visualMode = new QComboBox();
	m_visualMode->addItem(T("Visual.Off"), (int)VisualMode::Off);
	m_visualMode->addItem(T("Visual.Border"), (int)VisualMode::Border);
	m_visualMode->addItem(T("Visual.Tint"), (int)VisualMode::Tint);
	visualForm->addRow(T("Visual.Mode"), m_visualMode);

	m_theme = new QComboBox();
	m_theme->addItem(T("Theme.Cyberpunk"), (int)HudTheme::Cyberpunk);
	m_theme->addItem(T("Theme.Esports"), (int)HudTheme::Esports);
	m_theme->addItem(T("Theme.Amber"), (int)HudTheme::Amber);
	m_theme->addItem(T("Theme.Stealth"), (int)HudTheme::Stealth);
	visualForm->addRow(T("Visual.Theme"), m_theme);

	m_borderWidth = new QSpinBox();
	m_borderWidth->setRange(2, 80);
	m_borderWidth->setSuffix(" px");
	visualForm->addRow(T("Visual.BorderWidth"), m_borderWidth);

	m_tintOpacity = new QSpinBox();
	m_tintOpacity->setRange(0, 85);
	m_tintOpacity->setSuffix(" %");
	m_tintOpacity->setToolTip(T("Visual.TintOpacity.Tip"));
	visualForm->addRow(T("Visual.TintOpacity"), m_tintOpacity);

	m_pulse = new QCheckBox(T("Visual.Pulse"));
	visualForm->addRow(m_pulse);

	m_pulseHz = new QDoubleSpinBox();
	m_pulseHz->setRange(0.2, 6.0);
	m_pulseHz->setDecimals(1);
	m_pulseHz->setSingleStep(0.1);
	m_pulseHz->setSuffix(" Hz");
	visualForm->addRow(T("Visual.PulseSpeed"), m_pulseHz);

	m_modernGlow = new QCheckBox(T("Visual.ModernGlow"));
	visualForm->addRow(m_modernGlow);

	m_hudCorners = new QCheckBox(T("Visual.HudCorners"));
	visualForm->addRow(m_hudCorners);

	m_showSparkline = new QCheckBox(T("Visual.ShowSparkline"));
	m_showSparkline->setToolTip(T("Visual.ShowSparkline.Tip"));
	visualForm->addRow(m_showSparkline);

	layout->addWidget(visualGroup);
	layout->addStretch(1);

	connect(m_visualMode, &QComboBox::currentIndexChanged, this, &SettingsDialog::updateEnabledStates);
	connect(m_pulse, &QCheckBox::toggled, this, &SettingsDialog::updateEnabledStates);

	return page;
}

QWidget *SettingsDialog::buildSoundTab()
{
	auto *page = new QWidget();
	auto *layout = new QVBoxLayout(page);

	auto *alarmGroup = new QGroupBox(T("Group.AlarmSound"), page);
	auto *alarmForm = new QFormLayout(alarmGroup);

	m_soundEnabled = new QCheckBox(T("Sound.Enabled"));
	alarmForm->addRow(m_soundEnabled);

	auto *pathRow = new QHBoxLayout();
	m_soundPath = new QLineEdit();
	m_soundPath->setPlaceholderText(T("Sound.PathPlaceholder"));
	m_browse = new QPushButton(T("Button.Browse"));
	m_preview = new QPushButton(T("Button.Preview"));
	pathRow->addWidget(m_soundPath, 1);
	pathRow->addWidget(m_browse);
	pathRow->addWidget(m_preview);
	alarmForm->addRow(T("Sound.Path"), pathRow);

	m_soundRepeat = new QSpinBox();
	m_soundRepeat->setRange(0, 600);
	m_soundRepeat->setSuffix(" s");
	m_soundRepeat->setSpecialValueText(T("Sound.NoRepeat"));
	m_soundRepeat->setToolTip(T("Sound.Repeat.Tip"));
	alarmForm->addRow(T("Sound.Repeat"), m_soundRepeat);

	layout->addWidget(alarmGroup);

	auto *warnGroup = new QGroupBox(T("Group.WarnSound"), page);
	auto *warnForm = new QFormLayout(warnGroup);

	m_warnSoundEnabled = new QCheckBox(T("Sound.WarnEnabled"));
	warnForm->addRow(m_warnSoundEnabled);

	auto *warnPathRow = new QHBoxLayout();
	m_warnSoundPath = new QLineEdit();
	m_warnSoundPath->setPlaceholderText(T("Sound.WarnPathPlaceholder"));
	m_warnBrowse = new QPushButton(T("Button.Browse"));
	m_warnPreview = new QPushButton(T("Button.Preview"));
	warnPathRow->addWidget(m_warnSoundPath, 1);
	warnPathRow->addWidget(m_warnBrowse);
	warnPathRow->addWidget(m_warnPreview);
	warnForm->addRow(T("Sound.Path"), warnPathRow);

	layout->addWidget(warnGroup);

	auto *notifGroup = new QGroupBox(T("Group.NotificationExtra"), page);
	auto *notifForm = new QFormLayout(notifGroup);

	m_builtInSynth = new QCheckBox(T("Sound.BuiltInSynth"));
	m_builtInSynth->setToolTip(T("Sound.BuiltInSynth.Tip"));
	notifForm->addRow(m_builtInSynth);

	m_taskbarFlash = new QCheckBox(T("Sound.TaskbarFlash"));
	m_taskbarFlash->setToolTip(T("Sound.TaskbarFlash.Tip"));
	notifForm->addRow(m_taskbarFlash);

	layout->addWidget(notifGroup);
	layout->addStretch(1);

	connect(m_browse, &QPushButton::clicked, this, &SettingsDialog::onBrowseSound);
	connect(m_preview, &QPushButton::clicked, this, &SettingsDialog::soundPreviewRequested);
	connect(m_warnBrowse, &QPushButton::clicked, this, &SettingsDialog::onBrowseWarnSound);
	connect(m_warnPreview, &QPushButton::clicked, this, &SettingsDialog::warnSoundPreviewRequested);
	connect(m_soundEnabled, &QCheckBox::toggled, this, &SettingsDialog::updateEnabledStates);
	connect(m_warnSoundEnabled, &QCheckBox::toggled, this, &SettingsDialog::updateEnabledStates);

	return page;
}

QWidget *SettingsDialog::buildOptimizerTab()
{
	auto *page = new QWidget();
	auto *layout = new QVBoxLayout(page);

	auto *optimizerGroup = new QGroupBox(T("Group.Optimizer"), page);
	auto *optimizerForm = new QFormLayout(optimizerGroup);

	m_autoHighPriority = new QCheckBox(T("Optimizer.AutoHighPriority"));
	m_autoHighPriority->setToolTip(T("Optimizer.AutoHighPriority.Tip"));
	optimizerForm->addRow(m_autoHighPriority);

	m_autoMultimediaTimer = new QCheckBox(T("Optimizer.AutoMultimediaTimer"));
	m_autoMultimediaTimer->setToolTip(T("Optimizer.AutoMultimediaTimer.Tip"));
	optimizerForm->addRow(m_autoMultimediaTimer);

	m_enablePreWarning = new QCheckBox(T("Optimizer.EnablePreWarning"));
	m_enablePreWarning->setToolTip(T("Optimizer.EnablePreWarning.Tip"));
	optimizerForm->addRow(m_enablePreWarning);

	m_autoPausePreview = new QCheckBox(T("Optimizer.AutoPausePreview"));
	m_autoPausePreview->setToolTip(T("Optimizer.AutoPausePreview.Tip"));
	optimizerForm->addRow(m_autoPausePreview);

	m_optimizeNow = new QPushButton(T("Button.OptimizeNow"));
	m_optimizeNow->setToolTip(T("Button.OptimizeNow.Tip"));
	optimizerForm->addRow(m_optimizeNow);

	layout->addWidget(optimizerGroup);
	layout->addStretch(1);

	connect(m_optimizeNow, &QPushButton::clicked, this, &SettingsDialog::onRunOptimizer);

	return page;
}

QWidget *SettingsDialog::buildDiagnosticsTab()
{
	auto *page = new QWidget();
	auto *layout = new QVBoxLayout(page);

	auto *liveGroup = new QGroupBox(T("Group.LiveDiagnostics"), page);
	auto *form = new QFormLayout(liveGroup);

	m_diagFps = new QLabel("-");
	m_diagFps->setStyleSheet("font-weight: bold; color: #00E676;");
	form->addRow(T("Diag.Fps"), m_diagFps);

	m_diagDropped = new QLabel("-");
	m_diagDropped->setStyleSheet("font-weight: bold; color: #FF3D00;");
	form->addRow(T("Diag.Dropped"), m_diagDropped);

	m_diagLagged = new QLabel("-");
	m_diagLagged->setStyleSheet("font-weight: bold; color: #FF9100;");
	form->addRow(T("Diag.Lagged"), m_diagLagged);

	m_diagSkipped = new QLabel("-");
	m_diagSkipped->setStyleSheet("font-weight: bold; color: #FFD600;");
	form->addRow(T("Diag.Skipped"), m_diagSkipped);

	m_diagDisk = new QLabel("-");
	m_diagDisk->setStyleSheet("font-weight: bold; color: #00B0FF;");
	form->addRow(T("Diag.Disk"), m_diagDisk);

	m_diagOptimizerStatus = new QLabel("-");
	m_diagOptimizerStatus->setStyleSheet("font-weight: bold; color: #E040FB;");
	form->addRow(T("Diag.OptimizerStatus"), m_diagOptimizerStatus);

	layout->addWidget(liveGroup);
	layout->addStretch(1);

	return page;
}

void SettingsDialog::onDiagTick()
{
	// Canlı teşhis verilerini topla ve yaz
	const double fps = obs_get_active_fps();
	m_diagFps->setText(QString("%1 FPS").arg(fps, 0, 'f', 1));

	uint32_t lagged = obs_get_lagged_frames();
	uint32_t totalRendered = obs_get_total_frames();
	double lagPct = totalRendered > 0 ? ((double)lagged / totalRendered * 100.0) : 0.0;
	m_diagLagged->setText(QString("%1 / %2 (%3 %)").arg(lagged).arg(totalRendered).arg(lagPct, 0, 'f', 2));

	if (video_t *video = obs_get_video()) {
		uint32_t skipped = video_output_get_skipped_frames(video);
		uint32_t totalEncoded = video_output_get_total_frames(video);
		double skipPct = totalEncoded > 0 ? ((double)skipped / totalEncoded * 100.0) : 0.0;
		m_diagSkipped->setText(QString("%1 / %2 (%3 %)").arg(skipped).arg(totalEncoded).arg(skipPct, 0, 'f', 2));
	}

	if (obs_output_t *out = obs_frontend_get_streaming_output()) {
		int dropped = obs_output_get_frames_dropped(out);
		int total = obs_output_get_total_frames(out);
		double dropPct = total > 0 ? ((double)dropped / total * 100.0) : 0.0;
		m_diagDropped->setText(QString("%1 / %2 (%3 %)").arg(dropped).arg(total).arg(dropPct, 0, 'f', 2));
		obs_output_release(out);
	} else {
		m_diagDropped->setText(T("Diag.StreamInactive"));
	}

	m_diagOptimizerStatus->setText(Optimizer::getStatusSummary());
}

void SettingsDialog::loadFromSettings()
{
	const Settings &s = settings();

	m_network->setChecked(s.monitorNetwork);
	m_render->setChecked(s.monitorRender);
	m_encoder->setChecked(s.monitorEncoder);
	m_disk->setChecked(s.monitorDisk);
	m_outputError->setChecked(s.monitorOutputError);
	m_streamDrop->setChecked(s.monitorStreamDrop);
	m_stall->setChecked(s.monitorStall);
	m_stallMs->setValue(s.stallMs);
	m_networkPct->setValue(s.thresholdNetworkPct);
	m_renderPct->setValue(s.thresholdRenderPct);
	m_encoderPct->setValue(s.thresholdEncoderPct);
	m_recordPct->setValue(s.thresholdRecordPct);
	m_diskGb->setValue(s.thresholdDiskGb);

	m_pollMs->setValue(s.pollMs);
	m_windowSeconds->setValue(s.windowSeconds);
	m_triggerSamples->setValue(s.triggerSamples);
	m_clearSeconds->setValue(s.clearSeconds);
	m_eventHold->setValue(s.eventHoldSeconds);
	m_onlyWhenActive->setChecked(s.onlyWhenActive);

	m_visualMode->setCurrentIndex(m_visualMode->findData((int)s.visualMode));
	m_theme->setCurrentIndex(m_theme->findData((int)s.theme));
	m_pulse->setChecked(s.pulse);
	m_pulseHz->setValue(s.pulseHz);
	m_borderWidth->setValue(s.borderWidth);
	m_tintOpacity->setValue((int)(s.tintOpacity * 100.0 + 0.5));
	m_modernGlow->setChecked(s.modernGlow);
	m_hudCorners->setChecked(s.hudCorners);
	m_showSparkline->setChecked(s.showSparkline);

	m_soundEnabled->setChecked(s.soundEnabled);
	m_soundPath->setText(QString::fromStdString(s.soundPath));
	m_soundRepeat->setValue(s.soundRepeatSeconds);

	m_warnSoundEnabled->setChecked(s.warnSoundEnabled);
	m_warnSoundPath->setText(QString::fromStdString(s.warnSoundPath));
	m_builtInSynth->setChecked(s.builtInSynth);
	m_taskbarFlash->setChecked(s.taskbarFlash);

	m_autoHighPriority->setChecked(s.autoHighPriority);
	m_autoMultimediaTimer->setChecked(s.autoMultimediaTimer);
	m_enablePreWarning->setChecked(s.enablePreWarning);
	m_autoPausePreview->setChecked(s.autoPausePreviewOnRenderLag);
}

void SettingsDialog::storeToSettings()
{
	Settings &s = settings();

	s.monitorNetwork = m_network->isChecked();
	s.monitorRender = m_render->isChecked();
	s.monitorEncoder = m_encoder->isChecked();
	s.monitorDisk = m_disk->isChecked();
	s.monitorOutputError = m_outputError->isChecked();
	s.monitorStreamDrop = m_streamDrop->isChecked();
	s.monitorStall = m_stall->isChecked();
	s.stallMs = m_stallMs->value();
	s.thresholdNetworkPct = m_networkPct->value();
	s.thresholdRenderPct = m_renderPct->value();
	s.thresholdEncoderPct = m_encoderPct->value();
	s.thresholdRecordPct = m_recordPct->value();
	s.thresholdDiskGb = m_diskGb->value();

	s.pollMs = m_pollMs->value();
	s.windowSeconds = m_windowSeconds->value();
	s.triggerSamples = m_triggerSamples->value();
	s.clearSeconds = m_clearSeconds->value();
	s.eventHoldSeconds = m_eventHold->value();
	s.onlyWhenActive = m_onlyWhenActive->isChecked();

	s.visualMode = (VisualMode)m_visualMode->currentData().toInt();
	s.theme = (HudTheme)m_theme->currentData().toInt();
	s.pulse = m_pulse->isChecked();
	s.pulseHz = m_pulseHz->value();
	s.borderWidth = m_borderWidth->value();
	s.tintOpacity = m_tintOpacity->value() / 100.0;
	s.modernGlow = m_modernGlow->isChecked();
	s.hudCorners = m_hudCorners->isChecked();
	s.showSparkline = m_showSparkline->isChecked();

	s.soundEnabled = m_soundEnabled->isChecked();
	s.soundPath = m_soundPath->text().trimmed().toStdString();
	s.soundRepeatSeconds = m_soundRepeat->value();

	s.warnSoundEnabled = m_warnSoundEnabled->isChecked();
	s.warnSoundPath = m_warnSoundPath->text().trimmed().toStdString();
	s.builtInSynth = m_builtInSynth->isChecked();
	s.taskbarFlash = m_taskbarFlash->isChecked();

	s.autoHighPriority = m_autoHighPriority->isChecked();
	s.autoMultimediaTimer = m_autoMultimediaTimer->isChecked();
	s.enablePreWarning = m_enablePreWarning->isChecked();
	s.autoPausePreviewOnRenderLag = m_autoPausePreview->isChecked();
}

void SettingsDialog::updateEnabledStates()
{
	const bool tint = m_visualMode->currentData().toInt() == (int)VisualMode::Tint;
	const bool visual = m_visualMode->currentData().toInt() != (int)VisualMode::Off;

	m_theme->setEnabled(visual);
	m_borderWidth->setEnabled(visual);
	m_tintOpacity->setEnabled(tint);
	m_pulse->setEnabled(visual);
	m_pulseHz->setEnabled(visual && m_pulse->isChecked());
	m_modernGlow->setEnabled(visual);
	m_hudCorners->setEnabled(visual);
	m_showSparkline->setEnabled(visual);

	m_soundPath->setEnabled(m_soundEnabled->isChecked());
	m_browse->setEnabled(m_soundEnabled->isChecked());
	m_preview->setEnabled(m_soundEnabled->isChecked());
	m_soundRepeat->setEnabled(m_soundEnabled->isChecked());

	m_warnSoundPath->setEnabled(m_warnSoundEnabled->isChecked());
	m_warnBrowse->setEnabled(m_warnSoundEnabled->isChecked());
	m_warnPreview->setEnabled(m_warnSoundEnabled->isChecked());

	m_networkPct->setEnabled(m_network->isChecked());
	m_renderPct->setEnabled(m_render->isChecked());
	m_encoderPct->setEnabled(m_encoder->isChecked());
	m_recordPct->setEnabled(m_disk->isChecked());
	m_diskGb->setEnabled(m_disk->isChecked());
}

void SettingsDialog::onRunOptimizer()
{
	OptimizationResult res = Optimizer::runManualOptimization();
	QMessageBox::information(this, T("Dialog.OptimizeTitle"), res.details);
	onDiagTick();
}

void SettingsDialog::onSave()
{
	storeToSettings();
	settings().save();
	emit settingsApplied();
	accept();
}

void SettingsDialog::onRestoreDefaults()
{
	settings() = Settings();
	loadFromSettings();
	updateEnabledStates();

	if (m_test->isChecked())
		emit settingsApplied();
}

void SettingsDialog::onBrowseSound()
{
	const QString filter = T("Sound.Filter.Wav") + " (*.wav);;" + T("Sound.Filter.All") + " (*)";
	const QString file = QFileDialog::getOpenFileName(this, T("Sound.PickTitle"), m_soundPath->text(), filter);
	if (!file.isEmpty())
		m_soundPath->setText(file);
}

void SettingsDialog::onBrowseWarnSound()
{
	const QString filter = T("Sound.Filter.Wav") + " (*.wav);;" + T("Sound.Filter.All") + " (*)";
	const QString file = QFileDialog::getOpenFileName(this, T("Sound.PickTitle"), m_warnSoundPath->text(), filter);
	if (!file.isEmpty())
		m_warnSoundPath->setText(file);
}

void SettingsDialog::onToggleTest(bool checked)
{
	if (checked) {
		storeToSettings();
		emit settingsApplied();
	}
	emit testAlarmRequested(checked);
}

void SettingsDialog::setTestChecked(bool on)
{
	if (!m_test || m_test->isChecked() == on)
		return;
	const QSignalBlocker blocker(m_test);
	m_test->setChecked(on);
}

void SettingsDialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	loadFromSettings();
	updateEnabledStates();
	onDiagTick();
	m_diagTimer.start(1000);
}

void SettingsDialog::hideEvent(QHideEvent *event)
{
	m_diagTimer.stop();
	QDialog::hideEvent(event);
}

void SettingsDialog::done(int result)
{
	m_diagTimer.stop();
	if (m_test && m_test->isChecked())
		m_test->setChecked(false);

	if (result != QDialog::Accepted) {
		settings() = Settings();
		settings().load();
		emit settingsApplied();
	}

	QDialog::done(result);
}
