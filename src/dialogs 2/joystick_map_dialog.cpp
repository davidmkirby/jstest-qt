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

#include "dialogs/joystick_map_dialog.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <sstream>

#include "joystick.h"
#include "utils/evdev_helper.h"

JoystickMapDialog::JoystickMapDialog(Joystick& joystick, QWidget* parent)
    : QDialog(parent),
      label(tr("Change the order of axis and button. The order applies directly to the "
               "joystick kernel driver, so it will work in any game, it is however not "
               "persistant across reboots.")),
      axis_map(joystick, RemapWidget::REMAP_AXIS),
      button_map(joystick, RemapWidget::REMAP_BUTTON)
{
    setWindowTitle("Mapping: " + joystick.getName());
    
    label.setWordWrap(true);
    
    // Set up dialog layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(&label);
    
    // Add remap widgets
    hbox.addWidget(&axis_map);
    hbox.addWidget(&button_map);
    mainLayout->addLayout(&hbox);
    
    // Add buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    revertButton = buttonBox->addButton(tr("Revert to Default"), QDialogButtonBox::ActionRole);
    closeButton = buttonBox->button(QDialogButtonBox::Close);
    mainLayout->addWidget(buttonBox);
    
    // Add button and axis mappings
    const std::vector<int>& button_mapping = joystick.getButtonMapping();
    for(size_t i = 0; i < button_mapping.size(); ++i)
    {
        std::ostringstream s;
        s << i << ": " << btn2str(button_mapping[i]);
        button_map.addEntry(button_mapping[i], QString::fromStdString(s.str()));
    }
    
    const std::vector<int>& axis_mapping = joystick.getAxisMapping();
    for(size_t i = 0; i < axis_mapping.size(); ++i)
    {
        std::ostringstream s;
        s << i << ": " << abs2str(axis_mapping[i]);
        axis_map.addEntry(axis_mapping[i], QString::fromStdString(s.str()));
    }
    
    // Connect signals
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(revertButton, &QPushButton::clicked, this, [this](){ onResponse(1); });
    
    closeButton->setFocus();
}

void
JoystickMapDialog::onResponse(int result)
{
    if (result == 0)
    {
        accept();
    }
    else if (result == 1)
    {
        button_map.onClear();
        axis_map.onClear();
    }
}
