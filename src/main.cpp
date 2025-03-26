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
#include <QProcess>

#include "joystick.h"
#include "dialogs/joystick_test_dialog.h"
#include "dialogs/joystick_list_dialog.h"
#include "dialogs/joystick_map_dialog.h"
#include "dialogs/joystick_calibration_dialog.h"
#include "utils/dialog_helper.h"

// Static member initialization
JoystickApp* JoystickApp::m_instance = nullptr;

JoystickGui::JoystickGui(std::unique_ptr<Joystick> joystick, bool simple_ui, QWidget* parent) :
    QObject(nullptr),
    m_joystick(std::move(joystick)),
    m_test_dialog()
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
    // Instead of creating a dialog directly, launch it in a separate process
    DialogManager::showCalibrationDialog(QString::fromStdString(m_joystick->getFilename()));
}

void
JoystickGui::showMappingDialog()
{
    // Instead of creating a dialog directly, launch it in a separate process
    DialogManager::showMappingDialog(QString::fromStdString(m_joystick->getFilename()));
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
    
    // Set environment variables for Wayland
    // Only set the platform if not already specified
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "wayland");
    }
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
            std::shared_ptr<JoystickGui> gui = std::make_shared<JoystickGui>(std::move(joystick), m_simple_ui, nullptr); // Note: parent set to nullptr

            JoystickTestDialog* dialog = gui->getTestDialog();
            dialog->setAttribute(Qt::WA_DeleteOnClose, false); // Prevent auto-delete
            dialog->setWindowModality(Qt::NonModal); // Make it a non-modal window
            dialog->show(); // Show it immediately

            connect(dialog, &QDialog::finished, this, [this, filename]() {
                m_joystick_guis.remove(filename);
            });

            m_joystick_guis[filename] = gui;
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
    
    QCommandLineOption waylandOption("wayland", "Force Wayland platform plugin");
    parser.addOption(waylandOption);
    
    QCommandLineOption externalDialogOption("external-dialog", "Launch as an external dialog");
    parser.addOption(externalDialogOption);
    
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
    
    if (parser.isSet(waylandOption)) {
        qputenv("QT_QPA_PLATFORM", "wayland");
    }
    
    // Handle external dialog requests
    if (parser.isSet("external-dialog")) {
        QStringList args = parser.positionalArguments();
        if (args.size() >= 2) {
            QString dialogType = args[0];
            QString devicePath = args[1];
            
            try {
                std::unique_ptr<Joystick> joystick(new Joystick(devicePath.toStdString()));
                
                if (dialogType == "mapping") {
                    JoystickMapDialog dialog(*joystick);
                    return dialog.exec();
                } 
                else if (dialogType == "calibration") {
                    JoystickCalibrationDialog dialog(*joystick);
                    return dialog.exec();
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }
        return EXIT_FAILURE;
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
        // Set environment variables before QApplication is constructed
        if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
            qputenv("QT_QPA_PLATFORM", "wayland");
        }
        
        JoystickApp app(argc, argv);
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
