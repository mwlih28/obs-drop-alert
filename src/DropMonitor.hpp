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
	Network, /* ağ yetersizliğinden düşen kareler (yayın çıktısı) */
	Render,  /* GPU yetişemediği için atlanan kareler */
	Encoder, /* encoder yetişemediği için atlanan kareler */
	Record,  /* kayıt çıktısının düşürdüğü kareler */
	Disk,    /* diskte boş alan azaldı */

	/* Aşağıdakiler yüzde eşiği değil, tek seferlik olaylardır: ölçüm penceresi
	 * yerine `eventHoldSeconds` boyunca ekranda tutulurlar. */
	OutputError,    /* OBS çıktı/kodlayıcı hatası verdi (kodlama hatası) */
	StreamDropped,  /* sunucu bağlantısı koptu, OBS yeniden bağlanıyor */
	Stall,          /* OBS yanıt vermedi ("bekleme modu" gibi takılma) */
};

/* Olay türleri eşik tabanlı olanlardan farklı yaşam döngüsüne sahip. */
bool isEventKind(DropKind kind);

struct DropStatus {
	bool active = false;
	DropKind kind = DropKind::None;
	double value = 0.0; /* yüzde, Disk için kalan GB, Stall için saniye */

	/* OBS'in kendi verdiği ham hata metni (yalnızca OutputError'da dolu). */
	QString detail;

	/* Uyarı üç parçaya ayrılıyor: kullanıcı ekrana bakınca yalnızca "bir şey
	 * kırmızı" değil, ne olduğunu, neden olduğunu ve ne yapacağını görsün. */
	QString title() const; /* NE oldu, örn. "Kodlama hatası" */
	QString cause() const; /* NEDEN, tek cümle */
	QString hint() const;  /* NE YAPMALI, tek cümle */

	/* Log ve dar yerler için tek satırlık özet, örn. "Ağ drop'u %4.2" */
	QString text() const;
};

/*
 * OBS'in istatistik sayaçlarını düzenli aralıkla okur ve kayan pencere üzerinden
 * anlık drop oranını hesaplar.
 *
 * OBS'in Stats penceresindeki yüzdeler kümülatiftir: iki saatlik bir yayında on
 * saniyelik bir drop patlaması toplam yüzdeyi zar zor kıpırdatır. Canlı uyarı için
 * bu işe yaramadığından burada son `windowSeconds` saniyedeki *artış* farkı
 * kullanılır. Eşiğin sınırında titremeyi önlemek için histerezis uygulanır:
 * alarm `triggerSamples` ardışık aşımdan sonra başlar, `clearSeconds` boyunca
 * temiz kalınca söner.
 */
class DropMonitor : public QObject {
	Q_OBJECT

public:
	explicit DropMonitor(QObject *parent = nullptr);

	void start();
	void stop();

	/* Ayarlar değiştiğinde çağrılır: zamanlayıcı periyodu ve pencere yeniden kurulur. */
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

	/* Sayaçların o anki değerini okur. */
	static Sample takeSample();

	/* Pencerenin en eski örneğini döndürür; yetersiz veri varsa nullptr. */
	const Sample *windowStart() const;

	/* Sayaç sıfırlanması (yayın/kayıt yeniden başladı) tespit edilirse tamponu boşaltır. */
	bool detectCounterReset(const Sample &now) const;

	void evaluate(const Sample &now);
	void setAlarm(bool on, DropKind kind, double value, const QString &detail = QString());

	/* Eşik ölçümünden önce çalışan olay tespiti (hata / kopma / takılma).
	 * Bir olay alarmı sürüyorsa true döner ve eşik değerlendirmesi atlanır. */
	bool checkEvents(uint64_t nowNs, uint64_t lateMs);

	QTimer m_timer;
	std::deque<Sample> m_samples;
	uint64_t m_windowNs = 5ull * 1000000000ull;

	/* Test alarmi elle acilip kapanir; izleme dongusu ona hic dokunmaz. */
	bool m_testActive = false;
	int m_overCount = 0;
	uint64_t m_lastOverNs = 0;

	/* Takılma tespiti: poll'un bir önceki ateşlenme anı. Zamanlayıcı OBS'in ana
	 * iş parçacığında olduğu için arayüz donduğunda geç ateşlenir. */
	uint64_t m_lastPollNs = 0;
	/* Olay alarmının en erken sönebileceği an. */
	uint64_t m_holdUntilNs = 0;
	/* Aynı hata metni için tekrar tekrar alarm çalmamak üzere son görülen hata. */
	QString m_lastError;

	DropStatus m_status;

public:
	/* Ayarlar diyaloğundaki "Test uyarısı" düğmesi için: gerçek drop beklemeden
	 * tüm uyarı zincirini tetikler. */
	void fireTestAlarm();
	void clearTestAlarm();
};
