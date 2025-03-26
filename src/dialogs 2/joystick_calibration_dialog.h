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

#ifndef JSTEST_QT_JOYSTICK_CALIBRATION_DIALOG_H
#define JSTEST_QT_JOYSTICK_CALIBRATION_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QFrame>
#include <QTableWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QVector>

// Include the full joystick.h instead of just forward declaring it
#include "../joystick.h"

class JoystickCalibrationDialog : public QDialog
{
    Q_OBJECT

private:
    Joystick& joystick;

    QLabel label;
    QFrame axis_frame;
    QTableWidget axis_table;
    QVBoxLayout buttonbox;
    QPushButton calibration_button;
    QScrollArea scroll;

    struct CalibrationData {
        QCheckBox* invert;
        QSpinBox* center_min;
        QSpinBox* center_max;
        QSpinBox* range_min;
        QSpinBox* range_max;
    };

    QVector<CalibrationData> calibration_data;

private slots:
    void onApply();
    void onClear();
    void onResponse(int result);
    void onCalibrate();

public:
    JoystickCalibrationDialog(Joystick& joystick, QWidget* parent = nullptr);

    void updateWith(const std::vector<Joystick::CalibrationData>& data);
};

#endif // JSTEST_QT_JOYSTICK_CALIBRATION_DIALOG_H
