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

#include "dialogs/calibrate_maximum_dialog.h"

#include <iostream>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <algorithm>

#include "joystick.h"

CalibrateMaximumDialog::CalibrateMaximumDialog(Joystick& joystick_, QWidget* parent)
    : QDialog(parent),
      joystick(joystick_),
      orig_data(joystick.getCalibration()),
      label(tr("1) Rotate your joystick around to move all axis into their extreme positions at least once\n"
               "2) Move all axis back to the center\n"
               "3) Press ok\n"))
{
    setWindowTitle("CalibrationWizard: " + joystick.getName());
    
    joystick.clearCalibration();
    
    // Set up dialog layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    
    // Configure label
    label.setWordWrap(true);
    mainLayout->addWidget(&label);
    
    // Create button box
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    
    // Connect signals
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this](){ onDone(0); });
    connect(buttonBox, &QDialogButtonBox::rejected, this, [this](){ onDone(1); });
    
    // Connect to joystick axis change signal
    connection = connect(&joystick, &Joystick::axisChanged, this, &CalibrateMaximumDialog::onAxisMove);
    
    // Initialize tracking variables
    is_init_axis_state.resize(joystick.getAxisCount());
    min_axis_state.resize(joystick.getAxisCount());
    max_axis_state.resize(joystick.getAxisCount());
    
    for(int i = 0; i < joystick.getAxisCount(); ++i)
    {
        is_init_axis_state[i] = false;
        min_axis_state[i] = joystick.getAxisState(i);
        max_axis_state[i] = joystick.getAxisState(i);
    }
}

void
CalibrateMaximumDialog::onDone(int result)
{
    if (result == 0)
    {
        // Calculate CalibrationData
        std::vector<Joystick::CalibrationData> data;
        
        for(int i = 0; i < joystick.getAxisCount(); ++i)
        {
            Joystick::CalibrationData axis;
            axis.calibrate  = true;
            axis.invert     = false;
            axis.center_min = axis.center_max = joystick.getAxisState(i);
            axis.range_min  = min_axis_state[i];
            axis.range_max  = max_axis_state[i];
            
            // When the center is the same as the outer edge of an axis,
            // we assume its a throttle control or analog button and
            // calculate the center on our own
            if (axis.center_min == axis.range_min ||
                axis.center_max == axis.range_max)
            {
                axis.center_min = axis.center_max = (axis.range_min + axis.range_max)/2;
            }
            
            data.push_back(axis);
        }
        
        joystick.setCalibration(data);
        
        accept();
    }
    else
    {
        joystick.setCalibration(orig_data);
        reject();
    }
    
    // Disconnect from joystick signals
    disconnect(connection);
}

void
CalibrateMaximumDialog::onAxisMove(int id, int value)
{
    // std::cout << "AxisMove: " << id << " " << value << std::endl;
    if (!is_init_axis_state[id])
    {
        min_axis_state[id] = value;
        max_axis_state[id] = value;
        is_init_axis_state[id] = true;
    }
    else
    {
        min_axis_state[id] = std::min(value, min_axis_state[id]);
        max_axis_state[id] = std::max(value, max_axis_state[id]);
    }
}
