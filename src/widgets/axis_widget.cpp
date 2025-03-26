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

#include "widgets/axis_widget.h"

#include <QPainter>
#include <sstream>
#include <iomanip>

AxisWidget::AxisWidget(int width, int height, bool show_values_, QWidget* parent)
    : QWidget(parent),
      x(0), y(0), raw_x(0), raw_y(0), show_values(show_values_)
{
    setFixedSize(width, height);
}

void
AxisWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width() - 10;
    int h = height() - 10;
    int px = w/2 + (w/2 * x);
    int py = h/2 + (h/2 * y);
    
    painter.translate(5, 5);
    
    // Outer Rectangle
    painter.setPen(Qt::black);
    painter.drawRect(0, 0, w, h);
    
    // BG Circle
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(0, 0, 0, 25)));
    painter.drawEllipse(w/2 - w/2, h/2 - w/2, w, w);
    
    // Cross
    painter.setPen(QPen(QColor(0, 0, 0, 128), 0.5));
    painter.drawLine(w/2, 0, w/2, h);
    painter.drawLine(0, h/2, w, h/2);
    
    // Cursor
    painter.setPen(QPen(Qt::black, 2.0));
    painter.drawLine(px, py-5, px, py+5);
    painter.drawLine(px-5, py, px+5, py);
    
    // Display values if enabled
    if (show_values) {
        std::ostringstream value_text;
        value_text << "X: " << std::setw(6) << raw_x << " Y: " << std::setw(6) << raw_y;
        
        QFont monospaceFont("Monospace");
        monospaceFont.setPointSize(10);
        painter.setFont(monospaceFont);
        
        QFontMetrics fm(monospaceFont);
        QRect textRect = fm.boundingRect(QString::fromStdString(value_text.str()));
        
        // Text background for better readability
        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(QColor(255, 255, 255, 179)));
        painter.drawRect(w/2 - textRect.width()/2 - 2, 
                         h - textRect.height() - 4,
                         textRect.width() + 4,
                         textRect.height() + 2);
        
        // Draw text
        painter.setPen(Qt::black);
        painter.drawText(w/2 - textRect.width()/2, h - 2, 
                        QString::fromStdString(value_text.str()));
    }
}

void
AxisWidget::setXAxis(double x_)
{
    x = x_;
    update();
}

void
AxisWidget::setYAxis(double y_)
{
    y = y_;
    update();
}

void
AxisWidget::setRawX(int raw_x_value)
{
    raw_x = raw_x_value;
    x = raw_x_value / 32767.0;
    update();
}

void
AxisWidget::setRawY(int raw_y_value)
{
    raw_y = raw_y_value;
    y = raw_y_value / 32767.0;
    update();
}
