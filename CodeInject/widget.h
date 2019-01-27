#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "processdialog.h"
#include <QFileDialog>
#include <QThread>
#include <windows.h>

namespace Ui {
class Widget;
}

typedef struct _data{
    DWORD dwLoadLibrary;
    DWORD dwGetProcAddress;
    DWORD dwGetModuleFileName;

    char user32dll[MAX_PATH];
    char MessageBox[MAX_PATH];
    char msg[MAX_PATH];
}DATA;

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();
private:
    QString getCurPName();
    int getCurPID();

private slots:

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();


private:
    Ui::Widget *ui;
    ProcessDialog m_procDlg;
    QThread m_thread;
};

#endif // WIDGET_H
