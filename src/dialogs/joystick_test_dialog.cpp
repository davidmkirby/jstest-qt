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

#include "dialogs/joystick_test_dialog.h"

#include <sstream>
#include <iostream>
#include <QIcon>

#include "main.h"
#include "joystick.h"
#include "widgets/button_widget.h"

JoystickTestDialog::JoystickTestDialog(JoystickGui& gui, Joystick& joystick_, bool simple_ui)
    : QDialog(nullptr),
      m_gui(gui),
      joystick(joystick_),
      m_simple_ui(simple_ui),
      label("<b>" + joystick.getName() + "</b><br>Device: " + QString::fromStdString(joystick.getFilename())),
      stick1_widget(128, 128),
      stick2_widget(128, 128),
      stick3_widget(128, 128),
      rudder_widget(128, 32),
      throttle_widget(32, 128),
      left_trigger_widget(32, 128, true),
      right_trigger_widget(32, 128, true)
{
    setWindowTitle(joystick_.getName());
    setWindowIcon(QIcon(":/resources/generic.png"));
    
    // Main layout setup
    setLayout(&m_vbox);
    
    // Title label
    label.setTextFormat(Qt::RichText);
    label.setTextInteractionFlags(Qt::TextSelectableByMouse);
    
    // Frames and layouts
    axis_frame.setFrameShape(QFrame::StyledPanel);
    axis_frame.setFrameShadow(QFrame::Raised);
    axis_frame.setLayout(&axis_vbox);
    
    button_frame.setFrameShape(QFrame::StyledPanel);
    button_frame.setFrameShadow(QFrame::Raised);
    button_frame.setLayout(&button_grid);
    
    // Add padding to the label
    QHBoxLayout* labelLayout = new QHBoxLayout(&alignment);
    labelLayout->addWidget(&label);
    labelLayout->setContentsMargins(8, 8, 8, 8);
    
    // Set up axis grid
    axis_grid.setSpacing(5);
    button_grid.setSpacing(8);
    
    // Vector to store value labels for each axis
    axis_value_labels.resize(joystick.getAxisCount());
    
    for(int i = 0; i < joystick.getAxisCount(); ++i)
    {
        QString labelText = QString("Axis %1: ").arg(i);
        QLabel* axisLabel = new QLabel(labelText);
        
        QProgressBar* progressbar = new QProgressBar();
        progressbar->setValue(50);
        
        // Create a label to show the axis value
        axis_value_labels[i] = new QLabel("0");
        axis_value_labels[i]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        axis_value_labels[i]->setFixedWidth(60);  // Fixed width for consistent display
        
        // Each column must have at most 10 axes
        int x = (i/10)*3;  // *3 instead of *2 to make room for value labels
        int y = i%10;
        
        axis_grid.addWidget(axisLabel, y, x);
        axis_grid.addWidget(progressbar, y, x+1);
        axis_grid.addWidget(axis_value_labels[i], y, x+2);
        
        axes.push_back(progressbar);
    }
    
    for(int i = 0; i < joystick.getButtonCount(); ++i)
    {
        int x = i / 10;
        int y = i % 10;
        
        ButtonWidget* button = new ButtonWidget(32, 32, QString::number(i));
        button_grid.addWidget(button, y, x);
        buttons.push_back(button);
    }
    
    // Button box
    buttonbox.addWidget(&mapping_button);
    buttonbox.addWidget(&calibration_button);
    buttonbox.addWidget(&close_button);
    
    mapping_button.setText(tr("Mapping"));
    calibration_button.setText(tr("Calibration"));
    close_button.setText(tr("Close"));
    
    // Layout construction
    test_hbox.addWidget(&axis_frame);
    test_hbox.addWidget(&button_frame);
    
    m_vbox.addWidget(&alignment);
    m_vbox.addLayout(&test_hbox);
    m_vbox.addLayout(&buttonbox);
    
    stick_hbox.setContentsMargins(5, 5, 5, 5);
    
    axis_callbacks.clear();
    raw_value_callbacks.clear();
    // Initialize callbacks
    for(int i = 0; i < joystick.getAxisCount(); ++i) {
        axis_callbacks.push_back([](double){});
        raw_value_callbacks.push_back([](int){});
    }
    
    // Check if this is a Thrustmaster HOTAS Warthog or always add stick widget for first two axes
    bool is_thrustmaster = joystick.getName().contains("Thrustmaster", Qt::CaseInsensitive);
    bool has_at_least_two_axes = (joystick.getAxisCount() >= 2);
    
    // Always show main stick widget for first two axes if device has at least 2 axes
    if (has_at_least_two_axes) {
        stick_hbox.addWidget(&stick1_widget, 1, Qt::AlignCenter);
        raw_value_callbacks[0] = [this](int val) { stick1_widget.setRawX(val); };
        raw_value_callbacks[1] = [this](int val) { stick1_widget.setRawY(val); };
    }
    
    // Continue with the regular joystick type handling
    switch(joystick.getAxisCount())
    {
    case 2: // Simple stick
        // Already handled above
        break;
    
    case 6: // Flightstick
    {
        QWidget* container = new QWidget();
        QGridLayout* gridLayout = new QGridLayout(container);
        
        // Don't add stick1_widget again since we already did it above
        gridLayout->addWidget(&rudder_widget, 1, 0);
        gridLayout->addWidget(&throttle_widget, 0, 1);
        
        stick_hbox.addWidget(container, 1, Qt::AlignCenter);
        stick_hbox.addWidget(&stick3_widget, 1, Qt::AlignCenter);
        
        // Don't connect stick1_widget again
        axis_callbacks[2] = [this](double val) { rudder_widget.setPos(val); };
        axis_callbacks[3] = [this](double val) { throttle_widget.setPos(val); };
        raw_value_callbacks[4] = [this](int val) { stick3_widget.setRawX(val); };
        raw_value_callbacks[5] = [this](int val) { stick3_widget.setRawY(val); };
        break;
    }
    
    case 8: // Dual Analog Gamepad + Analog Trigger
        // Don't add stick1_widget again
        stick_hbox.addWidget(&stick2_widget, 1, Qt::AlignCenter);
        stick_hbox.addWidget(&stick3_widget, 1, Qt::AlignCenter);
        stick_hbox.addWidget(&left_trigger_widget, 1, Qt::AlignCenter);
        stick_hbox.addWidget(&right_trigger_widget, 1, Qt::AlignCenter);
        
        // Don't connect stick1_widget again
        raw_value_callbacks[2] = [this](int val) { stick2_widget.setRawX(val); };
        raw_value_callbacks[3] = [this](int val) { stick2_widget.setRawY(val); };
        raw_value_callbacks[6] = [this](int val) { stick3_widget.setRawX(val); };
        raw_value_callbacks[7] = [this](int val) { stick3_widget.setRawY(val); };
        axis_callbacks[4] = [this](double val) { left_trigger_widget.setPos(val); };
        axis_callbacks[5] = [this](double val) { right_trigger_widget.setPos(val); };
        break;
    
    case 7: // Dual Analog Gamepad DragonRise Inc. Generic USB Joystick
        // Don't add stick1_widget again
        stick_hbox.addWidget(&stick2_widget, 1, Qt::AlignCenter);
        stick_hbox.addWidget(&stick3_widget, 1, Qt::AlignCenter);
        
        // Don't connect stick1_widget again
        raw_value_callbacks[3] = [this](int val) { stick2_widget.setRawX(val); };
        raw_value_callbacks[4] = [this](int val) { stick2_widget.setRawY(val); };
        raw_value_callbacks[5] = [this](int val) { stick3_widget.setRawX(val); };
        raw_value_callbacks[6] = [this](int val) { stick3_widget.setRawY(val); };
        break;
    
    case 27: // Playstation 3 Controller
        // Don't add stick1_widget again
        stick_hbox.addWidget(&stick2_widget, 1, Qt::AlignCenter);
        // Not using stick3 for now, as the dpad is 4 axis on the PS3, not 2 (one for each direction)
        stick_hbox.addWidget(&left_trigger_widget, 1, Qt::AlignCenter);
        stick_hbox.addWidget(&right_trigger_widget, 1, Qt::AlignCenter);
        
        // Don't connect stick1_widget again
        raw_value_callbacks[2] = [this](int val) { stick2_widget.setRawX(val); };
        raw_value_callbacks[3] = [this](int val) { stick2_widget.setRawY(val); };
        axis_callbacks[12] = [this](double val) { left_trigger_widget.setPos(val); };
        axis_callbacks[13] = [this](double val) { right_trigger_widget.setPos(val); };
        break;
    
    default:
        // Do nothing here, we already added stick1_widget for the first two axes
        std::cout << "Using default circular display for first two axes" << std::endl;
        break;
    }
    
    // Always show the stick widgets unless simple_ui is enabled
    if (!m_simple_ui)
    {
        axis_vbox.addLayout(&stick_hbox);
    }
    
    axis_vbox.addLayout(&axis_grid);
    
    // Connect signals
    connect(&joystick, &Joystick::axisChanged, this, &JoystickTestDialog::axisMove);
    connect(&joystick, &Joystick::buttonChanged, this, &JoystickTestDialog::buttonMove);
    
    connect(&calibration_button, &QPushButton::clicked, this, &JoystickTestDialog::onCalibrate);
    connect(&mapping_button, &QPushButton::clicked, this, &JoystickTestDialog::onMapping);
    connect(&close_button, &QPushButton::clicked, this, &QDialog::accept);
    
    close_button.setFocus();
}

void
JoystickTestDialog::axisMove(int number, int value)
{
    // Update progress bar
    axes.at(number)->setValue((value + 32767) / 655.34);
    
    // Update progress bar text with the value
    QString strValue = QString::number(value);
    axes.at(number)->setFormat(strValue);
    
    // Update value label
    axis_value_labels.at(number)->setText(strValue);
    
    // Update other widgets
    axis_callbacks[number](value / 32767.0);
    raw_value_callbacks[number](value);
}

void
JoystickTestDialog::buttonMove(int number, bool value)
{
    buttons.at(number)->setDown(value);
}

void
JoystickTestDialog::onCalibrate()
{
    m_gui.showCalibrationDialog();
}

void
JoystickTestDialog::onMapping()
{
    m_gui.showMappingDialog();
}
