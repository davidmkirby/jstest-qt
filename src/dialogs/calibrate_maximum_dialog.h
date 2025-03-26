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

#ifndef JSTEST_QT_CALIBRATE_MAXIMUM_DIALOG_H
#define JSTEST_QT_CALIBRATE_MAXIMUM_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QVector>

// Include the full joystick.h instead of just forward declaring it
#include "../joystick.h"

class CalibrateMaximumDialog : public QDialog
{
    Q_OBJECT

private:
    Joystick& joystick;
    std::vector<Joystick::CalibrationData> orig_data;
    QLabel label;
    QMetaObject::Connection connection;
    QVector<bool> is_init_axis_state;
    QVector<int> min_axis_state;
    QVector<int> max_axis_state;

private slots:
    void onAxisMove(int id, int value);
    void onDone(int result);

public:
    CalibrateMaximumDialog(Joystick& joystick, QWidget* parent = nullptr);
};

#endif // JSTEST_QT_CALIBRATE_MAXIMUM_DIALOG_H
