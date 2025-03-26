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

#include "joystick_factory.h"

#include <QDebug>
#include <QGuiApplication>
#include <QProcessEnvironment>

#include "joystick.h"
#include "libinput_joystick.h"
#include "utils/libinput_helper.h"

// Initialize static members
JoystickBackend JoystickFactory::s_defaultBackend = JoystickBackend::AUTO;

// Determine if we're running under Wayland
bool JoystickFactory::isWaylandSession()
{
    // Check if Wayland display is set
    static bool checked = false;
    static bool isWayland = false;
    
    if (!checked) {
        // Check if QT_QPA_PLATFORM is set to wayland
        QString platformName = qgetenv("QT_QPA_PLATFORM");
        if (platformName.toLower() == "wayland") {
            isWayland = true;
        }
        
        // Check if WAYLAND_DISPLAY is set
        QString waylandDisplay = qgetenv("WAYLAND_DISPLAY");
        if (!waylandDisplay.isEmpty()) {
            isWayland = true;
        }
        
        // Check QGuiApplication's platformName (more reliable but requires app to be initialized)
        if (QGuiApplication::instance()) {
            QString platform = QGuiApplication::platformName();
            isWayland = platform.toLower().contains("wayland");
        }
        
        checked = true;
    }
    
    return isWayland;
}

void JoystickFactory::setDefaultBackend(JoystickBackend backend)
{
    s_defaultBackend = backend;
}

JoystickBackend JoystickFactory::getDefaultBackend()
{
    return s_defaultBackend;
}

std::vector<JoystickDescription> JoystickFactory::getJoysticks(JoystickBackend backend)
{
    std::vector<JoystickDescription> result;
    
    // If AUTO, choose the best backend
    if (backend == JoystickBackend::AUTO) {
        if (isWaylandSession()) {
            backend = JoystickBackend::LIBINPUT;
        } else {
            backend = JoystickBackend::LEGACY;
        }
    }
    
    // Call the appropriate backend
    switch (backend) {
        case JoystickBackend::LIBINPUT: {
            // Get devices using libinput
            LibinputHelper* helper = LibinputHelper::instance();
            if (helper->initialize()) {
                QVector<LibinputHelper::DeviceInfo> devices = helper->findJoystickDevices();
                for (const auto& device : devices) {
                    JoystickDescription desc(
                        device.sysPath.toStdString(),
                        device.name.toStdString(),
                        device.axisCount,
                        device.buttonCount
                    );
                    result.push_back(desc);
                }
            }
            
            // If we didn't find any devices with libinput, fall back to legacy
            if (result.empty()) {
                qDebug() << "No devices found with libinput, falling back to legacy";
                backend = JoystickBackend::LEGACY;
            } else {
                break;
            }
        }
            
        case JoystickBackend::LEGACY:
        default:
            // Get devices using the traditional method
            result = Joystick::getJoysticks();
            break;
    }
    
    return result;
}

std::unique_ptr<Joystick> JoystickFactory::createJoystick(const std::string& device_path, JoystickBackend backend)
{
    // If AUTO, choose the best backend
    if (backend == JoystickBackend::AUTO) {
        if (isWaylandSession()) {
            backend = JoystickBackend::LIBINPUT;
        } else {
            backend = JoystickBackend::LEGACY;
        }
    }
    
    // Create a joystick with the selected backend
    try {
        switch (backend) {
            case JoystickBackend::LIBINPUT:
                try {
                    // Try creating a LibinputJoystick
                    // We need to cast it to Joystick* to make it compatible with the unique_ptr
                    Joystick* joystick = new LibinputJoystick(device_path);
                    return std::unique_ptr<Joystick>(joystick);
                } catch (const std::exception& e) {
                    // If libinput fails, fall back to legacy
                    qWarning() << "Failed to create LibinputJoystick, falling back to legacy:" << e.what();
                    backend = JoystickBackend::LEGACY;
                }
                
            case JoystickBackend::LEGACY:
            default:
                // Create a traditional joystick
                return std::make_unique<Joystick>(device_path);
        }
    } catch (const std::exception& e) {
        qWarning() << "Failed to create joystick:" << e.what();
        throw;
    }
}
