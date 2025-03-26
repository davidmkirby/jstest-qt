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

#ifndef JSTEST_QT_AXIS_WIDGET_H
#define JSTEST_QT_AXIS_WIDGET_H

#include <QWidget>
#include <QFontDatabase>

class AxisWidget : public QWidget
{
    Q_OBJECT

private:
    double x;
    double y;
    int raw_x;  // Raw value from -32767 to 32767
    int raw_y;  // Raw value from -32767 to 32767
    bool show_values;  // Flag to control whether to show values or not

public:
    AxisWidget(int width, int height, bool show_values = true, QWidget* parent = nullptr);

    void paintEvent(QPaintEvent* event) override;

public slots:
    void setXAxis(double x);
    void setYAxis(double y);
    
    // Methods to set raw values
    void setRawX(int raw_x_value);
    void setRawY(int raw_y_value);
};

#endif // JSTEST_QT_AXIS_WIDGET_H
