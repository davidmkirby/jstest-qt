// Add this to src/utils/dialog_helper.h

#ifndef JSTEST_QT_DIALOG_HELPER_H
#define JSTEST_QT_DIALOG_HELPER_H

#include <QObject>
#include <QWindow>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialog>
#include <QApplication>
#include <QScreen>
#include <QProcess>

class DialogManager : public QObject
{
    Q_OBJECT

public:
    static DialogManager& instance() {
        static DialogManager instance;
        return instance;
    }

    // Launch a dialog in a completely separate process
    static void launchExternalDialog(const QString& type, const QString& devicePath) {
        QString program = QApplication::applicationFilePath();
        QStringList arguments;
        arguments << "--external-dialog" << type << devicePath;
        
        QProcess* process = new QProcess();
        process->setProgram(program);
        process->setArguments(arguments);
        
        // Connect to finished signal to clean up
        QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                        process, &QProcess::deleteLater);
        
        process->start();
    }
    
    // Helper to create mapping dialog in a separate process
    static void showMappingDialog(const QString& devicePath) {
        launchExternalDialog("mapping", devicePath);
    }
    
    // Helper to create calibration dialog in a separate process
    static void showCalibrationDialog(const QString& devicePath) {
        launchExternalDialog("calibration", devicePath);
    }

private:
    DialogManager() {}
    ~DialogManager() {}
    
    // Don't allow copying
    DialogManager(const DialogManager&) = delete;
    DialogManager& operator=(const DialogManager&) = delete;
};

#endif // JSTEST_QT_DIALOG_HELPER_H
