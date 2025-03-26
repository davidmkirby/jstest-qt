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
#include <QPainterPath>
#include <sstream>
#include <iomanip>

AxisWidget::AxisWidget(int width, int height, bool show_values_, QWidget* parent)
    : QWidget(parent),
      x(0), y(0), raw_x(0), raw_y(0), show_values(show_values_)
{
    setFixedSize(width, height);
    
    // Set widget attributes for better rendering on Wayland
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_TranslucentBackground, false);
    
    // Use a proper size policy
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
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
    
    // Outer Rectangle - use QPainterPath for better Qt6 rendering
    QPainterPath rectPath;
    rectPath.addRect(0, 0, w, h);
    painter.setPen(Qt::black);
    painter.drawPath(rectPath);
    
    // BG Circle - use QPainterPath for better Qt6 rendering
    QPainterPath circlePath;
    circlePath.addEllipse(w/2 - w/2, h/2 - w/2, w, w);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(0, 0, 0, 25)));
    painter.drawPath(circlePath);
    
    // Cross - keep simple line drawing for these thin lines
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
        
        // Use system font for better scaling on HiDPI displays
        QFont monospaceFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        monospaceFont.setPointSize(10);
        painter.setFont(monospaceFont);
        
        QFontMetrics fm(monospaceFont);
        QString valueString = QString::fromStdString(value_text.str());
        QRect textRect = fm.boundingRect(valueString);
        
        // Text background for better readability
        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(QColor(255, 255, 255, 179)));
        
        // Use QPainterPath for text background
        QPainterPath textBgPath;
        textBgPath.addRect(w/2 - textRect.width()/2 - 2, 
                         h - textRect.height() - 4,
                         textRect.width() + 4,
                         textRect.height() + 2);
        painter.drawPath(textBgPath);
        
        // Draw text - calculate position with Qt6-friendly approach
        painter.setPen(Qt::black);
        painter.drawText(QPointF(w/2 - textRect.width()/2, h - 4),
                         valueString);
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
