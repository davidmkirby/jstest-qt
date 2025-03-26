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

#ifndef JSTEST_QT_REMAP_WIDGET_H
#define JSTEST_QT_REMAP_WIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QStandardItemModel>
#include <QTreeView>
#include <QScrollArea>

class Joystick;

class RemapWidget : public QWidget
{
    Q_OBJECT

public:
    enum Mode { REMAP_AXIS, REMAP_BUTTON };

private:
    Joystick& joystick;
    Mode mode;

    QVBoxLayout* layout;
    QTreeView treeview;
    QStandardItemModel* map_list;
    QScrollArea scroll;

public:
    RemapWidget(Joystick& joystick_, Mode mode, QWidget* parent = nullptr);

    void addEntry(int id, const QString& str);

public slots:
    void onClear();
    void onApply();
    void onRowMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);
};

#endif // JSTEST_QT_REMAP_WIDGET_H
