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

#ifndef JSTEST_QT_JOYSTICK_H
#define JSTEST_QT_JOYSTICK_H

#include <QObject>
#include <QSocketNotifier>
#include <QString>
#include <vector>
#include <linux/joystick.h>

#include "joystick_description.h"

class Joystick : public QObject
{
    Q_OBJECT

public:
    struct CalibrationData {
        bool calibrate;
        bool invert;
        int center_min;
        int center_max;
        int range_min;
        int range_max;
    };

private:
    int fd;

    std::string filename;
    std::string orig_name;
    QString name;
    int axis_count;
    int button_count;

    std::vector<int> axis_state;
    std::vector<CalibrationData> orig_calibration_data;

    QSocketNotifier* notifier;

public:
    Joystick(const std::string& filename);
    ~Joystick();

    int getFd() const { return fd; }

    void update();

    std::string getFilename() const { return filename; }
    QString getName() const { return name; }
    int getAxisCount() const { return axis_count; }
    int getButtonCount() const { return button_count; }

    int getAxisState(int id);

    static std::vector<JoystickDescription> getJoysticks();

    std::vector<CalibrationData> getCalibration();
    void setCalibration(const std::vector<CalibrationData>& data);
    void resetCalibration();

    /** Clears all calibration data, note that this will mean raw USB
        input values, not values scaled to -32767/32767 */
    void clearCalibration();

    std::vector<int> getButtonMapping();
    std::vector<int> getAxisMapping();

    void setButtonMapping(const std::vector<int>& mapping);
    void setAxisMapping(const std::vector<int>& mapping);

    /** Corrects calibration data after remaping axes */
    void correctCalibration(const std::vector<int>& mapping_old, const std::vector<int>& mapping_new);

    /** Get the evdev that this joystick device is based on. This call
        is just a guess, not guaranteed to be the exact same device, but
        for our uses that should be enough. */
    std::string getEvdev() const;

signals:
    void axisChanged(int number, int value);
    void buttonChanged(int number, bool value);

private slots:
    void onSocketActivated(int socket);

private:
    Joystick(const Joystick&) = delete;
    Joystick& operator=(const Joystick&) = delete;
};

#endif // JSTEST_QT_JOYSTICK_H
