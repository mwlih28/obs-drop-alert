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

#include <QObject>
#include <QString>
#include <QTimer>

/*
 * Görsel olmayan uyarılar: uyarı sesi ve Windows görev çubuğu flaşı.
 * OBS başka bir pencerenin arkasındayken tek fark edilebilir sinyal bunlardır.
 */
class Alerter : public QObject {
	Q_OBJECT

public:
	explicit Alerter(QObject *parent = nullptr);
	~Alerter() override;

	void startAlarm();
	void stopAlarm();
	void applySettings();

	/* Ayarlar diyaloğundaki ses önizlemesi için. */
	void previewSound();

private slots:
	void onRepeatTick();

private:
	void playSound();
	void setTaskbarFlash(bool on);

	/* Çalınacak dosya: ayarlardaki yol, boşsa eklentiyle gelen data/alert.wav. */
	QString resolveSoundPath() const;

	QTimer m_repeatTimer;
};
