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

#include "dialogs/joystick_list_dialog.h"

#include <QIcon>
#include <QHeaderView>
#include <sstream>

#include "main.h"
#include "joystick.h"
#include "joystick_factory.h"
#include "joystick_description.h"

JoystickListDialog::JoystickListDialog(QWidget* parent)
    : QDialog(parent),
      m_refresh_button(tr("Refresh")),
      m_properties_button(tr("Properties")),
      m_close_button(tr("Close"))
{
    setWindowTitle(tr("Joystick Preferences"));
    setWindowIcon(QIcon(":/resources/generic.png"));
    resize(450, 310);
    
    // Set up layout
    setLayout(&m_vbox);
    
    scrolled.setWidget(&treeview);
    scrolled.setWidgetResizable(true);
    scrolled.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    
    m_buttonbox.addWidget(&m_refresh_button, 1, Qt::AlignRight);
    m_buttonbox.addWidget(&m_properties_button);
    m_buttonbox.addWidget(&m_close_button);
    
    m_vbox.addWidget(&scrolled);
    m_vbox.addLayout(&m_buttonbox);
    
    // Create model
    device_list = new QStandardItemModel(this);
    device_list->setColumnCount(2);
    
    // Set up tree view
    treeview.setModel(device_list);
    treeview.setHeaderHidden(true);
    treeview.header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    
    // Connect signals
    connect(&treeview, &QTreeView::doubleClicked, this, &JoystickListDialog::onRowActivated);
    connect(&m_refresh_button, &QPushButton::clicked, this, &JoystickListDialog::onRefreshButton);
    connect(&m_properties_button, &QPushButton::clicked, this, &JoystickListDialog::onPropertiesButton);
    connect(&m_close_button, &QPushButton::clicked, this, &QDialog::accept);
    
    m_close_button.setFocus();
    
    onRefreshButton();
}

void
JoystickListDialog::onRowActivated(const QModelIndex& index)
{
    // Get the filename from the model data
    QModelIndex fileIndex = device_list->index(index.row(), 0);
    QVariant fileData = device_list->data(fileIndex, Qt::UserRole);
    
    if (fileData.isValid()) {
        JoystickApp::instance()->showDevicePropertyDialog(fileData.toString(), this);
    }
}

void
JoystickListDialog::onRefreshButton()
{
    // Get joysticks using the factory, which will use the best backend
    const std::vector<JoystickDescription>& joysticks = JoystickFactory::getJoysticks();
    
    device_list->clear();
    device_list->setHorizontalHeaderLabels(QStringList() << "Icon" << "Name");
    
    for(const auto& joystick : joysticks)
    {
        // Create icon item
        QStandardItem* iconItem = new QStandardItem();
        QString iconFilename;
        
        // Pick appropriate icon based on device name
        if (joystick.name.find("PLAYSTATION") != std::string::npos ||
            joystick.name.find("PS3") != std::string::npos ||
            joystick.name.find("PS4") != std::string::npos ||
            joystick.name.find("PS5") != std::string::npos) {
            iconFilename = ":/resources/PS3.png";
        } else if (joystick.name.find("X-Box") != std::string::npos ||
                   joystick.name.find("Xbox") != std::string::npos) {
            iconFilename = ":/resources/xbox360_small.png";
        } else {
            iconFilename = ":/resources/generic.png";
        }
        
        iconItem->setIcon(QIcon(iconFilename));
        iconItem->setData(QString::fromStdString(joystick.filename), Qt::UserRole);
        
        // Create text item
        QStandardItem* textItem = new QStandardItem();
        
        std::ostringstream out;
        out << joystick.name << "\n"
            << "Device: " << joystick.filename << "\n"
            << "Axes: " << joystick.axis_count << "\n"
            << "Buttons: " << joystick.button_count;
        
        // Add backend information if using libinput
        if (joystick.filename.find("/sys/") == 0) {
            out << "\nBackend: libinput";
        }
        
        textItem->setText(QString::fromStdString(out.str()));
        
        // Add items to model
        QList<QStandardItem*> row;
        row << iconItem << textItem;
        device_list->appendRow(row);
    }
    
    // Select first item if available
    if (!joysticks.empty()) {
        treeview.setCurrentIndex(device_list->index(0, 0));
    }
}

void
JoystickListDialog::onPropertiesButton()
{
    QModelIndex index = treeview.currentIndex();
    if (index.isValid()) {
        onRowActivated(index);
    }
}
