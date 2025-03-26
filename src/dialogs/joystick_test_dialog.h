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

#ifndef JSTEST_QT_JOYSTICK_TEST_DIALOG_H
#define JSTEST_QT_JOYSTICK_TEST_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFrame>
#include <QVector>
#include <functional>

// Forward declarations to avoid circular dependencies
class Joystick;
class JoystickGui;
class ButtonWidget;
class AxisWidget;
class RudderWidget;
class ThrottleWidget;

class JoystickTestDialog : public QDialog
{
    Q_OBJECT

private:
    JoystickGui& m_gui;
    Joystick& joystick;
    bool m_simple_ui;

    QVBoxLayout m_vbox;
    QWidget alignment;
    QLabel label;

    QFrame axis_frame;
    QVBoxLayout axis_vbox;
    QFrame button_frame;
    QGridLayout axis_grid;
    QGridLayout button_grid;
    QHBoxLayout test_hbox;
    QHBoxLayout stick_hbox;

    QPushButton mapping_button;
    QPushButton calibration_button;
    QPushButton close_button;
    QHBoxLayout buttonbox;

    // Changed to pointers for forward-declared classes
    AxisWidget* stick1_widget;
    AxisWidget* stick2_widget;
    AxisWidget* stick3_widget;

    RudderWidget* rudder_widget;
    ThrottleWidget* throttle_widget;

    ThrottleWidget* left_trigger_widget;
    ThrottleWidget* right_trigger_widget;

    QVector<QProgressBar*> axes;
    QVector<ButtonWidget*> buttons;
    QVector<QLabel*> axis_value_labels;

    QVector<std::function<void(double)>> axis_callbacks;
    QVector<std::function<void(int)>> raw_value_callbacks;

private slots:
    void axisMove(int number, int value);
    void buttonMove(int number, bool value);

    void onCalibrate();
    void onMapping();

public:
    JoystickTestDialog(JoystickGui& gui, Joystick& joystick, bool simple_ui);
    ~JoystickTestDialog(); // Need a destructor to clean up pointers
};

#endif // JSTEST_QT_JOYSTICK_TEST_DIALOG_H
