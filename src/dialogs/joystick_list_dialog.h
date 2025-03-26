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

#ifndef JSTEST_QT_JOYSTICK_LIST_DIALOG_H
#define JSTEST_QT_JOYSTICK_LIST_DIALOG_H

#include <QDialog>
#include <QTreeView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStandardItemModel>
#include <QScrollArea>

class JoystickListDialog : public QDialog
{
    Q_OBJECT

private:
    QVBoxLayout m_vbox;
    QScrollArea scrolled;
    QTreeView treeview;

    QHBoxLayout m_buttonbox;
    QPushButton m_refresh_button;
    QPushButton m_properties_button;
    QPushButton m_close_button;

    QStandardItemModel* device_list;

private slots:
    void onRefreshButton();
    void onPropertiesButton();
    void onRowActivated(const QModelIndex& index);

public:
    JoystickListDialog(QWidget* parent = nullptr);
};

#endif // JSTEST_QT_JOYSTICK_LIST_DIALOG_H
