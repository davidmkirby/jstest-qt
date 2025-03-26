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

// Test tool to verify libinput backend functionality

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <iostream>
#include <memory>

#include "../joystick_factory.h"
#include "../utils/libinput_helper.h"
#include "../libinput_joystick.h"

// Simple class to monitor joystick events
class JoystickMonitor : public QObject {
    Q_OBJECT
    
public:
    JoystickMonitor(QObject* parent = nullptr) : QObject(parent) {
        // Connect to hotplug events
        LibinputHelper* helper = LibinputHelper::instance();
        helper->initialize();
        
        helper->registerDeviceCallback([this](bool added, const LibinputHelper::DeviceInfo& device) {
            if (added) {
                qDebug() << "Device added:" << device.name;
                qDebug() << "  Path:" << device.sysPath;
                qDebug() << "  Axes:" << device.axisCount;
                qDebug() << "  Buttons:" << device.buttonCount;
                
                // Try to open the device
                try {
                    auto joystick = JoystickFactory::createJoystick(device.sysPath.toStdString(), JoystickBackend::LIBINPUT);
                    
                    // Store the joystick
                    m_joysticks.push_back(std::move(joystick));
                    
                    // Connect to signals
                    Joystick* js = m_joysticks.back().get();
                    connect(js, &Joystick::axisChanged, this, &JoystickMonitor::onAxisChanged);
                    connect(js, &Joystick::buttonChanged, this, &JoystickMonitor::onButtonChanged);
                    
                    qDebug() << "  Successfully opened device";
                } catch (const std::exception& e) {
                    qWarning() << "  Failed to open device:" << e.what();
                }
            } else {
                qDebug() << "Device removed:" << device.name;
                
                // Remove from our list (find by path)
                for (auto it = m_joysticks.begin(); it != m_joysticks.end(); ++it) {
                    if ((*it)->getFilename() == device.sysPath.toStdString()) {
                        m_joysticks.erase(it);
                        qDebug() << "  Removed device from active list";
                        break;
                    }
                }
            }
        });
        
        // Print initial devices
        qDebug() << "Looking for joystick devices...";
        QVector<LibinputHelper::DeviceInfo> devices = helper->findJoystickDevices();
        for (const auto& device : devices) {
            qDebug() << "Found device:" << device.name;
            qDebug() << "  Path:" << device.sysPath;
            qDebug() << "  Axes:" << device.axisCount;
            qDebug() << "  Buttons:" << device.buttonCount;
            
            // Try to open the device
            try {
                auto joystick = JoystickFactory::createJoystick(device.sysPath.toStdString(), JoystickBackend::LIBINPUT);
                
                // Store the joystick
                m_joysticks.push_back(std::move(joystick));
                
                // Connect to signals
                Joystick* js = m_joysticks.back().get();
                connect(js, &Joystick::axisChanged, this, &JoystickMonitor::onAxisChanged);
                connect(js, &Joystick::buttonChanged, this, &JoystickMonitor::onButtonChanged);
                
                qDebug() << "  Successfully opened device";
            } catch (const std::exception& e) {
                qWarning() << "  Failed to open device:" << e.what();
            }
        }
        
        if (devices.isEmpty()) {
            qDebug() << "No joystick devices found. Plugin a controller and it will be detected automatically.";
        }
    }
    
private slots:
    void onAxisChanged(int number, int value) {
        Joystick* js = qobject_cast<Joystick*>(sender());
        if (js) {
            std::cout << "Axis " << number << " = " << value << " (Device: " << js->getName().toStdString() << ")\r" << std::flush;
        }
    }
    
    void onButtonChanged(int number, bool value) {
        Joystick* js = qobject_cast<Joystick*>(sender());
        if (js) {
            if (value) {
                std::cout << "Button " << number << " pressed (Device: " << js->getName().toStdString() << ")" << std::endl;
            } else {
                std::cout << "Button " << number << " released (Device: " << js->getName().toStdString() << ")" << std::endl;
            }
        }
    }
    
private:
    std::vector<std::unique_ptr<Joystick>> m_joysticks;
};

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    
    qDebug() << "Libinput Joystick Test";
    qDebug() << "=====================";
    qDebug() << "This tool will monitor all joystick devices using the libinput backend.";
    qDebug() << "Press Ctrl+C to exit.";
    qDebug() << "";
    
    // Create our monitor
    JoystickMonitor monitor;
    
    // Exit after 5 minutes (optional)
    QTimer::singleShot(5 * 60 * 1000, &app, &QCoreApplication::quit);
    
    return app.exec();
}

// Include the Q_OBJECT macro in compilation
#include "libinput_test.moc"
