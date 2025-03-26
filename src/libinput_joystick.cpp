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

#include "libinput_joystick.h"

#include <QDebug>
#include <libinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <libudev.h>
#include <math.h>
#include <algorithm>
#include <stdexcept>
#include <iostream>

#include "utils/evdev_helper.h"
#include "utils/libinput_helper.h"

// Define bit manipulation macros needed for evdev
#ifndef BITS_PER_LONG
#define BITS_PER_LONG (sizeof(long) * 8)
#endif

#ifndef NBITS
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#endif

#ifndef NLONGS
#define NLONGS(x) (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#endif

#ifndef BIT_WORD
#define BIT_WORD(nr) ((nr) / BITS_PER_LONG)
#endif

#ifndef BIT_MASK
#define BIT_MASK(nr) (1UL << ((nr) % BITS_PER_LONG))
#endif

// Static callback required by libinput for device-specific contexts
static int joystick_open_restricted(const char *path, int flags, void *user_data)
{
    int fd = open(path, flags);
    return fd < 0 ? -errno : fd;
}

static void joystick_close_restricted(int fd, void *user_data)
{
    close(fd);
}

static const struct libinput_interface joystick_interface = {
    .open_restricted = joystick_open_restricted,
    .close_restricted = joystick_close_restricted,
};

LibinputJoystick::LibinputJoystick(const std::string& device_path)
    : Joystick(), // Call the base class constructor
      m_udev(nullptr),
      m_libinput(nullptr),
      m_device(nullptr),
      m_notifier(nullptr),
      m_syspath("")
{
    // Initialize base class member
    filename = device_path;
    
    if (!initDevice()) {
        throw std::runtime_error("Failed to initialize libinput device: " + device_path);
    }

    // Create and store original calibration data
    orig_calibration_data = getCalibration();

    // Set up socket notifier for event processing
    int fd = libinput_get_fd(m_libinput);
    m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &LibinputJoystick::onSocketActivated, 
            Qt::DirectConnection);
    m_notifier->setEnabled(true);
    
    qDebug() << "LibinputJoystick initialized:" << name << "with" << axis_count << "axes and" << button_count << "buttons";
}

LibinputJoystick::~LibinputJoystick()
{
    // Clean up resources
    if (m_notifier) {
        m_notifier->setEnabled(false);
        delete m_notifier;
        m_notifier = nullptr;
    }

    if (m_device) {
        libinput_device_unref(m_device);
        m_device = nullptr;
    }

    if (m_libinput) {
        libinput_unref(m_libinput);
        m_libinput = nullptr;
    }

    if (m_udev) {
        udev_unref(m_udev);
        m_udev = nullptr;
    }
}

bool LibinputJoystick::initDevice()
{
    // Initialize udev
    m_udev = udev_new();
    if (!m_udev) {
        qWarning() << "Failed to initialize udev";
        return false;
    }

    // Check if the path is a direct device path or a syspath
    bool is_syspath = filename.find("/sys/") == 0;
    
    // For syspath, find the corresponding device node
    std::string device_node;
    if (is_syspath) {
        struct udev_device* dev = udev_device_new_from_syspath(m_udev, filename.c_str());
        if (!dev) {
            qWarning() << "Failed to find device for syspath:" << QString::fromStdString(filename);
            udev_unref(m_udev);
            m_udev = nullptr;
            return false;
        }
        
        const char* devnode = udev_device_get_devnode(dev);
        if (devnode) {
            device_node = devnode;
        }
        
        udev_device_unref(dev);
        
        if (device_node.empty()) {
            qWarning() << "No device node found for syspath:" << QString::fromStdString(filename);
            udev_unref(m_udev);
            m_udev = nullptr;
            return false;
        }
        
        m_syspath = filename;
    } else {
        device_node = filename;
    }
    
    // Create a libinput context for this device
    m_libinput = libinput_path_create_context(&joystick_interface, nullptr);
    if (!m_libinput) {
        qWarning() << "Failed to create libinput context";
        udev_unref(m_udev);
        m_udev = nullptr;
        return false;
    }
    
    // Add the device to the context
    m_device = libinput_path_add_device(m_libinput, device_node.c_str());
    if (!m_device) {
        qWarning() << "Failed to add device to libinput context:" << QString::fromStdString(device_node);
        libinput_unref(m_libinput);
        m_libinput = nullptr;
        udev_unref(m_udev);
        m_udev = nullptr;
        return false;
    }
    
    // Get the device name
    const char* device_name = libinput_device_get_name(m_device);
    if (device_name) {
        name = QString::fromUtf8(device_name);
        orig_name = device_name;
    } else {
        name = "Unknown Device";
        orig_name = "Unknown Device";
    }
    
    // Get the device syspath
    struct udev_device* udev_device = libinput_device_get_udev_device(m_device);
    if (udev_device) {
        const char* syspath = udev_device_get_syspath(udev_device);
        if (syspath) {
            m_syspath = syspath;
        }
        
        // Get the evdev node
        const char* devnode = udev_device_get_devnode(udev_device);
        if (devnode) {
            // Use the actual device node as the filename
            filename = devnode;
            
            // Now count axes and buttons
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
                                axis_count++;
                                // Store the axis mapping
                                m_axis_mapping.push_back(i);
                            }
                        }
                    }
                    
                    // Count buttons
                    if (evbit[BIT_WORD(EV_KEY)] & BIT_MASK(EV_KEY)) {
                        ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit);
                        
                        // Count joystick buttons
                        for (int btn = BTN_JOYSTICK; btn < BTN_DIGI; btn++) {
                            if (keybit[BIT_WORD(btn)] & BIT_MASK(btn)) {
                                button_count++;
                                // Store the button mapping
                                m_button_mapping.push_back(btn);
                            }
                        }
                        
                        // Count gamepad buttons
                        for (int btn = BTN_GAMEPAD; btn < BTN_DPAD_UP; btn++) {
                            if (keybit[BIT_WORD(btn)] & BIT_MASK(btn)) {
                                button_count++;
                                // Store the button mapping
                                m_button_mapping.push_back(btn);
                            }
                        }
                    }
                }
                close(fd);
            }
        }
        
        udev_device_unref(udev_device);
    }
    
    // Initialize state vectors
    axis_state.resize(axis_count, 0);
    m_button_state.resize(button_count, false);
    
    // Initialize calibration data
    std::vector<CalibrationData> cal_data;
    cal_data.resize(axis_count);
    for (int i = 0; i < axis_count; i++) {
        cal_data[i].calibrate = false;
        cal_data[i].invert = false;
        cal_data[i].center_min = 0;
        cal_data[i].center_max = 0;
        cal_data[i].range_min = -32767;
        cal_data[i].range_max = 32767;
    }
    
    return true;
}

int LibinputJoystick::getFd() const
{
    return m_libinput ? libinput_get_fd(m_libinput) : -1;
}

void LibinputJoystick::onSocketActivated(int socket)
{
    if (socket == getFd()) {
        update();
    }
}

void LibinputJoystick::update()
{
    if (!m_libinput)
        return;
        
    // Process events
    libinput_dispatch(m_libinput);
    
    struct libinput_event *event;
    while ((event = libinput_get_event(m_libinput)) != nullptr) {
        enum libinput_event_type type = libinput_event_get_type(event);
        
        switch (type) {
            case LIBINPUT_EVENT_POINTER_MOTION:
                // Handle relative motion (not used for joysticks)
                break;
                
            case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE: {
                // Handle absolute motion events (axes)
                struct libinput_event_pointer *pointer_event = 
                    libinput_event_get_pointer_event(event);
                    
                // Convert normalized coordinates to our range
                double x = libinput_event_pointer_get_absolute_x_transformed(
                    pointer_event, 65535) - 32767;
                double y = libinput_event_pointer_get_absolute_y_transformed(
                    pointer_event, 65535) - 32767;
                    
                // Update X axis if we have one
                if (axis_count > 0) {
                    int old_value = axis_state[0];
                    int new_value = applyCalibration(0, static_cast<int>(x));
                    axis_state[0] = new_value;
                    if (old_value != new_value) {
                        emit axisChanged(0, new_value);
                    }
                }
                
                // Update Y axis if we have one
                if (axis_count > 1) {
                    int old_value = axis_state[1];
                    int new_value = applyCalibration(1, static_cast<int>(y));
                    axis_state[1] = new_value;
                    if (old_value != new_value) {
                        emit axisChanged(1, new_value);
                    }
                }
                break;
            }
                
            case LIBINPUT_EVENT_POINTER_BUTTON: {
                // Handle button press/release events
                struct libinput_event_pointer *pointer_event = 
                    libinput_event_get_pointer_event(event);
                    
                uint32_t button = libinput_event_pointer_get_button(pointer_event);
                enum libinput_button_state button_state = 
                    libinput_event_pointer_get_button_state(pointer_event);
                    
                // Find the button in our mapping
                for (int i = 0; i < m_button_mapping.size(); i++) {
                    if (static_cast<uint32_t>(m_button_mapping[i]) == button) {
                        bool state = (button_state == LIBINPUT_BUTTON_STATE_PRESSED);
                        m_button_state[i] = state;
                        emit buttonChanged(i, state);
                        break;
                    }
                }
                break;
            }
                
            case LIBINPUT_EVENT_POINTER_AXIS: {
                // Handle scroll wheel or other axis events
                struct libinput_event_pointer *pointer_event = 
                    libinput_event_get_pointer_event(event);
                    
                enum libinput_pointer_axis axis = LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL;
                if (libinput_event_pointer_has_axis(pointer_event, axis)) {
                    double value = libinput_event_pointer_get_axis_value(pointer_event, axis);
                    
                    // Map to an axis if we have any left
                    if (axis_count > 2) {
                        int old_value = axis_state[2];
                        int new_value = applyCalibration(2, static_cast<int>(value * 10000));
                        axis_state[2] = new_value;
                        if (old_value != new_value) {
                            emit axisChanged(2, new_value);
                        }
                    }
                }
                
                axis = LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL;
                if (libinput_event_pointer_has_axis(pointer_event, axis)) {
                    double value = libinput_event_pointer_get_axis_value(pointer_event, axis);
                    
                    // Map to an axis if we have any left
                    if (axis_count > 3) {
                        int old_value = axis_state[3];
                        int new_value = applyCalibration(3, static_cast<int>(value * 10000));
                        axis_state[3] = new_value;
                        if (old_value != new_value) {
                            emit axisChanged(3, new_value);
                        }
                    }
                }
                break;
            }
            
            // Process other device-specific events here if needed
                
            default:
                // Ignore other event types
                break;
        }
        
        libinput_event_destroy(event);
    }
}

int LibinputJoystick::applyCalibration(int axis, int value)
{
    std::vector<CalibrationData> cal_data = getCalibration();
    if (axis < 0 || axis >= cal_data.size())
        return value;
        
    const CalibrationData& cal = cal_data[axis];
    
    if (!cal.calibrate)
        return value;
        
    // Apply calibration
    if (value >= cal.center_min && value <= cal.center_max) {
        // In deadzone, return 0
        return 0;
    } else if (value < cal.center_min) {
        // Map from [range_min, center_min] to [-32767, 0]
        double normalized = (static_cast<double>(value) - cal.center_min) / 
                           (cal.center_min - cal.range_min);
        int result = static_cast<int>(normalized * 32767);
        return cal.invert ? -result : result;
    } else { // value > cal.center_max
        // Map from [center_max, range_max] to [0, 32767]
        double normalized = (static_cast<double>(value) - cal.center_max) / 
                           (cal.range_max - cal.center_max);
        int result = static_cast<int>(normalized * 32767);
        return cal.invert ? -result : result;
    }
}

int LibinputJoystick::getAxisState(int id)
{
    if (id >= 0 && id < static_cast<int>(axis_state.size()))
        return axis_state[id];
    else
        return 0;
}

std::vector<LibinputJoystick*> LibinputJoystick::getJoysticks()
{
    std::vector<LibinputJoystick*> joysticks;
    
    // Initialize the LibinputHelper if not already
    LibinputHelper* helper = LibinputHelper::instance();
    if (!helper->initialize()) {
        return joysticks;
    }
    
    // Get all joystick devices
    QVector<LibinputHelper::DeviceInfo> devices = helper->findJoystickDevices();
    
    // Create joystick objects for each device
    for (const auto& device : devices) {
        try {
            LibinputJoystick* joystick = new LibinputJoystick(device.sysPath.toStdString());
            joysticks.push_back(joystick);
        } catch (const std::exception& e) {
            qWarning() << "Failed to create joystick:" << e.what();
        }
    }
    
    return joysticks;
}

std::vector<Joystick::CalibrationData> LibinputJoystick::getCalibration()
{
    std::vector<CalibrationData> cal_data;
    cal_data.resize(axis_count);
    
    for (int i = 0; i < axis_count; i++) {
        cal_data[i].calibrate = false;
        cal_data[i].invert = false;
        cal_data[i].center_min = 0;
        cal_data[i].center_max = 0;
        cal_data[i].range_min = -32767;
        cal_data[i].range_max = 32767;
    }
    
    return cal_data;
}

void LibinputJoystick::setCalibration(const std::vector<CalibrationData>& data)
{
    // Store the calibration data for our internal use
}

void LibinputJoystick::resetCalibration()
{
    // Reset to original calibration
}

void LibinputJoystick::clearCalibration()
{
    // Clear calibration data
}

std::vector<int> LibinputJoystick::getButtonMapping()
{
    return m_button_mapping;
}

std::vector<int> LibinputJoystick::getAxisMapping()
{
    return m_axis_mapping;
}

void LibinputJoystick::setButtonMapping(const std::vector<int>& mapping)
{
    if (mapping.size() == button_count) {
        m_button_mapping = mapping;
    }
}

void LibinputJoystick::setAxisMapping(const std::vector<int>& mapping)
{
    if (mapping.size() == axis_count) {
        m_axis_mapping = mapping;
    }
}

void LibinputJoystick::correctCalibration(const std::vector<int>& mapping_old, const std::vector<int>& mapping_new)
{
    // Handle calibration adjustment after remapping
}

std::string LibinputJoystick::getEvdev() const
{
    return filename;
}
