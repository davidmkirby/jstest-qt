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

#ifndef JSTEST_QT_LIBINPUT_HELPER_H
#define JSTEST_QT_LIBINPUT_HELPER_H

#include <QObject>
#include <QSocketNotifier>
#include <QMap>
#include <QString>
#include <QVector>
#include <memory>
#include <functional>

#include "utils/evdev_helper.h" // Include for bit manipulation macros

// Forward declarations to avoid including libinput headers in our header
struct libinput;
struct libinput_device;
struct udev;

class LibinputHelper : public QObject {
    Q_OBJECT

public:
    // Information about a discovered input device
    struct DeviceInfo {
        QString name;
        QString sysPath;
        int vendorId;
        int productId;
        bool isJoystick;
        bool hasForceFeedback;
        int axisCount;
        int buttonCount;
    };

    // Singleton access
    static LibinputHelper* instance();

    // Initialize libinput context
    bool initialize();

    // Shutdown and cleanup
    void shutdown();

    // Find all joystick devices
    QVector<DeviceInfo> findJoystickDevices();

    // Check if a device is a joystick
    static bool isJoystickDevice(libinput_device* device);

    // Register for device hotplug notifications
    void registerDeviceCallback(std::function<void(bool added, const DeviceInfo&)> callback);

signals:
    // Signal emitted when a device is added or removed
    void deviceChanged(bool added, const DeviceInfo& device);

private slots:
    // Handle libinput events when data is available on the FD
    void handleLibinputEvents();

private:
    // Private constructor for singleton
    LibinputHelper();
    ~LibinputHelper();

    // Process a single libinput event
    void processEvent();

    // Helper to extract device info
    DeviceInfo getDeviceInfo(libinput_device* device);

    // Member variables
    struct udev* m_udev;
    struct libinput* m_libinput;
    QSocketNotifier* m_notifier;
    QVector<std::function<void(bool added, const DeviceInfo&)>> m_callbacks;

    // Prohibit copying
    LibinputHelper(const LibinputHelper&) = delete;
    LibinputHelper& operator=(const LibinputHelper&) = delete;
};

#endif // JSTEST_QT_LIBINPUT_HELPER_H
