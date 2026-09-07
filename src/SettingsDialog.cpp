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

#include <obs-module.h>

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
#include <QPushButton>
#include <QSpinBox>
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
}

void SettingsDialog::buildUi()
{
	auto *root = new QVBoxLayout(this);

	auto *monitorGroup = new QGroupBox(T("Group.Monitor"), this);
	auto *monitorGrid = new QGridLayout(monitorGroup);
	int row = 0;

	m_network = new QCheckBox(T("Monitor.Network"));
	m_networkPct = makePercentBox();
	m_networkPct->setToolTip(T("Monitor.Network.Tip"));
	monitorGrid->addWidget(m_network, row, 0);
	monitorGrid->addWidget(new QLabel(T("Label.Threshold")), row, 1);
	monitorGrid->addWidget(m_networkPct, row++, 2);

	m_render = new QCheckBox(T("Monitor.Render"));
	m_renderPct = makePercentBox();
	m_renderPct->setToolTip(T("Monitor.Render.Tip"));
	monitorGrid->addWidget(m_render, row, 0);
	monitorGrid->addWidget(new QLabel(T("Label.Threshold")), row, 1);
	monitorGrid->addWidget(m_renderPct, row++, 2);

	m_encoder = new QCheckBox(T("Monitor.Encoder"));
	m_encoderPct = makePercentBox();
	m_encoderPct->setToolTip(T("Monitor.Encoder.Tip"));
	monitorGrid->addWidget(m_encoder, row, 0);
	monitorGrid->addWidget(new QLabel(T("Label.Threshold")), row, 1);
	monitorGrid->addWidget(m_encoderPct, row++, 2);

	m_disk = new QCheckBox(T("Monitor.Disk"));
	m_recordPct = makePercentBox();
	m_recordPct->setToolTip(T("Monitor.Disk.Tip"));
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
	root->addWidget(monitorGroup);

	auto *visualGroup = new QGroupBox(T("Group.Visual"), this);
	auto *visualForm = new QFormLayout(visualGroup);

	m_visualMode = new QComboBox();
	m_visualMode->addItem(T("Visual.Off"), (int)VisualMode::Off);
	m_visualMode->addItem(T("Visual.Border"), (int)VisualMode::Border);
	m_visualMode->addItem(T("Visual.Tint"), (int)VisualMode::Tint);
	visualForm->addRow(T("Visual.Mode"), m_visualMode);

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

	root->addWidget(visualGroup);

	auto *soundGroup = new QGroupBox(T("Group.Sound"), this);
	auto *soundForm = new QFormLayout(soundGroup);

	m_soundEnabled = new QCheckBox(T("Sound.Enabled"));
	soundForm->addRow(m_soundEnabled);

	auto *pathRow = new QHBoxLayout();
	m_soundPath = new QLineEdit();
	m_soundPath->setPlaceholderText(T("Sound.PathPlaceholder"));
	m_browse = new QPushButton(T("Button.Browse"));
	m_preview = new QPushButton(T("Button.Preview"));
	pathRow->addWidget(m_soundPath, 1);
	pathRow->addWidget(m_browse);
	pathRow->addWidget(m_preview);
	soundForm->addRow(T("Sound.Path"), pathRow);

	m_soundRepeat = new QSpinBox();
	m_soundRepeat->setRange(0, 600);
	m_soundRepeat->setSuffix(" s");
	m_soundRepeat->setSpecialValueText(T("Sound.NoRepeat"));
	m_soundRepeat->setToolTip(T("Sound.Repeat.Tip"));
	soundForm->addRow(T("Sound.Repeat"), m_soundRepeat);

	m_taskbarFlash = new QCheckBox(T("Sound.TaskbarFlash"));
	m_taskbarFlash->setToolTip(T("Sound.TaskbarFlash.Tip"));
	soundForm->addRow(m_taskbarFlash);

	root->addWidget(soundGroup);

	auto *tuningGroup = new QGroupBox(T("Group.Tuning"), this);
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

	root->addWidget(tuningGroup);

	auto *buttonRow = new QHBoxLayout();
	m_test = new QPushButton(T("Button.Test"));
	m_test->setCheckable(true);
	m_test->setToolTip(T("Button.Test.Tip"));
	buttonRow->addWidget(m_test);
	buttonRow->addStretch(1);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel |
					     QDialogButtonBox::RestoreDefaults);
	buttonRow->addWidget(buttons);
	root->addLayout(buttonRow);

	connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::onSave);
	connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
	connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this,
		&SettingsDialog::onRestoreDefaults);
	connect(m_browse, &QPushButton::clicked, this, &SettingsDialog::onBrowseSound);
	connect(m_preview, &QPushButton::clicked, this, &SettingsDialog::soundPreviewRequested);
	connect(m_test, &QPushButton::toggled, this, &SettingsDialog::onToggleTest);

	connect(m_visualMode, &QComboBox::currentIndexChanged, this, &SettingsDialog::updateEnabledStates);
	connect(m_pulse, &QCheckBox::toggled, this, &SettingsDialog::updateEnabledStates);
	connect(m_soundEnabled, &QCheckBox::toggled, this, &SettingsDialog::updateEnabledStates);
	connect(m_network, &QCheckBox::toggled, this, &SettingsDialog::updateEnabledStates);
	connect(m_render, &QCheckBox::toggled, this, &SettingsDialog::updateEnabledStates);
	connect(m_encoder, &QCheckBox::toggled, this, &SettingsDialog::updateEnabledStates);
	connect(m_disk, &QCheckBox::toggled, this, &SettingsDialog::updateEnabledStates);
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

	m_visualMode->setCurrentIndex(m_visualMode->findData((int)s.visualMode));
	m_pulse->setChecked(s.pulse);
	m_pulseHz->setValue(s.pulseHz);
	m_borderWidth->setValue(s.borderWidth);
	m_tintOpacity->setValue((int)(s.tintOpacity * 100.0 + 0.5));

	m_soundEnabled->setChecked(s.soundEnabled);
	m_soundPath->setText(QString::fromStdString(s.soundPath));
	m_soundRepeat->setValue(s.soundRepeatSeconds);
	m_taskbarFlash->setChecked(s.taskbarFlash);

	m_pollMs->setValue(s.pollMs);
	m_windowSeconds->setValue(s.windowSeconds);
	m_triggerSamples->setValue(s.triggerSamples);
	m_clearSeconds->setValue(s.clearSeconds);
	m_eventHold->setValue(s.eventHoldSeconds);
	m_onlyWhenActive->setChecked(s.onlyWhenActive);
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

	s.visualMode = (VisualMode)m_visualMode->currentData().toInt();
	s.pulse = m_pulse->isChecked();
	s.pulseHz = m_pulseHz->value();
	s.borderWidth = m_borderWidth->value();
	s.tintOpacity = m_tintOpacity->value() / 100.0;

	s.soundEnabled = m_soundEnabled->isChecked();
	s.soundPath = m_soundPath->text().trimmed().toStdString();
	s.soundRepeatSeconds = m_soundRepeat->value();
	s.taskbarFlash = m_taskbarFlash->isChecked();

	s.pollMs = m_pollMs->value();
	s.windowSeconds = m_windowSeconds->value();
	s.triggerSamples = m_triggerSamples->value();
	s.clearSeconds = m_clearSeconds->value();
	s.eventHoldSeconds = m_eventHold->value();
	s.onlyWhenActive = m_onlyWhenActive->isChecked();
}

void SettingsDialog::updateEnabledStates()
{
	const bool tint = m_visualMode->currentData().toInt() == (int)VisualMode::Tint;
	const bool visual = m_visualMode->currentData().toInt() != (int)VisualMode::Off;

	m_borderWidth->setEnabled(visual);
	m_tintOpacity->setEnabled(tint);
	m_pulse->setEnabled(visual);
	m_pulseHz->setEnabled(visual && m_pulse->isChecked());

	m_soundPath->setEnabled(m_soundEnabled->isChecked());
	m_browse->setEnabled(m_soundEnabled->isChecked());
	m_preview->setEnabled(m_soundEnabled->isChecked());
	m_soundRepeat->setEnabled(m_soundEnabled->isChecked());

	m_networkPct->setEnabled(m_network->isChecked());
	m_renderPct->setEnabled(m_render->isChecked());
	m_encoderPct->setEnabled(m_encoder->isChecked());
	m_recordPct->setEnabled(m_disk->isChecked());
	m_diskGb->setEnabled(m_disk->isChecked());
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

void SettingsDialog::done(int result)
{

	if (m_test && m_test->isChecked())
		m_test->setChecked(false);

	if (result != QDialog::Accepted) {
		settings() = Settings();
		settings().load();
		emit settingsApplied();
	}

	QDialog::done(result);
}
