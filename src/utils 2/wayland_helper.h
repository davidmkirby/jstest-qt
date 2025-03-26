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

#ifndef JSTEST_QT_WAYLAND_HELPER_H
#define JSTEST_QT_WAYLAND_HELPER_H

#include <QObject>
#include <QSocketNotifier>
#include <QString>
#include <QMap>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <libudev.h>

class WaylandInputHelper : public QObject {
    Q_OBJECT

public:
    // Device types we're interested in
    enum DeviceType {
        JOYSTICK,
        KEYBOARD,
        MOUSE,
        UNKNOWN
    };

    // Information about a discovered device
    struct DeviceInfo {
        QString path;
        QString name;
        QString sysPath;
        DeviceType type;
    };

    static WaylandInputHelper* instance() {
        static WaylandInputHelper instance;
        return &instance;
    }

    // Find all input devices of a specific type
    QVector<DeviceInfo> findInputDevices(DeviceType type = JOYSTICK) {
        QVector<DeviceInfo> devices;
        
        struct udev* udev = udev_new();
        if (!udev) {
            qWarning() << "Failed to initialize udev";
            return devices;
        }

        struct udev_enumerate* enumerate = udev_enumerate_new(udev);
        udev_enumerate_add_match_subsystem(enumerate, "input");
        
        // Add specific property match for joysticks
        if (type == JOYSTICK) {
            udev_enumerate_add_match_property(enumerate, "ID_INPUT_JOYSTICK", "1");
        }
        
        udev_enumerate_scan_devices(enumerate);
        
        struct udev_list_entry* devices_list = udev_enumerate_get_list_entry(enumerate);
        struct udev_list_entry* entry;
        
        // Iterate through all devices found
        udev_list_entry_foreach(entry, devices_list) {
            const char* path = udev_list_entry_get_name(entry);
            struct udev_device* dev = udev_device_new_from_syspath(udev, path);
            
            // Skip virtual devices
            const char* sysname = udev_device_get_sysname(dev);
            if (strncmp(sysname, "event", 5) != 0) {
                udev_device_unref(dev);
                continue;
            }
            
            // Get device information
            struct udev_device* parent = udev_device_get_parent_with_subsystem_devtype(
                dev, "input", NULL);
                
            if (parent) {
                const char* devnode = udev_device_get_devnode(dev);
                if (devnode) {
                    const char* name = udev_device_get_property_value(parent, "NAME");
                    if (name) {
                        // Process the name (remove quotes)
                        QString cleanName = QString(name);
                        if (cleanName.startsWith('"') && cleanName.endsWith('"')) {
                            cleanName = cleanName.mid(1, cleanName.length() - 2);
                        }
                        
                        // Determine device type
                        DeviceType deviceType = determineDeviceType(parent);
                        
                        if (deviceType == type || type == UNKNOWN) {
                            DeviceInfo info;
                            info.path = QString(devnode);
                            info.name = cleanName;
                            info.sysPath = QString(path);
                            info.type = deviceType;
                            devices.append(info);
                        }
                    }
                }
            }
            
            udev_device_unref(dev);
        }
        
        udev_enumerate_unref(enumerate);
        udev_unref(udev);
        
        return devices;
    }

private:
    WaylandInputHelper() {}
    ~WaylandInputHelper() {}
    
    DeviceType determineDeviceType(struct udev_device* dev) {
        // Check for joystick capabilities
        if (udev_device_get_property_value(dev, "ID_INPUT_JOYSTICK")) {
            return JOYSTICK;
        }
        
        // Check for keyboard capabilities
        if (udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD")) {
            return KEYBOARD;
        }
        
        // Check for mouse capabilities
        if (udev_device_get_property_value(dev, "ID_INPUT_MOUSE")) {
            return MOUSE;
        }
        
        return UNKNOWN;
    }
};

#endif // JSTEST_QT_WAYLAND_HELPER_H
