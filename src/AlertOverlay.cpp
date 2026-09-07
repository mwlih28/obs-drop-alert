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

#include "AlertOverlay.hpp"
#include "Settings.hpp"

#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QRect>

#include <algorithm>
#include <cmath>

namespace {

constexpr int kPulseIntervalMs = 33;

constexpr int kSyncIntervalMs = 250;

constexpr double kTwoPi = 6.283185307179586;

}

AlertOverlay::AlertOverlay(QWidget *mainWindow)
	: QWidget(mainWindow, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowTransparentForInput |
				      Qt::NoDropShadowWindowHint),
	  m_mainWindow(mainWindow)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setAttribute(Qt::WA_ShowWithoutActivating);
	setAttribute(Qt::WA_NoSystemBackground);
	setFocusPolicy(Qt::NoFocus);

	m_pulseTimer.setTimerType(Qt::PreciseTimer);
	m_pulseTimer.setInterval(kPulseIntervalMs);
	connect(&m_pulseTimer, &QTimer::timeout, this, &AlertOverlay::onPulseTick);

	m_syncTimer.setTimerType(Qt::CoarseTimer);
	m_syncTimer.setInterval(kSyncIntervalMs);
	connect(&m_syncTimer, &QTimer::timeout, this, &AlertOverlay::syncGeometry);

	if (m_mainWindow)
		m_mainWindow->installEventFilter(this);

	hide();
}

AlertOverlay::~AlertOverlay()
{
	if (m_mainWindow)
		m_mainWindow->removeEventFilter(this);
}

void AlertOverlay::startAlarm()
{
	m_alarmActive = true;
	m_phase = 0.0;
	syncGeometry();
	updateVisibility();

	if (settings().pulse)
		m_pulseTimer.start();
	m_syncTimer.start();
}

void AlertOverlay::stopAlarm()
{
	m_alarmActive = false;
	m_pulseTimer.stop();
	m_syncTimer.stop();
	hide();
}

void AlertOverlay::setStatusText(const QString &title, const QString &cause, const QString &hint)
{
	if (m_title == title && m_cause == cause && m_hint == hint)
		return;
	m_title = title;
	m_cause = cause;
	m_hint = hint;
	update();
}

void AlertOverlay::applySettings()
{
	if (!m_alarmActive)
		return;

	if (settings().pulse) {
		if (!m_pulseTimer.isActive())
			m_pulseTimer.start();
	} else {
		m_pulseTimer.stop();
		m_phase = 0.0;
	}

	updateVisibility();
	update();
}

void AlertOverlay::updateVisibility()
{
	const bool shouldShow = m_alarmActive && settings().visualMode != VisualMode::Off && m_mainWindow &&
				m_mainWindow->isVisible() && !m_mainWindow->isMinimized();

	if (shouldShow) {
		if (!isVisible())
			show();
		raise();
		update();
	} else if (isVisible()) {
		hide();
	}
}

void AlertOverlay::syncGeometry()
{
	if (!m_mainWindow)
		return;

	const QRect target(m_mainWindow->mapToGlobal(QPoint(0, 0)), m_mainWindow->size());
	if (geometry() != target)
		setGeometry(target);

	updateVisibility();
}

void AlertOverlay::onPulseTick()
{
	m_phase += kTwoPi * settings().pulseHz * (kPulseIntervalMs / 1000.0);
	if (m_phase > kTwoPi)
		m_phase -= kTwoPi;
	update();
}

bool AlertOverlay::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == m_mainWindow) {
		switch (event->type()) {
		case QEvent::Move:
		case QEvent::Resize:
		case QEvent::Show:
		case QEvent::Hide:
		case QEvent::WindowStateChange:
		case QEvent::WindowActivate:
			if (m_alarmActive)
				syncGeometry();
			break;
		default:
			break;
		}
	}
	return QWidget::eventFilter(watched, event);
}

void AlertOverlay::paintEvent(QPaintEvent *)
{
	const Settings &s = settings();
	if (s.visualMode == VisualMode::Off)
		return;

	QPainter p(this);

	const double swing = s.pulse ? (0.35 + 0.65 * (0.5 + 0.5 * std::sin(m_phase))) : 1.0;

	if (s.visualMode == VisualMode::Tint) {
		QColor wash(200, 0, 0);
		wash.setAlphaF(s.tintOpacity * swing);
		p.fillRect(rect(), wash);
	}

	QColor border(255, 40, 40);
	border.setAlphaF(std::min(1.0, 0.45 + 0.55 * swing));

	const int w = std::min(s.borderWidth, std::min(width(), height()) / 2);
	p.setPen(Qt::NoPen);
	p.setBrush(border);
	p.drawRect(0, 0, width(), w);
	p.drawRect(0, height() - w, width(), w);
	p.drawRect(0, w, w, height() - 2 * w);
	p.drawRect(width() - w, w, w, height() - 2 * w);

	if (m_title.isEmpty())
		return;

	QFont titleFont = p.font();
	titleFont.setBold(true);
	titleFont.setPointSizeF(std::max(13.0, (double)s.borderWidth * 1.05));

	QFont bodyFont = p.font();
	bodyFont.setBold(false);
	bodyFont.setPointSizeF(std::max(9.5, (double)s.borderWidth * 0.70));

	const QFontMetrics titleFm(titleFont);
	const QFontMetrics bodyFm(bodyFont);

	const int padX = 22;
	const int padY = 14;
	const int gap = 6;
	const int maxCardW = std::min(width() - 2 * w - 40, 760);
	if (maxCardW < 120)
		return;
	const int textW = maxCardW - 2 * padX;

	const int flags = Qt::TextWordWrap | Qt::AlignLeft;
	const QRect bound(0, 0, textW, height() / 2);
	const QRect titleR = titleFm.boundingRect(bound, flags, m_title);
	const QRect causeR = m_cause.isEmpty() ? QRect() : bodyFm.boundingRect(bound, flags, m_cause);
	const QRect hintR = m_hint.isEmpty() ? QRect() : bodyFm.boundingRect(bound, flags, m_hint);

	int cardH = 2 * padY + titleR.height();
	if (!m_cause.isEmpty())
		cardH += gap + causeR.height();
	if (!m_hint.isEmpty())
		cardH += gap + hintR.height();

	const int contentW = std::max(titleR.width(), std::max(causeR.width(), hintR.width()));
	const int cardW = std::min(maxCardW, contentW + 2 * padX);
	const QRect card((width() - cardW) / 2, w + 10, cardW, cardH);

	QColor cardColor(150, 0, 0);
	cardColor.setAlphaF(std::min(1.0, 0.80 + 0.20 * swing));

	QPainterPath path;
	path.addRoundedRect(card, 10.0, 10.0);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.fillPath(path, cardColor);

	int y = card.top() + padY;
	const int x = card.left() + padX;
	const int lineW = card.width() - 2 * padX;

	p.setFont(titleFont);
	p.setPen(QColor(255, 255, 255));
	p.drawText(QRect(x, y, lineW, titleR.height()), flags, m_title);
	y += titleR.height();

	p.setFont(bodyFont);
	if (!m_cause.isEmpty()) {
		y += gap;
		p.setPen(QColor(255, 226, 226));
		p.drawText(QRect(x, y, lineW, causeR.height()), flags, m_cause);
		y += causeR.height();
	}
	if (!m_hint.isEmpty()) {
		y += gap;
		p.setPen(QColor(255, 196, 196));
		p.drawText(QRect(x, y, lineW, hintR.height()), flags, m_hint);
	}
}
