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

#include "DropMonitor.hpp"
#include "Settings.hpp"

#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <plugin-support.h>

#include <QFileInfo>

#include <algorithm>

namespace {

constexpr uint64_t kNsPerSecond = 1000000000ull;
constexpr double kBytesPerGb = 1024.0 * 1024.0 * 1024.0;

/* delta_dropped / delta_total olarak yuzde; payda sifirsa 0. */
double ratePercent(int64_t droppedDelta, int64_t totalDelta)
{
	if (totalDelta <= 0 || droppedDelta <= 0)
		return 0.0;
	return (double)droppedDelta / (double)totalDelta * 100.0;
}

/*
 * Kayit ciktisinin yazdigi klasoru bulur. OBS'in kayit ayarlari profile gore
 * "path" (tek dosya) veya "directory" (bolunmus kayit) anahtarinda tutulur.
 * Kayit aktif degilse yol bilinemez; bu durumda bos donulur ve disk kontrolu
 * sessizce atlanir.
 */
QString recordingDirectory()
{
	obs_output_t *out = obs_frontend_get_recording_output();
	if (!out)
		return QString();

	QString dir;
	obs_data_t *s = obs_output_get_settings(out);
	if (s) {
		const char *path = obs_data_get_string(s, "path");
		if (path && *path) {
			dir = QFileInfo(QString::fromUtf8(path)).absolutePath();
		} else {
			const char *directory = obs_data_get_string(s, "directory");
			if (directory && *directory)
				dir = QString::fromUtf8(directory);
		}
		obs_data_release(s);
	}

	obs_output_release(out);
	return dir;
}

} // namespace

bool isEventKind(DropKind kind)
{
	return kind == DropKind::OutputError || kind == DropKind::StreamDropped || kind == DropKind::Stall;
}

namespace {

/* Her tur icin locale anahtar kokunu dondurur: "Alert.Network" gibi. */
const char *kindKey(DropKind kind)
{
	switch (kind) {
	case DropKind::Network:
		return "Alert.Network";
	case DropKind::Render:
		return "Alert.Render";
	case DropKind::Encoder:
		return "Alert.Encoder";
	case DropKind::Record:
		return "Alert.Record";
	case DropKind::Disk:
		return "Alert.Disk";
	case DropKind::OutputError:
		return "Alert.OutputError";
	case DropKind::StreamDropped:
		return "Alert.StreamDropped";
	case DropKind::Stall:
		return "Alert.Stall";
	default:
		return "Alert.None";
	}
}

QString kindString(DropKind kind, const char *suffix)
{
	const QString key = QString("%1.%2").arg(kindKey(kind)).arg(suffix);
	return QString::fromUtf8(obs_module_text(key.toUtf8().constData()));
}

/* Olcum degerini turune gore bicimler: yuzde, GB ya da saniye. */
QString formatValue(DropKind kind, double value)
{
	switch (kind) {
	case DropKind::Disk:
		return QString("%1 GB").arg(value, 0, 'f', 2);
	case DropKind::Stall:
		return QString("%1 s").arg(value, 0, 'f', 1);
	case DropKind::OutputError:
	case DropKind::StreamDropped:
	case DropKind::None:
		return QString();
	default:
		/* Yuzde isaretinin yeri dile gore degisiyor: tr "%42.00", en "42.00%". */
		return QString::fromUtf8(obs_module_text("Format.Percent")).arg(value, 0, 'f', 2);
	}
}

} // namespace

QString DropStatus::title() const
{
	const QString name = kindString(kind, "Title");
	const QString measured = formatValue(kind, value);
	return measured.isEmpty() ? name : QString("%1 %2").arg(name, measured);
}

QString DropStatus::cause() const
{
	/* Takilma suresi cumlenin icinde geciyor, o yuzden %1 ile yerlestiriliyor. */
	if (kind == DropKind::Stall)
		return kindString(kind, "Cause").arg(value, 0, 'f', 1);
	return kindString(kind, "Cause");
}

QString DropStatus::hint() const
{
	/* Kodlama hatasinda OBS'in kendi mesaji ipucundan daha degerli: onu goster. */
	if (kind == DropKind::OutputError && !detail.isEmpty())
		return detail;
	return kindString(kind, "Hint");
}

QString DropStatus::text() const
{
	if (kind == DropKind::OutputError && !detail.isEmpty())
		return QString("%1 - %2").arg(title(), detail);
	return title();
}

DropMonitor::DropMonitor(QObject *parent) : QObject(parent)
{
	m_timer.setTimerType(Qt::CoarseTimer);
	connect(&m_timer, &QTimer::timeout, this, &DropMonitor::poll);
	applySettings();
}

void DropMonitor::applySettings()
{
	const Settings &s = settings();
	m_windowNs = (uint64_t)s.windowSeconds * kNsPerSecond;
	m_timer.setInterval(s.pollMs);
	m_samples.clear();
	m_overCount = 0;
}

void DropMonitor::start()
{
	m_samples.clear();
	m_overCount = 0;
	m_holdUntilNs = 0;
	m_lastPollNs = 0;
	m_lastError.clear();
	m_timer.start();
}

void DropMonitor::stop()
{
	m_timer.stop();
	if (m_status.active)
		setAlarm(false, DropKind::None, 0.0);
	m_samples.clear();
	m_overCount = 0;
}

DropMonitor::Sample DropMonitor::takeSample()
{
	Sample s;
	s.timeNs = os_gettime_ns();

	s.lagged = obs_get_lagged_frames();
	s.rendered = obs_get_total_frames();

	if (video_t *video = obs_get_video()) {
		s.skipped = video_output_get_skipped_frames(video);
		s.encoded = video_output_get_total_frames(video);
	}

	/* Bu iki cagri yeni bir referans dondurur; release edilmezse cikti sizar. */
	if (obs_output_t *out = obs_frontend_get_streaming_output()) {
		s.netDropped = obs_output_get_frames_dropped(out);
		s.netTotal = obs_output_get_total_frames(out);
		obs_output_release(out);
	}

	if (obs_output_t *out = obs_frontend_get_recording_output()) {
		s.recDropped = obs_output_get_frames_dropped(out);
		s.recTotal = obs_output_get_total_frames(out);
		obs_output_release(out);
	}

	return s;
}

bool DropMonitor::detectCounterReset(const Sample &now) const
{
	if (m_samples.empty())
		return false;

	const Sample &prev = m_samples.back();
	/* Sayaclar monotondur; kuculduyse cikti yeniden baslamistir. */
	return now.netTotal < prev.netTotal || now.netDropped < prev.netDropped || now.recTotal < prev.recTotal ||
	       now.recDropped < prev.recDropped || now.rendered < prev.rendered || now.lagged < prev.lagged ||
	       now.encoded < prev.encoded || now.skipped < prev.skipped;
}

const DropMonitor::Sample *DropMonitor::windowStart() const
{
	if (m_samples.size() < 2)
		return nullptr;
	return &m_samples.front();
}

void DropMonitor::poll()
{
	/* Zamanlayici OBS'in ana is parcacigindan atiyor: arayuz donarsa bu tik de
	 * gecikir. Gecikme miktari dogrudan "OBS takildi" olcumumuz. */
	const uint64_t nowNs = os_gettime_ns();
	const uint64_t lateMs = m_lastPollNs ? (nowNs - m_lastPollNs) / 1000000ull : 0;
	m_lastPollNs = nowNs;

	/* Test alarmi surerken olcum yapma. Yoksa asagidaki "yayin/kayit yokken
	 * uyarma" dali test alarmini gercek alarm sanip ilk tikta sondurur. */
	if (m_testActive)
		return;

	const Settings &s = settings();

	if (s.onlyWhenActive && !obs_frontend_streaming_active() && !obs_frontend_recording_active()) {
		if (m_status.active)
			setAlarm(false, DropKind::None, 0.0);
		m_samples.clear();
		m_overCount = 0;
		m_holdUntilNs = 0;
		m_lastError.clear();
		return;
	}

	/* Olaylar esik olcumunden once ve onun onunde: kodlama hatasi ya da kopmus
	 * bir yayin, yuzde kac kare dustugunden cok daha onemli. */
	if (checkEvents(nowNs, lateMs))
		return;

	Sample now = takeSample();

	if (detectCounterReset(now)) {
		m_samples.clear();
		m_overCount = 0;
	}

	m_samples.push_back(now);

	/* Pencereden tasan ornekleri at, ama en az iki ornek kalsin. */
	while (m_samples.size() > 2 && now.timeNs - m_samples.front().timeNs > m_windowNs)
		m_samples.pop_front();

	evaluate(now);
}

void DropMonitor::evaluate(const Sample &now)
{
	const Settings &s = settings();

	DropKind worstKind = DropKind::None;
	double worstSeverity = 0.0; /* esige gore kac kat asildi */
	double worstValue = 0.0;

	auto consider = [&](DropKind kind, double value, double threshold) {
		if (threshold <= 0.0 || value < threshold)
			return;
		const double severity = value / threshold;
		if (severity > worstSeverity) {
			worstSeverity = severity;
			worstKind = kind;
			worstValue = value;
		}
	};

	if (const Sample *base = windowStart()) {
		if (s.monitorNetwork) {
			const double pct = ratePercent(now.netDropped - base->netDropped, now.netTotal - base->netTotal);
			consider(DropKind::Network, pct, s.thresholdNetworkPct);
		}
		if (s.monitorRender) {
			const double pct = ratePercent((int64_t)now.lagged - (int64_t)base->lagged,
						       (int64_t)now.rendered - (int64_t)base->rendered);
			consider(DropKind::Render, pct, s.thresholdRenderPct);
		}
		if (s.monitorEncoder) {
			const double pct = ratePercent((int64_t)now.skipped - (int64_t)base->skipped,
						       (int64_t)now.encoded - (int64_t)base->encoded);
			consider(DropKind::Encoder, pct, s.thresholdEncoderPct);
		}
		if (s.monitorDisk) {
			const double pct = ratePercent(now.recDropped - base->recDropped, now.recTotal - base->recTotal);
			consider(DropKind::Record, pct, s.thresholdRecordPct);
		}
	}

	/* Disk bos alani oran degil mutlak bir esik: kalan alan esigin ALTINA
	 * dusunce alarm verir, dolayisiyla consider() yerine ayri ele alinir. */
	if (s.monitorDisk) {
		const QString dir = recordingDirectory();
		if (!dir.isEmpty()) {
			const uint64_t freeBytes = os_get_free_disk_space(dir.toUtf8().constData());
			const double freeGb = (double)freeBytes / kBytesPerGb;
			if (freeBytes > 0 && freeGb < s.thresholdDiskGb) {
				const double severity = s.thresholdDiskGb / std::max(freeGb, 0.01);
				if (severity > worstSeverity) {
					worstSeverity = severity;
					worstKind = DropKind::Disk;
					worstValue = freeGb;
				}
			}
		}
	}

	const bool over = worstKind != DropKind::None;

	if (over) {
		m_overCount++;
		m_lastOverNs = now.timeNs;

		if (!m_status.active && m_overCount >= s.triggerSamples) {
			setAlarm(true, worstKind, worstValue);
		} else if (m_status.active) {
			/* Alarm surerken sebep degisebilir (or. encoder'dan aga gecis). */
			m_status.kind = worstKind;
			m_status.value = worstValue;
			emit alarmUpdated(m_status);
		}
	} else {
		m_overCount = 0;
		if (m_status.active) {
			const uint64_t clearNs = (uint64_t)s.clearSeconds * kNsPerSecond;
			if (now.timeNs - m_lastOverNs >= clearNs)
				setAlarm(false, DropKind::None, 0.0);
		}
	}
}

/* Yayin ve kayit ciktilarindan hata metnini / yeniden baglanma durumunu okur. */
bool DropMonitor::checkEvents(uint64_t nowNs, uint64_t lateMs)
{
	const Settings &s = settings();

	QString error;
	bool reconnecting = false;

	if (obs_output_t *out = obs_frontend_get_streaming_output()) {
		reconnecting = obs_output_reconnecting(out);
		if (const char *e = obs_output_get_last_error(out))
			error = QString::fromUtf8(e);
		obs_output_release(out);
	}
	if (error.isEmpty()) {
		if (obs_output_t *out = obs_frontend_get_recording_output()) {
			if (const char *e = obs_output_get_last_error(out))
				error = QString::fromUtf8(e);
			obs_output_release(out);
		}
	}

	const uint64_t holdNs = (uint64_t)s.eventHoldSeconds * kNsPerSecond;

	/* Hata metni ayni kaldigi surece tekrar alarm calmiyoruz; yalnizca degisince. */
	if (s.monitorOutputError && !error.isEmpty() && error != m_lastError) {
		m_lastError = error;
		m_holdUntilNs = nowNs + holdNs;
		setAlarm(true, DropKind::OutputError, 0.0, error);
		return true;
	}

	if (s.monitorStreamDrop && reconnecting) {
		m_holdUntilNs = nowNs + holdNs;
		setAlarm(true, DropKind::StreamDropped, 0.0);
		return true;
	}

	if (s.monitorStall && lateMs >= (uint64_t)s.stallMs) {
		m_holdUntilNs = nowNs + holdNs;
		setAlarm(true, DropKind::Stall, (double)lateMs / 1000.0);
		return true;
	}

	/* Olay gecti ama tutma suresi dolmadi: ekranda kalsin, esik olcumu araya girmesin. */
	if (isEventKind(m_status.kind) && m_status.active) {
		if (nowNs < m_holdUntilNs)
			return true;
		m_holdUntilNs = 0;
		setAlarm(false, DropKind::None, 0.0);
	}

	return false;
}

void DropMonitor::setAlarm(bool on, DropKind kind, double value, const QString &detail)
{
	if (m_status.active == on && m_status.kind == kind && m_status.detail == detail)
		return;

	m_status.active = on;
	m_status.kind = kind;
	m_status.value = value;
	m_status.detail = detail;

	if (on) {
		obs_log(LOG_WARNING, "drop alarm ON (%s)", m_status.text().toUtf8().constData());
		emit alarmStarted(m_status);
	} else {
		obs_log(LOG_INFO, "drop alarm OFF");
		emit alarmCleared();
	}
}

void DropMonitor::fireTestAlarm()
{
	m_testActive = true;
	m_status.active = true;
	m_status.kind = DropKind::Network;
	m_status.value = 42.0;
	m_status.detail.clear();
	obs_log(LOG_INFO, "test alarm ON");
	emit alarmStarted(m_status);
}

void DropMonitor::clearTestAlarm()
{
	m_testActive = false;
	m_status.active = false;
	m_status.kind = DropKind::None;
	m_status.value = 0.0;
	m_status.detail.clear();
	m_overCount = 0;
	m_samples.clear();
	obs_log(LOG_INFO, "test alarm OFF");
	emit alarmCleared();
}
