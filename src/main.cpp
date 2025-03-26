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

#include "main.h"

#include <iostream>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

#include "joystick.h"
#include "dialogs/joystick_test_dialog.h"
#include "dialogs/joystick_list_dialog.h"
#include "dialogs/joystick_map_dialog.h"
#include "dialogs/joystick_calibration_dialog.h"

// Static member initialization
JoystickApp* JoystickApp::m_instance = nullptr;

JoystickGui::JoystickGui(std::unique_ptr<Joystick> joystick, bool simple_ui, QWidget* parent) :
    QObject(nullptr),
    m_joystick(std::move(joystick)),
    m_test_dialog(),
    m_mapping_dialog(),
    m_calibration_dialog()
{
    m_test_dialog = std::make_unique<JoystickTestDialog>(*this, *m_joystick, simple_ui);
    if (parent) {
        m_test_dialog->setParent(parent);
    }

    m_test_dialog->show();
}

void
JoystickGui::showCalibrationDialog()
{
    if (m_calibration_dialog) {
        m_calibration_dialog->activateWindow();
    } else {
        m_calibration_dialog = std::make_unique<JoystickCalibrationDialog>(*m_joystick);
        connect(m_calibration_dialog.get(), &QDialog::finished, this, [this]() {
            m_calibration_dialog.reset();
        });
        m_calibration_dialog->setParent(m_test_dialog.get());
        m_calibration_dialog->show();
    }
}

void
JoystickGui::showMappingDialog()
{
    if (m_mapping_dialog) {
        m_mapping_dialog->activateWindow();
    } else {
        m_mapping_dialog = std::make_unique<JoystickMapDialog>(*m_joystick);
        connect(m_mapping_dialog.get(), &QDialog::finished, this, [this]() {
            m_mapping_dialog.reset();
        });
        m_mapping_dialog->setParent(m_test_dialog.get());
        m_mapping_dialog->show();
    }
}

JoystickApp::JoystickApp(int& argc, char** argv) :
    QApplication(argc, argv),
    m_datadir("resources/"),
    m_simple_ui(false),
    m_joystick_guis()
{
    m_instance = this;
    setApplicationName("jstest-qt");
    setApplicationVersion("0.1.1");
}

JoystickApp::~JoystickApp()
{
}

JoystickTestDialog*
JoystickApp::showDevicePropertyDialog(const QString& filename, QWidget* parent)
{
    auto it = m_joystick_guis.find(filename);
    if (it != m_joystick_guis.end()) {
        JoystickTestDialog* dialog = it.value()->getTestDialog();
        dialog->activateWindow();
        return dialog;
    } else {
        try {
            std::unique_ptr<Joystick> joystick(new Joystick(filename.toStdString()));
            std::unique_ptr<JoystickGui> gui(new JoystickGui(std::move(joystick), m_simple_ui, parent));

            JoystickTestDialog* dialog = gui->getTestDialog();
            connect(dialog, &QDialog::finished, this, [this, filename]() {
                m_joystick_guis.remove(filename);
            });

            m_joystick_guis[filename] = std::move(gui);
            return dialog;
        } catch (const std::exception& e) {
            QMessageBox::critical(nullptr, "Error", QString("Error: %1").arg(e.what()));
            return nullptr;
        }
    }
}

int
JoystickApp::run()
{
    QCommandLineParser parser;
    parser.setApplicationDescription("A Qt joystick tester");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption simpleOption("simple", "Hide graphical representation of axis");
    parser.addOption(simpleOption);
    
    QCommandLineOption datadirOption(QStringList() << "datadir", "Load application data from DIR", "dir");
    parser.addOption(datadirOption);
    
    parser.process(*this);
    
    if (parser.isSet(simpleOption)) {
        m_simple_ui = true;
    }
    
    if (parser.isSet(datadirOption)) {
        m_datadir = parser.value(datadirOption);
        if (!m_datadir.endsWith('/')) {
            m_datadir += '/';
        }
    }
    
    QStringList args = parser.positionalArguments();
    
    try {
        if (args.isEmpty()) {
            JoystickListDialog listDialog;
            listDialog.show();
            return exec();
        } else {
            if (args.size() > 1) {
                std::cerr << "Error: multiple device files given, only one allowed" << std::endl;
                return EXIT_FAILURE;
            }
            
            JoystickTestDialog* dialog = showDevicePropertyDialog(args.first());
            if (dialog) {
                return exec();
            } else {
                return EXIT_FAILURE;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

int main(int argc, char** argv)
{
    try {
        JoystickApp app(argc, argv);
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
