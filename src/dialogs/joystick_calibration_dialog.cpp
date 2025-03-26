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

#include "dialogs/joystick_calibration_dialog.h"

#include <iostream>
#include <assert.h>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHeaderView>

#include "joystick.h"
#include "dialogs/calibrate_maximum_dialog.h"

JoystickCalibrationDialog::JoystickCalibrationDialog(Joystick& joystick_)
    : QDialog(nullptr),
      joystick(joystick_),
      label(tr("The <i>center</i> values are the minimum and the maximum values of the deadzone.\n"
               "The <i>min</i> and <i>max</i> values refer to the outer values. You have to unplug\n"
               "your joystick or reboot to reset the values to their original default.\n"
               "\n"
               "To run the calibration wizard, press the <i>Calibrate</i> button.")),
      calibration_button(tr("Start Calibration"))
{
    setWindowTitle("Calibration: " + joystick.getName());
    
    // Set up dialog layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Configure label
    label.setTextFormat(Qt::RichText);
    label.setWordWrap(true);
    mainLayout->addWidget(&label);
    
    // Configure calibration button
    connect(&calibration_button, &QPushButton::clicked, this, &JoystickCalibrationDialog::onCalibrate);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(&calibration_button);
    btnLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->addLayout(btnLayout);
    
    // Configure table
    axis_table.setRowCount(joystick.getAxisCount() + 1);
    axis_table.setColumnCount(6);
    
    QStringList headers;
    headers << tr("Axes") << tr("CenterMin") << tr("CenterMax") 
            << tr("RangeMin") << tr("RangeMax") << tr("Invert");
    axis_table.setHorizontalHeaderLabels(headers);
    
    // First row is headers
    for (int i = 0; i < joystick.getAxisCount(); ++i)
    {
        CalibrationData data;
        
        // Create axis row label
        QString axisLabel = QString::number(i);
        QTableWidgetItem* axisItem = new QTableWidgetItem(axisLabel);
        axis_table.setItem(i+1, 0, axisItem);
        
        // Create input widgets
        data.center_min = new QSpinBox();
        data.center_max = new QSpinBox();
        data.range_min = new QSpinBox();
        data.range_max = new QSpinBox();
        data.invert = new QCheckBox();
        
        // Configure spinboxes
        data.center_min->setRange(-32768, 32767);
        data.center_max->setRange(-32768, 32767);
        data.range_min->setRange(-32768, 32767);
        data.range_max->setRange(-32768, 32767);
        
        // Add tooltips
        data.center_min->setToolTip(tr("The minimal value of the dead zone"));
        data.center_max->setToolTip(tr("The maximum value of the dead zone"));
        data.range_min->setToolTip(tr("The minimal position reachable"));
        data.range_max->setToolTip(tr("The maximum position reachable"));
        
        // Connect signals
        connect(data.center_min, QOverload<int>::of(&QSpinBox::valueChanged), this, &JoystickCalibrationDialog::onApply);
        connect(data.center_max, QOverload<int>::of(&QSpinBox::valueChanged), this, &JoystickCalibrationDialog::onApply);
        connect(data.range_min, QOverload<int>::of(&QSpinBox::valueChanged), this, &JoystickCalibrationDialog::onApply);
        connect(data.range_max, QOverload<int>::of(&QSpinBox::valueChanged), this, &JoystickCalibrationDialog::onApply);
        connect(data.invert, &QCheckBox::clicked, this, &JoystickCalibrationDialog::onApply);
        
        // Add widgets to table
        axis_table.setCellWidget(i+1, 1, data.center_min);
        axis_table.setCellWidget(i+1, 2, data.center_max);
        axis_table.setCellWidget(i+1, 3, data.range_min);
        axis_table.setCellWidget(i+1, 4, data.range_max);
        axis_table.setCellWidget(i+1, 5, data.invert);
        
        calibration_data.append(data);
    }
    
    // Configure scroll area
    scroll.setWidget(&axis_table);
    scroll.setWidgetResizable(true);
    scroll.setPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded, Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
    scroll.setMinimumHeight(300);
    
    // Add table to frame
    axis_frame.setFrameShape(QFrame::StyledPanel);
    axis_frame.setFrameShadow(QFrame::Raised);
    QVBoxLayout* frameLayout = new QVBoxLayout(&axis_frame);
    frameLayout->addWidget(&scroll);
    
    mainLayout->addWidget(&axis_frame);
    
    // Set up buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox;
    QPushButton* revertButton = buttonBox->addButton(tr("Revert to Saved"), QDialogButtonBox::ActionRole);
    QPushButton* rawButton = buttonBox->addButton(tr("Raw Events"), QDialogButtonBox::ActionRole);
    QPushButton* closeButton = buttonBox->addButton(QDialogButtonBox::Close);
    
    mainLayout->addWidget(buttonBox);
    
    // Connect dialog buttons
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(revertButton, &QPushButton::clicked, this, [this](){ onResponse(2); });
    connect(rawButton, &QPushButton::clicked, this, [this](){ onResponse(1); });
    
    closeButton->setFocus();
    
    updateWith(joystick.getCalibration());
}

void
JoystickCalibrationDialog::onClear()
{
    joystick.clearCalibration();
    updateWith(joystick.getCalibration());
}

void
JoystickCalibrationDialog::updateWith(const std::vector<Joystick::CalibrationData>& data)
{
    assert((int)data.size() == calibration_data.size());
    
    for(int i = 0; i < (int)data.size(); ++i)
    {
        calibration_data[i].invert->setChecked(data[i].invert);
        calibration_data[i].center_min->setValue(data[i].center_min);
        calibration_data[i].center_max->setValue(data[i].center_max);
        calibration_data[i].range_min->setValue(data[i].range_min);
        calibration_data[i].range_max->setValue(data[i].range_max);
    }
}

void
JoystickCalibrationDialog::onApply()
{
    std::vector<Joystick::CalibrationData> data(calibration_data.size());
    
    for(int i = 0; i < (int)data.size(); ++i)
    {
        data[i].calibrate  = true;
        data[i].invert     = calibration_data[i].invert->isChecked();
        data[i].center_min = calibration_data[i].center_min->value();
        data[i].center_max = calibration_data[i].center_max->value();
        data[i].range_min  = calibration_data[i].range_min->value();
        data[i].range_max  = calibration_data[i].range_max->value();
    }
    
    joystick.setCalibration(data);
}

void
JoystickCalibrationDialog::onCalibrate()
{
    CalibrateMaximumDialog* dialog = new CalibrateMaximumDialog(joystick);
    dialog->setParent(this);
    dialog->exec();
    delete dialog;
    updateWith(joystick.getCalibration());
}

void
JoystickCalibrationDialog::onResponse(int result)
{
    if (result == 0)
    {
        accept();
    }
    else if (result == 1)
    {
        onClear();
    }
    else if (result == 2)
    {
        joystick.resetCalibration();
        updateWith(joystick.getCalibration());
    }
}
