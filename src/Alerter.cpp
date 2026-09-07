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

#include "Alerter.hpp"
#include "Settings.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include <QFile>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#include <thread>

Alerter::Alerter(QObject *parent) : QObject(parent)
{
	m_repeatTimer.setTimerType(Qt::CoarseTimer);
	connect(&m_repeatTimer, &QTimer::timeout, this, &Alerter::onRepeatTick);
}

Alerter::~Alerter()
{
	stopAlarm();
}

QString Alerter::resolveSoundPath(AlertSeverity severity) const
{
	const Settings &s = settings();

	if (severity == AlertSeverity::Warning) {
		if (!s.warnSoundPath.empty()) {
			const QString custom = QString::fromStdString(s.warnSoundPath);
			if (QFile::exists(custom))
				return custom;
		}
		// Warning için özel dosya yoksa ve synth isteniyorsa boş dönüp syntha düşecek
		return QString();
	}

	if (!s.soundPath.empty()) {
		const QString custom = QString::fromStdString(s.soundPath);
		if (QFile::exists(custom))
			return custom;
		obs_log(LOG_WARNING, "sound file not found: '%s', falling back to bundled sound", s.soundPath.c_str());
	}

	char *bundled = obs_module_file("alert.wav");
	if (!bundled)
		return QString();

	const QString path = QString::fromUtf8(bundled);
	bfree(bundled);
	return path;
}

void Alerter::playBuiltInTone(AlertSeverity severity)
{
	// Non-blocking arka plan iş parçacığında pürüzsüz melodik ton
	std::thread([severity]() {
		if (severity == AlertSeverity::Warning) {
			Beep(659, 100);
			Sleep(30);
			Beep(880, 130);
		} else {
			Beep(880, 110);
			Sleep(25);
			Beep(587, 110);
			Sleep(25);
			Beep(880, 160);
		}
	}).detach();
}

void Alerter::playSound(AlertSeverity severity)
{
	const Settings &s = settings();

	if (severity == AlertSeverity::Warning) {
		if (!s.warnSoundEnabled)
			return;
	} else {
		if (!s.soundEnabled)
			return;
	}

	const QString path = resolveSoundPath(severity);

	if (!path.isEmpty()) {
		const std::wstring wide = path.toStdWString();
		if (PlaySoundW(wide.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT))
			return;
		obs_log(LOG_WARNING, "PlaySound failed for '%s'", path.toUtf8().constData());
	}

	if (s.builtInSynth) {
		playBuiltInTone(severity);
		return;
	}

	MessageBeep(severity == AlertSeverity::Warning ? MB_ICONWARNING : MB_ICONHAND);
}

void Alerter::setTaskbarFlash(bool on)
{
	HWND hwnd = (HWND)obs_frontend_get_main_window_handle();
	if (!hwnd)
		return;

	FLASHWINFO info = {};
	info.cbSize = sizeof(info);
	info.hwnd = hwnd;

	info.dwFlags = on ? (FLASHW_TRAY | FLASHW_TIMERNOFG) : FLASHW_STOP;
	info.uCount = 0;
	info.dwTimeout = 0;

	FlashWindowEx(&info);
}

void Alerter::startAlarm(AlertSeverity severity)
{
	m_currentSeverity = severity;
	const Settings &s = settings();

	playSound(severity);

	if (s.taskbarFlash)
		setTaskbarFlash(true);

	const bool canRepeat = (severity == AlertSeverity::Critical) && s.soundEnabled && (s.soundRepeatSeconds > 0);
	if (canRepeat) {
		m_repeatTimer.setInterval(s.soundRepeatSeconds * 1000);
		m_repeatTimer.start();
	} else {
		m_repeatTimer.stop();
	}
}

void Alerter::stopAlarm()
{
	m_repeatTimer.stop();
	PlaySoundW(nullptr, nullptr, SND_PURGE);
	setTaskbarFlash(false);
}

void Alerter::applySettings()
{
	const Settings &s = settings();

	if (!m_repeatTimer.isActive())
		return;

	if (s.soundEnabled && s.soundRepeatSeconds > 0)
		m_repeatTimer.setInterval(s.soundRepeatSeconds * 1000);
	else
		m_repeatTimer.stop();
}

void Alerter::previewSound()
{
	playSound(AlertSeverity::Critical);
}

void Alerter::previewWarnSound()
{
	playSound(AlertSeverity::Warning);
}

void Alerter::onRepeatTick()
{
	playSound(m_currentSeverity);
}
