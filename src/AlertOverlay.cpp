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

/* Nabız animasyonunun kare aralığı (~30 fps). */
constexpr int kPulseIntervalMs = 33;

/* Ana pencere taşınırken olay gelmese bile geometriyi tazeleyen güvenlik ağı. */
constexpr int kSyncIntervalMs = 250;

constexpr double kTwoPi = 6.283185307179586;

} // namespace

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

void AlertOverlay::setStatusText(const QString &text)
{
	if (m_statusText == text)
		return;
	m_statusText = text;
	if (isVisible())
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

	/* Ana pencerenin istemci alanı, ekran koordinatlarında. */
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

	/* Nabız açıkken 0.35–1.0 arasında salınır, kapalıyken sabit tam yoğunluk. */
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

	if (m_statusText.isEmpty())
		return;

	/* Sebep metnini üst kenarın ortasında bir kırmızı rozette göster: kullanıcı
	 * sadece "bir şey kırmızı" değil, hangi sorunun olduğunu da görsün. */
	QFont font = p.font();
	font.setBold(true);
	font.setPointSizeF(std::max(11.0, (double)s.borderWidth * 0.95));
	p.setFont(font);

	const QFontMetrics fm(font);
	const int padX = 18;
	const int padY = 8;
	const QSize textSize = fm.size(Qt::TextSingleLine, m_statusText);
	const int badgeW = std::min(width() - 2 * w, textSize.width() + 2 * padX);
	const int badgeH = textSize.height() + 2 * padY;
	const QRect badge((width() - badgeW) / 2, w, badgeW, badgeH);

	QColor badgeColor(190, 0, 0);
	badgeColor.setAlphaF(std::min(1.0, 0.70 + 0.30 * swing));

	QPainterPath path;
	path.addRoundedRect(badge, badgeH / 4.0, badgeH / 4.0);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.fillPath(path, badgeColor);

	p.setPen(QColor(255, 255, 255));
	p.drawText(badge, Qt::AlignCenter, fm.elidedText(m_statusText, Qt::ElideRight, badgeW - 2 * padX));
}
