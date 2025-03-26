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

#ifndef JSTEST_QT_BUTTON_WIDGET_H
#define JSTEST_QT_BUTTON_WIDGET_H

#include <QWidget>
#include <QString>

class ButtonWidget : public QWidget
{
    Q_OBJECT

private:
    QString name;
    bool down;

public:
    ButtonWidget(int width, int height, const QString& name, QWidget* parent = nullptr);

    void paintEvent(QPaintEvent* event) override;

public slots:
    void setDown(bool t);
};

#endif // JSTEST_QT_BUTTON_WIDGET_H
