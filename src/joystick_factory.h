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

#ifndef JSTEST_QT_JOYSTICK_FACTORY_H
#define JSTEST_QT_JOYSTICK_FACTORY_H

#include <memory>
#include <vector>
#include <string>
#include <QObject>

#include "joystick.h"
#include "joystick_description.h"

// Define settings for backend selection
enum class JoystickBackend {
    AUTO,      // Automatically select the best backend
    LEGACY,    // Use the traditional Linux joystick API
    LIBINPUT,  // Use libinput backend
    EVDEV      // Use direct evdev access
};

class JoystickFactory {
public:
    // Get a list of available joysticks
    static std::vector<JoystickDescription> getJoysticks(JoystickBackend backend = JoystickBackend::AUTO);
    
    // Create a joystick instance
    static std::unique_ptr<Joystick> createJoystick(const std::string& device_path, JoystickBackend backend = JoystickBackend::AUTO);
    
    // Determine if we're running under Wayland
    static bool isWaylandSession();
    
    // Set the default backend
    static void setDefaultBackend(JoystickBackend backend);
    
    // Get the current default backend
    static JoystickBackend getDefaultBackend();
    
private:
    static JoystickBackend s_defaultBackend;
};

#endif // JSTEST_QT_JOYSTICK_FACTORY_H
