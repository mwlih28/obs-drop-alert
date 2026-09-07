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

#include <string>

enum class VisualMode {
	Off = 0,
	Border = 1,
	Tint = 2,
};

struct Settings {

	bool monitorNetwork = true;
	bool monitorRender = true;
	bool monitorEncoder = true;
	bool monitorDisk = true;

	bool monitorOutputError = true;
	bool monitorStreamDrop = true;
	bool monitorStall = true;

	double thresholdNetworkPct = 2.0;
	double thresholdRenderPct = 5.0;
	double thresholdEncoderPct = 5.0;
	double thresholdRecordPct = 1.0;
	double thresholdDiskGb = 2.0;

	int pollMs = 250;
	int windowSeconds = 5;
	int triggerSamples = 2;
	int clearSeconds = 3;

	int stallMs = 1500;

	int eventHoldSeconds = 8;

	VisualMode visualMode = VisualMode::Border;
	bool pulse = true;
	double pulseHz = 1.5;
	double tintOpacity = 0.22;
	int borderWidth = 12;

	bool soundEnabled = true;
	std::string soundPath;
	int soundRepeatSeconds = 0;

	bool taskbarFlash = true;
	bool onlyWhenActive = true;

	void load();
	void save() const;
};

Settings &settings();
