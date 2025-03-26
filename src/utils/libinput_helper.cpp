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

#include "utils/libinput_helper.h"

#include <QDebug>
#include <libinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <libudev.h>

// Static callback required by libinput
static int open_restricted(const char *path, int flags, void *user_data)
{
    int fd = open(path, flags);
    return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data)
{
    close(fd);
}

static const struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

// Singleton instance
LibinputHelper* LibinputHelper::instance()
{
    static LibinputHelper instance;
    return &instance;
}

LibinputHelper::LibinputHelper()
    : m_udev(nullptr),
      m_libinput(nullptr),
      m_notifier(nullptr)
{
}

LibinputHelper::~LibinputHelper()
{
    shutdown();
}

bool LibinputHelper::initialize()
{
    // Already initialized?
    if (m_libinput)
        return true;

    // Initialize udev
    m_udev = udev_new();
    if (!m_udev) {
        qWarning() << "Failed to initialize udev";
        return false;
    }

    // Initialize libinput
    m_libinput = libinput_udev_create_context(&interface, nullptr, m_udev);
    if (!m_libinput) {
        qWarning() << "Failed to initialize libinput";
        udev_unref(m_udev);
        m_udev = nullptr;
        return false;
    }

    // Set up libinput to monitor all input devices
    if (libinput_udev_assign_seat(m_libinput, "seat0") != 0) {
        qWarning() << "Failed to assign seat to libinput context";
        libinput_unref(m_libinput);
        m_libinput = nullptr;
        udev_unref(m_udev);
        m_udev = nullptr;
        return false;
    }

    // Get libinput file descriptor for monitoring
    int fd = libinput_get_fd(m_libinput);
    
    // Create socket notifier to watch for events
    m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &LibinputHelper::handleLibinputEvents);
    m_notifier->setEnabled(true);

    // Initial enumeration of devices
    handleLibinputEvents();
    
    return true;
}

void LibinputHelper::shutdown()
{
    if (m_notifier) {
        m_notifier->setEnabled(false);
        delete m_notifier;
        m_notifier = nullptr;
    }

    if (m_libinput) {
        libinput_unref(m_libinput);
        m_libinput = nullptr;
    }

    if (m_udev) {
        udev_unref(m_udev);
        m_udev = nullptr;
    }
    
    m_callbacks.clear();
}

void LibinputHelper::handleLibinputEvents()
{
    if (!m_libinput)
        return;

    // Process events while they're available
    libinput_dispatch(m_libinput);
    
    // Process pending events
    struct libinput_event *event;
    while ((event = libinput_get_event(m_libinput)) != nullptr) {
        enum libinput_event_type type = libinput_event_get_type(event);
        
        // Handle device added/removed events
        if (type == LIBINPUT_EVENT_DEVICE_ADDED || type == LIBINPUT_EVENT_DEVICE_REMOVED) {
            struct libinput_device *device = libinput_event_get_device(event);
            
            // Only care about joystick devices
            if (isJoystickDevice(device)) {
                DeviceInfo info = getDeviceInfo(device);
                bool added = (type == LIBINPUT_EVENT_DEVICE_ADDED);
                
                // Emit signal for listeners
                emit deviceChanged(added, info);
                
                // Call registered callbacks
                for (auto& callback : m_callbacks) {
                    callback(added, info);
                }
            }
        }
        
        libinput_event_destroy(event);
    }
}

bool LibinputHelper::isJoystickDevice(libinput_device* device)
{
    if (!device)
        return false;
        
    // Check if this is a joystick using various heuristics
    
    // Get udev device for more detailed info
    struct udev_device* udev_device = libinput_device_get_udev_device(device);
    if (!udev_device)
        return false;
        
    // First check: udev tagging
    const char* id_joystick = udev_device_get_property_value(udev_device, "ID_INPUT_JOYSTICK");
    if (id_joystick && strcmp(id_joystick, "1") == 0) {
        udev_device_unref(udev_device);
        return true;
    }
    
    // Second check: is it in the js* device list?
    const char* devnode = udev_device_get_devnode(udev_device);
    if (devnode && strstr(devnode, "/dev/input/js") == devnode) {
        udev_device_unref(udev_device);
        return true;
    }
    
    // Final check: has joystick-like capabilities?
    const char* sysname = udev_device_get_sysname(udev_device);
    if (sysname && strncmp(sysname, "event", 5) == 0) {
        // Get the corresponding event device
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "/dev/input/%s", sysname);
        
        // Open the device to check capabilities
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            unsigned long evbit[NLONGS(EV_CNT)] = { 0 };
            if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) >= 0) {
                // Check for absolute axes
                if (evbit[BIT_WORD(EV_ABS)] & BIT_MASK(EV_ABS)) {
                    unsigned long absbit[NLONGS(ABS_CNT)] = { 0 };
                    ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit);
                    
                    // Check for buttons
                    if (evbit[BIT_WORD(EV_KEY)] & BIT_MASK(EV_KEY)) {
                        unsigned long keybit[NLONGS(KEY_CNT)] = { 0 };
                        ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit);
                        
                        // Check for joystick buttons
                        bool has_js_btn = false;
                        for (int btn = BTN_JOYSTICK; btn < BTN_DIGI; btn++) {
                            if (keybit[BIT_WORD(btn)] & BIT_MASK(btn)) {
                                has_js_btn = true;
                                break;
                            }
                        }
                        
                        // Check for gamepad buttons
                        if (!has_js_btn) {
                            for (int btn = BTN_GAMEPAD; btn < BTN_DPAD_UP; btn++) {
                                if (keybit[BIT_WORD(btn)] & BIT_MASK(btn)) {
                                    has_js_btn = true;
                                    break;
                                }
                            }
                        }
                        
                        // Check for common joystick axes
                        bool has_js_axes = false;
                        if ((absbit[BIT_WORD(ABS_X)] & BIT_MASK(ABS_X)) &&
                            (absbit[BIT_WORD(ABS_Y)] & BIT_MASK(ABS_Y))) {
                            has_js_axes = true;
                        }
                        
                        close(fd);
                        udev_device_unref(udev_device);
                        
                        // If we have joystick buttons and axes, it's likely a joystick
                        return has_js_btn && has_js_axes;
                    }
                }
            }
            close(fd);
        }
    }
    
    udev_device_unref(udev_device);
    return false;
}

LibinputHelper::DeviceInfo LibinputHelper::getDeviceInfo(libinput_device* device)
{
    DeviceInfo info;
    
    // Get the device name
    const char* name = libinput_device_get_name(device);
    if (name) {
        info.name = QString::fromUtf8(name);
    } else {
        info.name = "Unknown Device";
    }
    
    // Get device IDs
    info.vendorId = libinput_device_get_id_vendor(device);
    info.productId = libinput_device_get_id_product(device);
    
    // Get device path from udev
    struct udev_device* udev_device = libinput_device_get_udev_device(device);
    if (udev_device) {
        const char* syspath = udev_device_get_syspath(udev_device);
        if (syspath) {
            info.sysPath = QString::fromUtf8(syspath);
        }
        
        // Check for force feedback
        info.hasForceFeedback = false;
        const char* sysname = udev_device_get_sysname(udev_device);
        if (sysname && strncmp(sysname, "event", 5) == 0) {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "/dev/input/%s", sysname);
            
            int fd = open(path, O_RDONLY | O_NONBLOCK);
            if (fd >= 0) {
                unsigned long evbit[NLONGS(EV_CNT)] = { 0 };
                if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) >= 0) {
                    if (evbit[BIT_WORD(EV_FF)] & BIT_MASK(EV_FF)) {
                        info.hasForceFeedback = true;
                    }
                }
                close(fd);
            }
        }
        
        // Count axis and buttons
        info.axisCount = 0;
        info.buttonCount = 0;
        
        const char* devnode = udev_device_get_devnode(udev_device);
        if (devnode) {
            int fd = open(devnode, O_RDONLY | O_NONBLOCK);
            if (fd >= 0) {
                unsigned long evbit[NLONGS(EV_CNT)] = { 0 };
                unsigned long keybit[NLONGS(KEY_CNT)] = { 0 };
                unsigned long absbit[NLONGS(ABS_CNT)] = { 0 };
                
                if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) >= 0) {
                    // Count absolute axes
                    if (evbit[BIT_WORD(EV_ABS)] & BIT_MASK(EV_ABS)) {
                        ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit);
                        for (int i = 0; i < ABS_CNT; i++) {
                            if (absbit[BIT_WORD(i)] & BIT_MASK(i)) {
                                info.axisCount++;
                            }
                        }
                    }
                    
                    // Count buttons
                    if (evbit[BIT_WORD(EV_KEY)] & BIT_MASK(EV_KEY)) {
                        ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit);
                        
                        // Count joystick buttons
                        for (int btn = BTN_JOYSTICK; btn < BTN_DIGI; btn++) {
                            if (keybit[BIT_WORD(btn)] & BIT_MASK(btn)) {
                                info.buttonCount++;
                            }
                        }
                        
                        // Count gamepad buttons
                        for (int btn = BTN_GAMEPAD; btn < BTN_DPAD_UP; btn++) {
                            if (keybit[BIT_WORD(btn)] & BIT_MASK(btn)) {
                                info.buttonCount++;
                            }
                        }
                    }
                }
                close(fd);
            }
        }
        
        udev_device_unref(udev_device);
    }
    
    // Mark as a joystick (we only get to this point if it is one)
    info.isJoystick = true;
    
    return info;
}

QVector<LibinputHelper::DeviceInfo> LibinputHelper::findJoystickDevices()
{
    QVector<DeviceInfo> devices;
    
    if (!m_libinput) {
        if (!initialize()) {
            return devices;
        }
    }
    
    // Make sure we have the most current device state
    libinput_dispatch(m_libinput);
    
    // Make a udev query for all input devices to find joysticks
    struct udev_enumerate* enumerate = udev_enumerate_new(m_udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_add_match_property(enumerate, "ID_INPUT_JOYSTICK", "1");
    udev_enumerate_scan_devices(enumerate);
    
    struct udev_list_entry* devices_list = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* entry;
    
    // Iterate through all devices found
    udev_list_entry_foreach(entry, devices_list) {
        const char* path = udev_list_entry_get_name(entry);
        struct udev_device* dev = udev_device_new_from_syspath(m_udev, path);
        
        const char* devnode = udev_device_get_devnode(dev);
        if (devnode) {
            // Try to add this device to our libinput instance
            libinput_device* libinput_dev = libinput_path_add_device(m_libinput, devnode);
            if (libinput_dev) {
                // Check if it's a joystick
                if (isJoystickDevice(libinput_dev)) {
                    DeviceInfo info = getDeviceInfo(libinput_dev);
                    devices.append(info);
                }
                
                // We don't remove the device from libinput context to keep it available for events
            }
        }
        
        udev_device_unref(dev);
    }
    
    udev_enumerate_unref(enumerate);
    
    return devices;
}

void LibinputHelper::registerDeviceCallback(std::function<void(bool added, const DeviceInfo&)> callback)
{
    m_callbacks.append(callback);
}
