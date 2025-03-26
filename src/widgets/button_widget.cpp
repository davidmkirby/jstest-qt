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

ButtonWidget::ButtonWidget(int width, int height, const QString& name_, QWidget* parent)
    : QWidget(parent),
      name(name_),
      down(false)
{
    setFixedSize(width, height);
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
    
    painter.setPen(Qt::black);
    painter.drawRect(0, 0, w, h);
    
    if (down) {
        painter.setBrush(Qt::black);
        painter.drawRect(0, 0, w, h);
        painter.setPen(Qt::white);
    }
    
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
