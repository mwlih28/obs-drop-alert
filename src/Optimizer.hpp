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

#include <QString>

struct OptimizationResult {
	bool prioritySuccess = false;
	bool timerSuccess = false;
	QString details;
};

class Optimizer {
public:
	static void applyAutoOptimizations();
	static void cleanup();

	static bool setHighPriority(bool enable);
	static bool setMultimediaTimer(bool enable);

	static OptimizationResult runManualOptimization();
	static QString getStatusSummary();
};
