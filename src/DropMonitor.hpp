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

#include <QObject>
#include <QString>
#include <QTimer>

#include <cstdint>
#include <deque>

enum class DropKind {
	None,
	Network,
	Render,
	Encoder,
	Record,
	Disk,

	OutputError,
	StreamDropped,
	Stall,
};

bool isEventKind(DropKind kind);

struct DropStatus {
	bool active = false;
	DropKind kind = DropKind::None;
	double value = 0.0;

	QString detail;
	bool isTest = false;

	QString title() const;
	QString cause() const;
	QString hint() const;

	QString text() const;
};

class DropMonitor : public QObject {
	Q_OBJECT

public:
	explicit DropMonitor(QObject *parent = nullptr);

	void start();
	void stop();

	void applySettings();

	const DropStatus &status() const { return m_status; }

signals:
	void alarmStarted(const DropStatus &status);
	void alarmUpdated(const DropStatus &status);
	void alarmCleared();

private slots:
	void poll();

private:
	struct Sample {
		uint64_t timeNs = 0;
		int64_t netDropped = 0;
		int64_t netTotal = 0;
		int64_t recDropped = 0;
		int64_t recTotal = 0;
		uint32_t lagged = 0;
		uint32_t rendered = 0;
		uint32_t skipped = 0;
		uint32_t encoded = 0;
	};

	static Sample takeSample();

	const Sample *windowStart() const;

	bool detectCounterReset(const Sample &now) const;

	void evaluate(const Sample &now);
	void setAlarm(bool on, DropKind kind, double value, const QString &detail = QString());

	bool checkEvents(uint64_t nowNs, uint64_t lateMs);
	bool holdingEvent(uint64_t nowNs) const;
	static void readOutputErrors(QString &streamError, QString &recordError);

	QTimer m_timer;
	std::deque<Sample> m_samples;
	uint64_t m_windowNs = 5ull * 1000000000ull;

	bool m_testActive = false;
	int m_overCount = 0;
	uint64_t m_lastOverNs = 0;

	uint64_t m_lastPollNs = 0;
	uint64_t m_lastAwakeMs = 0;

	uint64_t m_holdUntilNs = 0;

	QString m_lastStreamError;
	QString m_lastRecordError;

	DropStatus m_status;

public:

	void fireTestAlarm();
	void clearTestAlarm();
};
