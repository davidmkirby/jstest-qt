/*
**  jstest-qt - A Qt joystick tester
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

#ifndef JSTEST_QT_LIBINPUT_JOYSTICK_H
#define JSTEST_QT_LIBINPUT_JOYSTICK_H

#include <QObject>
#include <QSocketNotifier>
#include <QString>
#include <vector>
#include <memory>

#include "joystick.h" // Include the base class header

// Forward declarations
struct libinput;
struct libinput_device;
struct udev;

// Inherit from Joystick class
class LibinputJoystick : public Joystick
{
    Q_OBJECT

private:
    // Private device-specific context
    struct udev* m_udev;
    struct libinput* m_libinput;
    libinput_device* m_device;
    QSocketNotifier* m_notifier;
    
    std::string m_syspath;
    std::vector<bool> m_button_state;
    std::vector<int> m_axis_mapping;
    std::vector<int> m_button_mapping;

public:
    // Constructor takes a device path
    LibinputJoystick(const std::string& device_path);
    ~LibinputJoystick() override;

    // Override methods from Joystick
    int getFd() const override;
    void update() override;

    int getAxisState(int id) override;

    // Static helper methods
    static std::vector<LibinputJoystick*> getJoysticks();

    // Calibration methods
    std::vector<CalibrationData> getCalibration() override;
    void setCalibration(const std::vector<CalibrationData>& data) override;
    void resetCalibration() override;
    void clearCalibration() override;

    // Mapping methods
    std::vector<int> getButtonMapping() override;
    std::vector<int> getAxisMapping() override;
    void setButtonMapping(const std::vector<int>& mapping) override;
    void setAxisMapping(const std::vector<int>& mapping) override;
    void correctCalibration(const std::vector<int>& mapping_old, const std::vector<int>& mapping_new) override;

    // Get the evdev that this joystick device is based on
    std::string getEvdev() const override;

private slots:
    void onSocketActivated(int socket);

private:
    // Initialize device-specific resources
    bool initDevice();
    
    // Process libinput events
    void processEvent();
    
    // Apply calibration to raw axis values
    int applyCalibration(int axis, int value);
    
    // Prohibit copying
    LibinputJoystick(const LibinputJoystick&) = delete;
    LibinputJoystick& operator=(const LibinputJoystick&) = delete;
};

#endif // JSTEST_QT_LIBINPUT_JOYSTICK_H
