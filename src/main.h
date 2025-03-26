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

#ifndef JSTEST_QT_MAIN_H
#define JSTEST_QT_MAIN_H

#include <QApplication>
#include <QString>
#include <QMap>
#include <memory>

class Joystick;
class QWidget;
class JoystickListDialog;
class JoystickTestDialog;
class JoystickMapDialog;
class JoystickCalibrationDialog;

class JoystickGui : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<Joystick> m_joystick;
    std::unique_ptr<JoystickTestDialog> m_test_dialog;
    std::unique_ptr<JoystickMapDialog> m_mapping_dialog;
    std::unique_ptr<JoystickCalibrationDialog> m_calibration_dialog;

public:
    JoystickGui(std::unique_ptr<Joystick> joystick,
                bool simple_ui,
                QWidget* parent = nullptr);

    JoystickTestDialog* getTestDialog() const { return m_test_dialog.get(); }

public slots:
    void showCalibrationDialog();
    void showMappingDialog();
};

class JoystickApp : public QApplication
{
    Q_OBJECT

private:
    static JoystickApp* m_instance;
    
    QString m_datadir;
    bool m_simple_ui;

    QMap<QString, std::unique_ptr<JoystickGui>> m_joystick_guis;

public:
    JoystickApp(int& argc, char** argv);
    ~JoystickApp();

    JoystickTestDialog* showDevicePropertyDialog(const QString& filename, QWidget* parent = nullptr);

    static JoystickApp* instance() { return m_instance; }
    
    int run();
    QString getDataDirectory() const { return m_datadir; }
};

#endif // JSTEST_QT_MAIN_H
