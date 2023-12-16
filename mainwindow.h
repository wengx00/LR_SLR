#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "grammer.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_toParseGrammer_clicked();

    void on_chooseFile_clicked();

    void on_saveFile_clicked();

    void on_toParseStatement_clicked();

private:
    Ui::MainWindow *ui;
    void renderBasicInfo();
    void renderDfaTable();
    void renderSlrTable();
    Grammer* currentGrammer;
};
#endif // MAINWINDOW_H
