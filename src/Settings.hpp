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

#include <string>

enum class VisualMode {
	Off = 0,    /* görsel uyarı yok */
	Border = 1, /* sadece kalın kırmızı kenarlık */
	Tint = 2,   /* kenarlık + tüm pencereye kırmızı yıkama */
};

/*
 * Eklentinin tüm ayarları. Diskte obs_module_config_path("config.json")
 * altında obs_data_t (JSON) olarak tutulur; OBS'in frontend config API'si
 * sürümler arasında değiştiği için bilinçli olarak kullanılmıyor.
 */
struct Settings {
	/* --- izlenecek sorunlar --- */
	bool monitorNetwork = true;
	bool monitorRender = true;
	bool monitorEncoder = true;
	bool monitorDisk = true;

	double thresholdNetworkPct = 2.0;
	double thresholdRenderPct = 5.0;
	double thresholdEncoderPct = 5.0;
	double thresholdRecordPct = 1.0;
	double thresholdDiskGb = 2.0;

	/* --- hassasiyet --- */
	int pollMs = 250;
	int windowSeconds = 5;
	int triggerSamples = 2;
	int clearSeconds = 3;

	/* --- görsel uyarı --- */
	VisualMode visualMode = VisualMode::Border;
	bool pulse = true;
	double pulseHz = 1.5;
	double tintOpacity = 0.22;
	int borderWidth = 12;

	/* --- ses --- */
	bool soundEnabled = true;
	std::string soundPath;      /* boş = eklentiyle gelen data/alert.wav */
	int soundRepeatSeconds = 0; /* 0 = alarm başında bir kez çal */

	/* --- diğer --- */
	bool taskbarFlash = true;
	bool onlyWhenActive = true;

	void load();
	void save() const;
};

/* Süreç boyunca tek örnek. */
Settings &settings();
