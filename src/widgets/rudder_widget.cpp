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

#include "widgets/rudder_widget.h"

#include <QPainter>

RudderWidget::RudderWidget(int width, int height, QWidget* parent)
    : QWidget(parent),
      pos(0.0)
{
    setFixedSize(width, height);
}

void
RudderWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    double p = (pos + 1.0)/2.0;
    
    int w = width() - 10;
    int h = height() - 10;
    
    painter.translate(5, 5);
    
    // Outer Rectangle
    painter.setPen(Qt::black);
    painter.drawRect(0, 0, w, h);
    
    // Center line
    painter.setPen(QPen(QColor(0, 0, 0, 128), 0.5));
    painter.drawLine(w/2, 0, w/2, h);
    
    // Position indicator
    painter.setPen(QPen(Qt::black, 2.0));
    painter.drawLine(w * p, 0, w * p, h);
}

void
RudderWidget::setPos(double p)
{
    pos = p;
    update();
}
