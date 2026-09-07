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

#include "Optimizer.hpp"
#include "Settings.hpp"

#include <obs-module.h>
#include <plugin-support.h>
#include <QStringList>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>

namespace {

static bool s_multimediaTimerActive = false;
static bool s_priorityBoostActive = false;

} // namespace

void Optimizer::applyAutoOptimizations()
{
	const Settings &s = settings();

	if (s.autoHighPriority) {
		setHighPriority(true);
	}

	if (s.autoMultimediaTimer) {
		setMultimediaTimer(true);
	}
}

void Optimizer::cleanup()
{
	if (s_multimediaTimerActive) {
		setMultimediaTimer(false);
	}
	if (s_priorityBoostActive) {
		setHighPriority(false);
	}
}

bool Optimizer::setHighPriority(bool enable)
{
	HANDLE process = GetCurrentProcess();
	DWORD priorityClass = enable ? HIGH_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS;

	if (SetPriorityClass(process, priorityClass)) {
		s_priorityBoostActive = enable;
		obs_log(LOG_INFO, "Optimizer: Process priority set to %s", enable ? "HIGH" : "NORMAL");
		return true;
	} else {
		DWORD err = GetLastError();
		obs_log(LOG_WARNING, "Optimizer: Failed to set process priority (error %lu)", err);
		return false;
	}
}

bool Optimizer::setMultimediaTimer(bool enable)
{
	if (enable) {
		if (!s_multimediaTimerActive) {
			MMRESULT res = timeBeginPeriod(1);
			if (res == TIMERR_NOERROR) {
				s_multimediaTimerActive = true;
				obs_log(LOG_INFO, "Optimizer: Windows multimedia timer set to 1ms (smooth frames)");
				return true;
			} else {
				obs_log(LOG_WARNING, "Optimizer: timeBeginPeriod(1) failed");
				return false;
			}
		}
		return true;
	} else {
		if (s_multimediaTimerActive) {
			timeEndPeriod(1);
			s_multimediaTimerActive = false;
			obs_log(LOG_INFO, "Optimizer: Windows multimedia timer restored");
			return true;
		}
		return true;
	}
}

OptimizationResult Optimizer::runManualOptimization()
{
	OptimizationResult res;
	res.prioritySuccess = setHighPriority(true);
	res.timerSuccess = setMultimediaTimer(true);

	QStringList details;
	if (res.prioritySuccess) {
		details << QString::fromUtf8(
			"✔ OBS İşlem Önceliği Yüksek (HIGH) seviyeye çıkarıldı (Oyun altında FPS kaybı ve gecikmeler önlendi).");
	} else {
		details << QString::fromUtf8("✖ OBS İşlem Önceliği artırılamadı (Yetki yetersiz).");
	}

	if (res.timerSuccess) {
		details << QString::fromUtf8(
			"✔ Windows 1ms Hassas Multimedya Zamanlayıcısı devrede (Zamanlama jitter'ı ve takılmalar engellendi).");
	} else {
		details << QString::fromUtf8("✖ 1ms Zamanlayıcı etkinleştirilemedi.");
	}

	res.details = details.join("\n");
	return res;
}

QString Optimizer::getStatusSummary()
{
	QStringList items;
	items << QString("Process Priority: %1").arg(s_priorityBoostActive ? "HIGH" : "NORMAL");
	items << QString("Timer Resolution: %1").arg(s_multimediaTimerActive ? "1ms (Optimized)" : "Default");
	return items.join(" | ");
}
