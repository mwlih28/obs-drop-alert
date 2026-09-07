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

#include "Settings.hpp"

#include <obs-module.h>
#include <util/platform.h>
#include <plugin-support.h>

#include <algorithm>

namespace {

Settings g_settings;

std::string configFilePath()
{
	char *dir = obs_module_config_path(nullptr);
	if (!dir)
		return {};

	os_mkdirs(dir);
	std::string path = std::string(dir) + "/config.json";
	bfree(dir);
	return path;
}

int clampInt(int value, int lo, int hi)
{
	return std::max(lo, std::min(hi, value));
}

double clampDouble(double value, double lo, double hi)
{
	return std::max(lo, std::min(hi, value));
}

}

Settings &settings()
{
	return g_settings;
}

void Settings::load()
{
	const std::string path = configFilePath();
	if (path.empty())
		return;

	obs_data_t *data = obs_data_create_from_json_file_safe(path.c_str(), "bak");
	if (!data) {
		obs_log(LOG_INFO, "no config found at '%s', using defaults", path.c_str());
		return;
	}

	obs_data_set_default_bool(data, "monitor_network", monitorNetwork);
	obs_data_set_default_bool(data, "monitor_render", monitorRender);
	obs_data_set_default_bool(data, "monitor_encoder", monitorEncoder);
	obs_data_set_default_bool(data, "monitor_disk", monitorDisk);
	obs_data_set_default_bool(data, "monitor_output_error", monitorOutputError);
	obs_data_set_default_bool(data, "monitor_stream_drop", monitorStreamDrop);
	obs_data_set_default_bool(data, "monitor_stall", monitorStall);
	obs_data_set_default_int(data, "stall_ms", stallMs);
	obs_data_set_default_int(data, "event_hold_seconds", eventHoldSeconds);
	obs_data_set_default_double(data, "threshold_network_pct", thresholdNetworkPct);
	obs_data_set_default_double(data, "threshold_render_pct", thresholdRenderPct);
	obs_data_set_default_double(data, "threshold_encoder_pct", thresholdEncoderPct);
	obs_data_set_default_double(data, "threshold_record_pct", thresholdRecordPct);
	obs_data_set_default_double(data, "threshold_disk_gb", thresholdDiskGb);
	obs_data_set_default_int(data, "poll_ms", pollMs);
	obs_data_set_default_int(data, "window_seconds", windowSeconds);
	obs_data_set_default_int(data, "trigger_samples", triggerSamples);
	obs_data_set_default_int(data, "clear_seconds", clearSeconds);
	obs_data_set_default_int(data, "visual_mode", (int)visualMode);
	obs_data_set_default_bool(data, "pulse", pulse);
	obs_data_set_default_double(data, "pulse_hz", pulseHz);
	obs_data_set_default_double(data, "tint_opacity", tintOpacity);
	obs_data_set_default_int(data, "border_width", borderWidth);
	obs_data_set_default_bool(data, "sound_enabled", soundEnabled);
	obs_data_set_default_string(data, "sound_path", soundPath.c_str());
	obs_data_set_default_int(data, "sound_repeat_seconds", soundRepeatSeconds);
	obs_data_set_default_bool(data, "taskbar_flash", taskbarFlash);
	obs_data_set_default_bool(data, "only_when_active", onlyWhenActive);

	monitorNetwork = obs_data_get_bool(data, "monitor_network");
	monitorRender = obs_data_get_bool(data, "monitor_render");
	monitorEncoder = obs_data_get_bool(data, "monitor_encoder");
	monitorDisk = obs_data_get_bool(data, "monitor_disk");
	monitorOutputError = obs_data_get_bool(data, "monitor_output_error");
	monitorStreamDrop = obs_data_get_bool(data, "monitor_stream_drop");
	monitorStall = obs_data_get_bool(data, "monitor_stall");

	thresholdNetworkPct = clampDouble(obs_data_get_double(data, "threshold_network_pct"), 0.1, 100.0);
	thresholdRenderPct = clampDouble(obs_data_get_double(data, "threshold_render_pct"), 0.1, 100.0);
	thresholdEncoderPct = clampDouble(obs_data_get_double(data, "threshold_encoder_pct"), 0.1, 100.0);
	thresholdRecordPct = clampDouble(obs_data_get_double(data, "threshold_record_pct"), 0.1, 100.0);
	thresholdDiskGb = clampDouble(obs_data_get_double(data, "threshold_disk_gb"), 0.1, 1000.0);

	pollMs = clampInt((int)obs_data_get_int(data, "poll_ms"), 50, 5000);
	windowSeconds = clampInt((int)obs_data_get_int(data, "window_seconds"), 1, 120);
	stallMs = clampInt((int)obs_data_get_int(data, "stall_ms"), 300, 30000);
	eventHoldSeconds = clampInt((int)obs_data_get_int(data, "event_hold_seconds"), 1, 120);
	triggerSamples = clampInt((int)obs_data_get_int(data, "trigger_samples"), 1, 100);
	clearSeconds = clampInt((int)obs_data_get_int(data, "clear_seconds"), 1, 120);

	visualMode = (VisualMode)clampInt((int)obs_data_get_int(data, "visual_mode"), 0, 2);
	pulse = obs_data_get_bool(data, "pulse");
	pulseHz = clampDouble(obs_data_get_double(data, "pulse_hz"), 0.2, 6.0);
	tintOpacity = clampDouble(obs_data_get_double(data, "tint_opacity"), 0.0, 0.85);
	borderWidth = clampInt((int)obs_data_get_int(data, "border_width"), 2, 80);

	soundEnabled = obs_data_get_bool(data, "sound_enabled");
	soundPath = obs_data_get_string(data, "sound_path");
	soundRepeatSeconds = clampInt((int)obs_data_get_int(data, "sound_repeat_seconds"), 0, 600);

	taskbarFlash = obs_data_get_bool(data, "taskbar_flash");
	onlyWhenActive = obs_data_get_bool(data, "only_when_active");

	obs_data_release(data);
	obs_log(LOG_INFO, "settings loaded from '%s'", path.c_str());
}

void Settings::save() const
{
	const std::string path = configFilePath();
	if (path.empty())
		return;

	obs_data_t *data = obs_data_create();

	obs_data_set_bool(data, "monitor_network", monitorNetwork);
	obs_data_set_bool(data, "monitor_render", monitorRender);
	obs_data_set_bool(data, "monitor_encoder", monitorEncoder);
	obs_data_set_bool(data, "monitor_disk", monitorDisk);
	obs_data_set_bool(data, "monitor_output_error", monitorOutputError);
	obs_data_set_bool(data, "monitor_stream_drop", monitorStreamDrop);
	obs_data_set_bool(data, "monitor_stall", monitorStall);
	obs_data_set_int(data, "stall_ms", stallMs);
	obs_data_set_int(data, "event_hold_seconds", eventHoldSeconds);
	obs_data_set_double(data, "threshold_network_pct", thresholdNetworkPct);
	obs_data_set_double(data, "threshold_render_pct", thresholdRenderPct);
	obs_data_set_double(data, "threshold_encoder_pct", thresholdEncoderPct);
	obs_data_set_double(data, "threshold_record_pct", thresholdRecordPct);
	obs_data_set_double(data, "threshold_disk_gb", thresholdDiskGb);
	obs_data_set_int(data, "poll_ms", pollMs);
	obs_data_set_int(data, "window_seconds", windowSeconds);
	obs_data_set_int(data, "trigger_samples", triggerSamples);
	obs_data_set_int(data, "clear_seconds", clearSeconds);
	obs_data_set_int(data, "visual_mode", (int)visualMode);
	obs_data_set_bool(data, "pulse", pulse);
	obs_data_set_double(data, "pulse_hz", pulseHz);
	obs_data_set_double(data, "tint_opacity", tintOpacity);
	obs_data_set_int(data, "border_width", borderWidth);
	obs_data_set_bool(data, "sound_enabled", soundEnabled);
	obs_data_set_string(data, "sound_path", soundPath.c_str());
	obs_data_set_int(data, "sound_repeat_seconds", soundRepeatSeconds);
	obs_data_set_bool(data, "taskbar_flash", taskbarFlash);
	obs_data_set_bool(data, "only_when_active", onlyWhenActive);

	if (!obs_data_save_json_safe(data, path.c_str(), "tmp", "bak"))
		obs_log(LOG_WARNING, "failed to save settings to '%s'", path.c_str());

	obs_data_release(data);
}
