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

#include <QString>
#include <QTimer>
#include <QWidget>

/*
 * OBS ana penceresinin üzerinde duran, tıklamaları geçiren kırmızı uyarı katmanı.
 *
 * Neden ana pencerenin çocuğu bir widget değil de ayrı bir üst düzey pencere:
 * OBS'in önizleme alanı (OBSQTDisplay) native bir alt pencere kullanır, native
 * alt pencereler ise kardeş Qt widget'larının HER ZAMAN üstüne çizilir. Ana
 * pencerenin içine konan bir katman önizlemenin altında kalırdı. Bunun yerine
 * ana pencereye "sahipli" (owned) bir Qt::Tool penceresi kullanılıyor: her şeyin
 * üstünde çizilir, OBS'le birlikte gizlenir/gösterilir, başka uygulamaların
 * üstüne çıkmaz.
 */
class AlertOverlay : public QWidget {
	Q_OBJECT

public:
	explicit AlertOverlay(QWidget *mainWindow);
	~AlertOverlay() override;

	void startAlarm();
	void stopAlarm();

	/* Kenarlıkta gösterilecek sebep metni, örn. "Ağ drop'u %4.21". */
	/* Uyarı kartının üç satırı: ne oldu / neden / ne yapmalı. */
	void setStatusText(const QString &title, const QString &cause, const QString &hint);

	void applySettings();

protected:
	void paintEvent(QPaintEvent *event) override;
	bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
	void onPulseTick();
	void syncGeometry();

private:
	void updateVisibility();

	QWidget *m_mainWindow = nullptr;
	QTimer m_pulseTimer;
	QTimer m_syncTimer;
	QString m_title;
	QString m_cause;
	QString m_hint;
	double m_phase = 0.0;
	bool m_alarmActive = false;
};
