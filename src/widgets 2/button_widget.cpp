/*
**  jstest-qt - A Qt joystick tester
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmail.com>
**  Copyright (C) 2025 Qt port contributors
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "widgets/button_widget.h"

#include <QPainter>
#include <QPainterPath>

ButtonWidget::ButtonWidget(int width, int height, const QString& name_, QWidget* parent)
    : QWidget(parent),
      name(name_),
      down(false)
{
    setFixedSize(width, height);
    
    // Set widget attributes for better rendering on Wayland
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_TranslucentBackground, false);
    
    // Use a proper size policy
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void
ButtonWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width() - 10;
    int h = height() - 10;
    
    painter.translate(5, 5);
    
    // Create a path for the button rectangle
    QPainterPath rectPath;
    rectPath.addRect(0, 0, w, h);
    
    // Draw button outline
    painter.setPen(Qt::black);
    
    // Fix: Create proper QBrush object instead of using Qt::black directly
    if (down) {
        painter.setBrush(QBrush(Qt::black));
    } else {
        painter.setBrush(Qt::NoBrush);
    }
    
    painter.drawPath(rectPath);
    
    // Set text color based on button state
    painter.setPen(down ? Qt::white : Qt::black);
    
    // Use system font for better scaling on HiDPI displays
    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);
    
    // Center the text
    QFontMetrics fm = painter.fontMetrics();
    QRect textRect = fm.boundingRect(name);
    int textX = (w - textRect.width()) / 2;
    int textY = (h + textRect.height()) / 2 - fm.descent();
    
    painter.drawText(textX, textY, name);
}

void
ButtonWidget::setDown(bool t)
{
    down = t;
    update();
}
