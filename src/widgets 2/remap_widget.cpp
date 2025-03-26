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

#include "widgets/remap_widget.h"

#include <iostream>
#include <algorithm>
#include <QHeaderView>

#include "joystick.h"

RemapWidget::RemapWidget(Joystick& joystick_, Mode mode_, QWidget* parent)
    : QWidget(parent),
      joystick(joystick_),
      mode(mode_)
{
    // Set up layout
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    
    // Create model
    map_list = new QStandardItemModel(this);
    map_list->setColumnCount(1);
    
    // Set up tree view
    treeview.setModel(map_list);
    treeview.setHeaderHidden(true);
    treeview.setDragEnabled(true);
    treeview.setAcceptDrops(true);
    treeview.setDropIndicatorShown(true);
    treeview.setDragDropMode(QAbstractItemView::InternalMove);
    
    // Set up scroll area
    scroll.setWidget(&treeview);
    scroll.setWidgetResizable(true);
    scroll.setMinimumSize(200, 300);
    scroll.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    
    layout->addWidget(&scroll);
    
    // Set header text based on mode
    if (mode == REMAP_AXIS)
        map_list->setHeaderData(0, Qt::Horizontal, tr("Axes"));
    else
        map_list->setHeaderData(0, Qt::Horizontal, tr("Buttons"));
    
    // Connect signals
    connect(map_list, &QStandardItemModel::rowsMoved, this, &RemapWidget::onRowMoved);
}

void
RemapWidget::addEntry(int id, const QString& str)
{
    QStandardItem* item = new QStandardItem(str);
    item->setData(id, Qt::UserRole);
    map_list->appendRow(item);
}

struct RemapEntry {
    int id;
    QString name;
    
    bool operator<(const RemapEntry& rhs) const {
        return id < rhs.id;
    }
};

void
RemapWidget::onClear()
{
    // We simple sort the list here by 'id'
    
    // Convert the model into a vector
    QVector<RemapEntry> rows;
    for(int i = 0; i < map_list->rowCount(); ++i)
    {
        RemapEntry entry;
        QStandardItem* item = map_list->item(i);
        entry.id = item->data(Qt::UserRole).toInt();
        entry.name = item->text();
        rows.append(entry);
    }
    
    // Sort the vector
    std::sort(rows.begin(), rows.end());
    
    // Reenter the vector into the model
    map_list->clear();
    for(const auto& entry : rows)
    {
        addEntry(entry.id, entry.name);
    }
    
    onApply();
}

void
RemapWidget::onApply()
{
    std::vector<int> mapping;
    for(int i = 0; i < map_list->rowCount(); ++i)
    {
        QStandardItem* item = map_list->item(i);
        mapping.push_back(item->data(Qt::UserRole).toInt());
    }
    
    if (mode == REMAP_AXIS)
    {
        std::vector<int> mapping_old = joystick.getAxisMapping();
        joystick.setAxisMapping(mapping);
        joystick.correctCalibration(mapping_old, mapping);
    }
    else if (mode == REMAP_BUTTON)
    {
        joystick.setButtonMapping(mapping);
    }
}

void
RemapWidget::onRowMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row)
{
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    Q_UNUSED(destination);
    Q_UNUSED(row);
    
    // Check if we have the proper number of items
    if (mode == REMAP_AXIS)
    {
        if (joystick.getAxisCount() == map_list->rowCount())
        {
            onApply();
        }
    }
    else if (mode == REMAP_BUTTON)
    {
        if (joystick.getButtonCount() == map_list->rowCount())
        {
            onApply();
        }
    }
}
