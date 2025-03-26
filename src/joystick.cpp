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

#include "joystick.h"

#include <assert.h>
#include <math.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <QFile>
#include <QDir>

#include "utils/evdev_helper.h"

Joystick::Joystick(const std::string& filename_)
    : QObject(nullptr),
      filename(filename_)
{
    if ((fd = open(filename.c_str(), O_RDONLY)) < 0)
    {
        QString errorMsg = QString("%1: %2").arg(QString::fromStdString(filename)).arg(strerror(errno));
        throw std::runtime_error(errorMsg.toStdString());
    }
    else
    {
        // ok
        uint8_t num_axis   = 0;
        uint8_t num_button = 0;
        ioctl(fd, JSIOCGAXES,    &num_axis);
        ioctl(fd, JSIOCGBUTTONS, &num_button);
        axis_count   = num_axis;
        button_count = num_button;

        // Get Name
        char name_c_str[1024];
        if (ioctl(fd, JSIOCGNAME(sizeof(name_c_str)), name_c_str) < 0)
        {
            QString errorMsg = QString("%1: %2").arg(QString::fromStdString(filename)).arg(strerror(errno));
            throw std::runtime_error(errorMsg.toStdString());
        }
        else
        {
            orig_name = name_c_str;
            name = QString::fromLatin1(name_c_str);
        }

        axis_state.resize(axis_count);
    }

    orig_calibration_data = getCalibration();

    notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, &Joystick::onSocketActivated);
}

Joystick::~Joystick()
{
    delete notifier;
    close(fd);
}

void
Joystick::onSocketActivated(int socket)
{
    if (socket == fd) {
        update();
    }
}

void
Joystick::update()
{
    struct js_event event;

    ssize_t len = read(fd, &event, sizeof(event));

    if (len < 0)
    {
        QString errorMsg = QString("%1: %2").arg(QString::fromStdString(filename)).arg(strerror(errno));
        throw std::runtime_error(errorMsg.toStdString());
    }
    else if (len == sizeof(event))
    { // ok
        if (event.type & JS_EVENT_AXIS)
        {
            //std::cout << "Axis: " << (int)event.number << " -> " << (int)event.value << std::endl;
            axis_state[event.number] = event.value;
            emit axisChanged(event.number, event.value);
        }
        else if (event.type & JS_EVENT_BUTTON)
        {
            //std::cout << "Button: " << (int)event.number << " -> " << (int)event.value << std::endl;
            emit buttonChanged(event.number, event.value);
        }
    }
    else
    {
        throw std::runtime_error("Joystick::update(): unknown read error");
    }
}

std::vector<JoystickDescription>
Joystick::getJoysticks()
{
    std::vector<JoystickDescription> joysticks;

    // Traditional method - try the joystick devices directly
    for(int i = 0; i < 32; ++i)
    {
        try
        {
            QDir devDir("/dev/input");
            QString joystickPath = QString("js%1").arg(i);
            
            if (devDir.exists(joystickPath))
            {
                QString fullPath = devDir.filePath(joystickPath);
                Joystick joystick(fullPath.toStdString());

                joysticks.push_back(JoystickDescription(joystick.getFilename(),
                                                       joystick.getName().toStdString(),
                                                       joystick.getAxisCount(),
                                                       joystick.getButtonCount()));
            }
        }
        catch(std::exception& err)
        {
            // ok
        }
    }

    // If no joysticks found using the traditional method, try alternative detection
    if (joysticks.empty())
    {
        // First try with evdev directly
        QDir evdevDir("/dev/input");
        QStringList filters;
        filters << "event*";
        evdevDir.setNameFilters(filters);
        QStringList eventDevices = evdevDir.entryList();
        
        for (const QString& eventDevice : eventDevices)
        {
            QString fullPath = evdevDir.filePath(eventDevice);
            int fd = open(fullPath.toStdString().c_str(), O_RDONLY | O_NONBLOCK);
            
            if (fd >= 0)
            {
                uint8_t evbit[EV_MAX/8 + 1];
                memset(evbit, 0, sizeof(evbit));
                
                if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) >= 0)
                {
                    // Check if this device has joystick-like capabilities
                    if ((evbit[EV_ABS/8] & (1 << (EV_ABS % 8))) &&
                        (evbit[EV_KEY/8] & (1 << (EV_KEY % 8))))
                    {
                        // Looks like it might be a joystick - get more information
                        char name[256] = "Unknown";
                        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0)
                        {
                            // Get axis and button counts
                            uint8_t absbit[ABS_MAX/8 + 1];
                            uint8_t keybit[KEY_MAX/8 + 1];
                            memset(absbit, 0, sizeof(absbit));
                            memset(keybit, 0, sizeof(keybit));
                            
                            ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit);
                            ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit);
                            
                            // Count axes and buttons
                            int axis_count = 0;
                            for (int i = 0; i < ABS_MAX; i++) {
                                if (absbit[i/8] & (1 << (i % 8))) {
                                    axis_count++;
                                }
                            }
                            
                            int button_count = 0;
                            for (int i = BTN_JOYSTICK; i < KEY_MAX; i++) {
                                if (keybit[i/8] & (1 << (i % 8))) {
                                    button_count++;
                                }
                            }
                            
                            if (axis_count > 0 && button_count > 0) {
                                // Transform evdev path to joystick path if possible
                                QDir jsDevDir("/dev/input");
                                QString joystickPath;
                                
                                // Find corresponding js device by matching device name
                                for (int i = 0; i < 32; i++) {
                                    QString jsPath = QString("js%1").arg(i);
                                    if (jsDevDir.exists(jsPath)) {
                                        QString fullJsPath = jsDevDir.filePath(jsPath);
                                        int js_fd = open(fullJsPath.toStdString().c_str(), O_RDONLY);
                                        if (js_fd >= 0) {
                                            char js_name[256] = "";
                                            if (ioctl(js_fd, JSIOCGNAME(sizeof(js_name)), js_name) >= 0) {
                                                if (strcmp(name, js_name) == 0) {
                                                    joystickPath = fullJsPath;
                                                    close(js_fd);
                                                    break;
                                                }
                                            }
                                            close(js_fd);
                                        }
                                    }
                                }
                                
                                // If no matching js device found, use the event device
                                if (joystickPath.isEmpty()) {
                                    joystickPath = fullPath;
                                }
                                
                                joysticks.push_back(JoystickDescription(joystickPath.toStdString(),
                                                                      name,
                                                                      axis_count,
                                                                      button_count));
                            }
                        }
                    }
                }
                close(fd);
            }
        }
    }

    return joysticks;
}

Joystick::CalibrationData corr2cal(const struct js_corr& corr_)
{
    struct js_corr corr = corr_;

    Joystick::CalibrationData data;

    if (corr.type)
    {
        data.calibrate = true;
        data.invert    = (corr.coef[2] < 0 && corr.coef[3] < 0);
        data.center_min = corr.coef[0];
        data.center_max = corr.coef[1];

        if (data.invert)
        {
            corr.coef[2] = -corr.coef[2];
            corr.coef[3] = -corr.coef[3];
        }

        // Need to use double and rint(), since calculation doesn't end
        // up on clean integer positions (i.e. 0.9999 can happen)
        data.range_min = rint(data.center_min - ((32767.0 * 16384) / corr.coef[2]));
        data.range_max = rint((32767.0 * 16384) / corr.coef[3] + data.center_max);
    }
    else
    {
        data.calibrate  = false;
        data.invert     = false;
        data.center_min = 0;
        data.center_max = 0;
        data.range_min  = 0;
        data.range_max  = 0;
    }

    return data;
}

std::vector<Joystick::CalibrationData>
Joystick::getCalibration()
{
    std::vector<struct js_corr> corr(getAxisCount());

    if (ioctl(fd, JSIOCGCORR, &*corr.begin()) < 0)
    {
        QString errorMsg = QString("%1: %2").arg(QString::fromStdString(filename)).arg(strerror(errno));
        throw std::runtime_error(errorMsg.toStdString());
    }
    else
    {
        std::vector<CalibrationData> data;
        std::transform(corr.begin(), corr.end(), std::back_inserter(data), corr2cal);
        return data;
    }
}

struct js_corr cal2corr(const Joystick::CalibrationData& data)
{
    struct js_corr corr;

    if (data.calibrate &&
        (data.center_min - data.range_min)  != 0 &&
        (data.range_max  - data.center_max) != 0)
    {
        corr.type = 1;
        corr.prec = 0;
        corr.coef[0] = data.center_min;
        corr.coef[1] = data.center_max;

        corr.coef[2] = (32767 * 16384) / (data.center_min - data.range_min);
        corr.coef[3] = (32767 * 16384) / (data.range_max  - data.center_max);

        if (data.invert)
        {
            corr.coef[2] = -corr.coef[2];
            corr.coef[3] = -corr.coef[3];
        }
    }
    else
    {
        corr.type = 0;
        corr.prec = 0;
        memset(corr.coef, 0, sizeof(corr.coef));
    }

    return corr;
}

void
Joystick::setCalibration(const std::vector<CalibrationData>& data)
{
    std::vector<struct js_corr> corr;

    std::transform(data.begin(), data.end(), std::back_inserter(corr), cal2corr);

    if (ioctl(fd, JSIOCSCORR, &*corr.begin()) < 0)
    {
        QString errorMsg = QString("%1: %2").arg(QString::fromStdString(filename)).arg(strerror(errno));
        throw std::runtime_error(errorMsg.toStdString());
    }
}

void
Joystick::clearCalibration()
{
    std::vector<CalibrationData> data;

    for(int i = 0; i < getAxisCount(); ++i)
    {
        CalibrationData cal;

        cal.calibrate  = false;
        cal.invert     = false;
        cal.center_min = 0;
        cal.center_max = 0;
        cal.range_min  = 0;
        cal.range_max  = 0;

        data.push_back(cal);
    }

    setCalibration(data);
}

void
Joystick::resetCalibration()
{
    setCalibration(orig_calibration_data);
}

std::vector<int>
Joystick::getButtonMapping()
{
    uint16_t btnmap[KEY_MAX - BTN_MISC + 1];
    if (ioctl(fd, JSIOCGBTNMAP, btnmap) < 0)
    {
        QString errorMsg = QString("%1: %2").arg(QString::fromStdString(filename)).arg(strerror(errno));
        throw std::runtime_error(errorMsg.toStdString());
    }
    else
    {
        std::vector<int> mapping;
        std::copy(btnmap, btnmap + button_count, std::back_inserter(mapping));
        return mapping;
    }
}

std::vector<int>
Joystick::getAxisMapping()
{
    uint8_t axismap[ABS_MAX + 1];
    if (ioctl(fd, JSIOCGAXMAP, axismap) < 0)
    {
        QString errorMsg = QString("%1: %2").arg(QString::fromStdString(filename)).arg(strerror(errno));
        throw std::runtime_error(errorMsg.toStdString());
    }
    else
    {
        std::vector<int> mapping;
        std::copy(axismap, axismap + axis_count, std::back_inserter(mapping));
        return mapping;
    }
}

void
Joystick::setButtonMapping(const std::vector<int>& mapping)
{
    assert((int)mapping.size() == button_count);

    uint16_t btnmap[KEY_MAX - BTN_MISC + 1];
    memset(btnmap, 0, sizeof(btnmap));
    std::copy(mapping.begin(), mapping.end(), btnmap);

    if (0)
        for(int i = 0; i < button_count; ++i)
        {
            std::cout << i << " -> " << btnmap[i] << std::endl;
        }

    if (ioctl(fd, JSIOCSBTNMAP, btnmap) < 0)
    {
        QString errorMsg = QString("%1: %2").arg(QString::fromStdString(filename)).arg(strerror(errno));
        throw std::runtime_error(errorMsg.toStdString());
    }
}

int
Joystick::getAxisState(int id)
{
    if (id >= 0 && id < (int)axis_state.size())
        return axis_state[id];
    else
        return 0;
}

void
Joystick::setAxisMapping(const std::vector<int>& mapping)
{
    assert((int)mapping.size() == axis_count);

    uint8_t axismap[ABS_MAX + 1];

    std::copy(mapping.begin(), mapping.end(), axismap);

    if (ioctl(fd, JSIOCSAXMAP, axismap) < 0)
    {
        QString errorMsg = QString("%1: %2").arg(QString::fromStdString(filename)).arg(strerror(errno));
        throw std::runtime_error(errorMsg.toStdString());
    }
}

void
Joystick::correctCalibration(const std::vector<int>& mapping_old, const std::vector<int>& mapping_new)
{
    int axes[ABS_MAX + 1]; // axes[name] -> old_idx
    for(std::vector<int>::const_iterator i = mapping_old.begin(); i != mapping_old.end(); ++i)
    {
        axes[*i] = i - mapping_old.begin();
    }

    std::vector<CalibrationData> callib_old = getCalibration();
    std::vector<CalibrationData> callib_new;
    for(std::vector<int>::const_iterator i = mapping_new.begin(); i != mapping_new.end(); ++i)
    {
        callib_new.push_back(callib_old[axes[*i]]);
    }

    setCalibration(callib_new);
}

std::string
Joystick::getEvdev() const
{
    // See /usr/share/doc/linux-doc-2.6.28/devices.txt.gz
    for(int i = 0; i < 32; ++i)
    {
        QString eventPath = QString("/dev/input/event%1").arg(i);

        int evdev_fd;
        if ((evdev_fd = open(eventPath.toStdString().c_str(), O_RDONLY)) < 0)
        {
            // ignore
        }
        else
        {
            char evdev_name[256];
            if (ioctl(evdev_fd, EVIOCGNAME(sizeof(evdev_name)), evdev_name) < 0)
            {
                std::cout << eventPath.toStdString() << ": " << strerror(errno) << std::endl;
            }
            else
            {
                if (orig_name == evdev_name)
                {
                    // Found a device that matches, so return it
                    close(evdev_fd);
                    return eventPath.toStdString();
                }
            }

            close(evdev_fd);
        }
    }

    throw std::runtime_error("couldn't find evdev for " + filename);
}
