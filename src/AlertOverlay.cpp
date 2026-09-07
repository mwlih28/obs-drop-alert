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
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRect>

#include <algorithm>
#include <cmath>

namespace {

constexpr int kPulseIntervalMs = 16;
constexpr int kSyncIntervalMs = 1000;
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

void AlertOverlay::setStatus(const DropStatus &status)
{
	m_title = status.title();
	m_cause = status.cause();
	m_hint = status.hint();
	m_severity = status.severity;
	m_history = status.history;
	m_droppedFrames = status.droppedFrames;
	m_totalFrames = status.totalFrames;
	update();
}

void AlertOverlay::setStatusText(const QString &title, const QString &cause, const QString &hint,
				 AlertSeverity severity)
{
	if (m_title == title && m_cause == cause && m_hint == hint && m_severity == severity)
		return;
	m_title = title;
	m_cause = cause;
	m_hint = hint;
	m_severity = severity;
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
	p.setRenderHint(QPainter::Antialiasing, true);

	const double swing = s.pulse ? (0.38 + 0.62 * (0.5 + 0.5 * std::sin(m_phase))) : 1.0;
	const bool isWarn = (m_severity == AlertSeverity::Warning);

	// Temaya göre renk paleti belirleme
	QColor baseColor, glowColor, darkCardBase;
	switch (s.theme) {
	case HudTheme::Esports:
		baseColor = isWarn ? QColor(255, 150, 0) : QColor(235, 20, 45);
		glowColor = isWarn ? QColor(255, 185, 20) : QColor(255, 50, 50);
		darkCardBase = QColor(14, 15, 18, 244);
		break;
	case HudTheme::Amber:
		baseColor = isWarn ? QColor(255, 180, 10) : QColor(255, 105, 0);
		glowColor = isWarn ? QColor(255, 215, 50) : QColor(255, 140, 20);
		darkCardBase = QColor(24, 18, 10, 242);
		break;
	case HudTheme::Stealth:
		baseColor = isWarn ? QColor(210, 170, 70) : QColor(195, 55, 65);
		glowColor = isWarn ? QColor(230, 190, 90) : QColor(220, 85, 95);
		darkCardBase = QColor(18, 20, 22, 246);
		break;
	case HudTheme::Cyberpunk:
	default:
		baseColor = isWarn ? QColor(255, 170, 15) : QColor(255, 30, 75);
		glowColor = isWarn ? QColor(255, 205, 30) : QColor(255, 55, 125);
		darkCardBase = isWarn ? QColor(24, 18, 12, 240) : QColor(20, 10, 20, 240);
		break;
	}

	if (s.visualMode == VisualMode::Tint) {
		QColor wash = baseColor;
		wash.setAlphaF((float)(s.tintOpacity * swing * 0.85));
		p.fillRect(rect(), wash);
	}

	const int w = std::min(s.borderWidth, std::min(width(), height()) / 4);

	// 1. Neon Glow Efekti
	if (s.modernGlow && w > 3) {
		const int glowSteps = 4;
		for (int i = glowSteps; i >= 1; --i) {
			const int extra = (w * i) / 2;
			QColor g = glowColor;
			g.setAlphaF((float)std::min(0.25, (0.04 * (glowSteps - i + 1)) * swing));
			p.setPen(QPen(g, (qreal)extra));
			p.setBrush(Qt::NoBrush);
			p.drawRect(QRectF(extra / 2.0, extra / 2.0, width() - extra, height() - extra));
		}
	}

	// 2. Ana Çerçeve
	QColor border = baseColor;
	border.setAlphaF((float)std::min(1.0, 0.45 + 0.55 * swing));
	p.setPen(Qt::NoPen);
	p.setBrush(border);
	p.drawRect(0, 0, width(), w);
	p.drawRect(0, height() - w, width(), w);
	p.drawRect(0, w, w, height() - 2 * w);
	p.drawRect(width() - w, w, w, height() - 2 * w);

	// 3. Fütüristik HUD Köşe Braketleri
	if (s.hudCorners && width() > 160 && height() > 160) {
		const int cornerLen = std::min(55, std::min(width(), height()) / 6);
		const int cornerThick = std::max(w + 3, 7);

		QColor cColor = glowColor;
		cColor.setAlphaF((float)std::min(1.0, 0.70 + 0.30 * swing));
		p.setPen(Qt::NoPen);
		p.setBrush(cColor);

		p.drawRect(0, 0, cornerLen, cornerThick);
		p.drawRect(0, 0, cornerThick, cornerLen);

		p.drawRect(width() - cornerLen, 0, cornerLen, cornerThick);
		p.drawRect(width() - cornerThick, 0, cornerThick, cornerLen);

		p.drawRect(0, height() - cornerThick, cornerLen, cornerThick);
		p.drawRect(0, height() - cornerLen, cornerThick, cornerLen);

		p.drawRect(width() - cornerLen, height() - cornerThick, cornerLen, cornerThick);
		p.drawRect(width() - cornerThick, height() - cornerLen, cornerThick, cornerLen);
	}

	if (m_title.isEmpty())
		return;

	// 4. Teşhis Kartı
	QFont titleFont = p.font();
	titleFont.setBold(true);
	const double fontCeiling = std::max(12.0, (double)height() / 24.0);
	titleFont.setPointSizeF(std::min(std::max(13.5, (double)s.borderWidth * 1.08), fontCeiling));

	QFont badgeFont = p.font();
	badgeFont.setBold(true);
	badgeFont.setPointSizeF(std::max(8.0, titleFont.pointSizeF() * 0.60));

	QFont bodyFont = p.font();
	bodyFont.setBold(false);
	bodyFont.setPointSizeF(std::min(std::max(9.5, (double)s.borderWidth * 0.68), fontCeiling * 0.65));

	QFont statsFont = p.font();
	statsFont.setBold(true);
	statsFont.setPointSizeF(std::max(7.5, titleFont.pointSizeF() * 0.55));

	const QFontMetrics titleFm(titleFont);
	const QFontMetrics badgeFm(badgeFont);
	const QFontMetrics bodyFm(bodyFont);

	const int padX = 26;
	const int padY = 18;
	const int gap = 7;
	const int maxCardW = std::min(width() - 2 * w - 30, 840);
	if (maxCardW < 140)
		return;

	const int textW = maxCardW - 2 * padX - 44;

	const int flags = Qt::TextWordWrap | Qt::AlignLeft;
	const QRect bound(0, 0, textW, height() / 2);

	const QString badgeText = isWarn ? QString::fromUtf8("⚠ PERFORMANS RİSKİ") : QString::fromUtf8("⚡ KRİTİK DROP");
	const QRect badgeR = badgeFm.boundingRect(bound, flags, badgeText);
	const QRect titleR = titleFm.boundingRect(bound, flags, m_title);
	const QRect causeR = m_cause.isEmpty() ? QRect() : bodyFm.boundingRect(bound, flags, m_cause);
	const QRect hintR = m_hint.isEmpty() ? QRect() : bodyFm.boundingRect(bound, flags, m_hint);

	// Sparkline ve istatistik alanı yüksekliği
	const bool hasSparkline = s.showSparkline && m_history.size() >= 2;
	const int sparkH = hasSparkline ? 34 : 0;

	int cardH = 2 * padY + badgeR.height() + 4 + titleR.height();
	if (!m_cause.isEmpty())
		cardH += gap + causeR.height();
	if (!m_hint.isEmpty())
		cardH += gap + hintR.height();
	if (hasSparkline)
		cardH += gap + sparkH;

	const int maxCardH = std::max(50, height() - 2 * w - 20);
	cardH = std::min(cardH, maxCardH);

	const int contentW = std::max(titleR.width(), std::max(causeR.width(), hintR.width()));
	const int cardW = std::min(maxCardW, contentW + 2 * padX + 54);
	const QRect card((width() - cardW) / 2, w + 14, cardW, cardH);

	// Kart Arka Planı (Derin Cam + Degrade)
	QLinearGradient cardGrad(card.topLeft(), card.bottomLeft());
	QColor gradTop = darkCardBase;
	gradTop.setAlpha(246);
	QColor gradBottom = darkCardBase;
	gradBottom.setAlpha(228);
	cardGrad.setColorAt(0.0, gradTop);
	cardGrad.setColorAt(1.0, gradBottom);

	QPainterPath cardPath;
	cardPath.addRoundedRect(card, 12.0, 12.0);
	p.fillPath(cardPath, cardGrad);

	// Kart İnce Neon Sınırı
	QColor cardBorderColor = glowColor;
	cardBorderColor.setAlphaF((float)std::min(1.0, 0.40 + 0.45 * swing));
	p.setPen(QPen(cardBorderColor, 1.5));
	p.setBrush(Qt::NoBrush);
	p.drawPath(cardPath);

	p.save();
	p.setClipRect(card);

	int y = card.top() + padY;
	const int iconBoxSize = 36;
	const int iconX = card.left() + padX;
	const int textX = iconX + iconBoxSize + 14;
	const int lineW = card.right() - padX - textX;

	// Sol Durum İkonu
	const QRect iconRect(iconX, y + 4, iconBoxSize, iconBoxSize);
	QPainterPath iconBg;
	iconBg.addRoundedRect(iconRect, 8.0, 8.0);
	QColor iconBgColor = baseColor;
	iconBgColor.setAlphaF((float)(0.20 + 0.15 * swing));
	p.fillPath(iconBg, iconBgColor);
	p.setPen(QPen(glowColor, 1.2));
	p.drawPath(iconBg);

	QFont iconFont = p.font();
	iconFont.setBold(true);
	iconFont.setPointSizeF(16.0);
	p.setFont(iconFont);
	p.setPen(glowColor);
	p.drawText(iconRect, Qt::AlignCenter, isWarn ? QString::fromUtf8("▲") : QString::fromUtf8("⚡"));

	// Üst Kategori Rozeti (Badge Chip)
	const QRect badgeBox(textX, y, badgeR.width() + 14, badgeR.height() + 4);
	QPainterPath badgePath;
	badgePath.addRoundedRect(badgeBox, 4.0, 4.0);
	QColor chipBg = baseColor;
	chipBg.setAlphaF(0.28f);
	p.fillPath(badgePath, chipBg);

	p.setFont(badgeFont);
	p.setPen(glowColor);
	p.drawText(badgeBox, Qt::AlignCenter, badgeText);

	// Canlı Sayaç (Frame Stats Pill)
	if (m_totalFrames > 0) {
		const QString statStr = QString::fromUtf8("Kare Kaybı: %1 / %2").arg(m_droppedFrames).arg(m_totalFrames);
		const QFontMetrics statFm(statsFont);
		const int statW = statFm.horizontalAdvance(statStr) + 12;
		const QRect statBox(card.right() - padX - statW, y, statW, badgeBox.height());

		QPainterPath statPath;
		statPath.addRoundedRect(statBox, 4.0, 4.0);
		p.fillPath(statPath, QColor(0, 0, 0, 140));
		p.setPen(QPen(QColor(255, 255, 255, 120), 1.0));
		p.drawPath(statPath);

		p.setFont(statsFont);
		p.setPen(QColor(240, 240, 240));
		p.drawText(statBox, Qt::AlignCenter, statStr);
	}

	y += badgeBox.height() + 5;

	// Başlık
	p.setFont(titleFont);
	p.setPen(QColor(255, 255, 255));
	p.drawText(QRect(textX, y, lineW, titleR.height()), flags, m_title);
	y += titleR.height();

	// Sebep
	p.setFont(bodyFont);
	if (!m_cause.isEmpty()) {
		y += gap;
		p.setPen(isWarn ? QColor(255, 235, 200) : QColor(255, 215, 215));
		p.drawText(QRect(textX, y, lineW, causeR.height()), flags, m_cause);
		y += causeR.height();
	}

	// İpucu / Çözüm
	if (!m_hint.isEmpty()) {
		y += gap;
		p.setPen(isWarn ? QColor(255, 210, 140) : QColor(255, 185, 195));
		p.drawText(QRect(textX, y, lineW, hintR.height()), flags, m_hint);
		y += hintR.height();
	}

	// 5. Canlı Sparkline Geçmiş Grafiği
	if (hasSparkline) {
		y += gap + 2;
		const int sparkW = card.right() - padX - textX;
		const QRect sparkArea(textX, y, sparkW, sparkH);

		double maxVal = 1.0;
		for (double v : m_history)
			if (v > maxVal)
				maxVal = v;

		QPainterPath sparkLine;
		QPainterPath sparkFill;
		const int n = (int)m_history.size();
		const double stepX = (double)sparkW / std::max(1, n - 1);

		for (int i = 0; i < n; ++i) {
			const double norm = std::clamp(m_history[i] / maxVal, 0.0, 1.0);
			const double ptX = sparkArea.left() + i * stepX;
			const double ptY = sparkArea.bottom() - norm * (sparkH - 6) - 2;

			if (i == 0) {
				sparkLine.moveTo(ptX, ptY);
				sparkFill.moveTo(ptX, sparkArea.bottom());
				sparkFill.lineTo(ptX, ptY);
			} else {
				sparkLine.lineTo(ptX, ptY);
				sparkFill.lineTo(ptX, ptY);
			}
		}

		if (n > 1) {
			sparkFill.lineTo(sparkArea.right(), sparkArea.bottom());
			sparkFill.closeSubpath();

			QLinearGradient fillGrad(sparkArea.topLeft(), sparkArea.bottomLeft());
			QColor fillTop = glowColor;
			fillTop.setAlphaF(0.25f);
			QColor fillBottom = baseColor;
			fillBottom.setAlphaF(0.02f);
			fillGrad.setColorAt(0.0, fillTop);
			fillGrad.setColorAt(1.0, fillBottom);
			p.fillPath(sparkFill, fillGrad);

			p.setPen(QPen(glowColor, 1.8));
			p.setBrush(Qt::NoBrush);
			p.drawPath(sparkLine);

			// Son noktada parıldayan nokta
			const double lastNorm = std::clamp(m_history.back() / maxVal, 0.0, 1.0);
			const QPointF lastPt(sparkArea.right(), sparkArea.bottom() - lastNorm * (sparkH - 6) - 2);
			p.setPen(Qt::NoPen);
			p.setBrush(QColor(255, 255, 255));
			p.drawEllipse(lastPt, 3.0, 3.0);
			p.setBrush(QColor(glowColor.red(), glowColor.green(), glowColor.blue(), 100));
			p.drawEllipse(lastPt, 6.0, 6.0);
		}
	}

	// 6. Kart Altındaki Canlı Neon Nabız Çizgisi
	const int barH = 3;
	const QRect barRect(card.left() + 16, card.bottom() - barH - 4, card.width() - 32, barH);
	QLinearGradient barGrad(barRect.topLeft(), barRect.topRight());
	QColor barColor1 = baseColor;
	barColor1.setAlphaF(0.1f);
	QColor barColor2 = glowColor;
	barColor2.setAlphaF((float)std::min(1.0, 0.55 + 0.45 * swing));
	const double waveOffset = 0.5 + 0.4 * std::sin(m_phase);
	barGrad.setColorAt(0.0, barColor1);
	barGrad.setColorAt(std::clamp(waveOffset, 0.1, 0.9), barColor2);
	barGrad.setColorAt(1.0, barColor1);
	p.fillRect(barRect, barGrad);

	p.restore();
}
