#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <windows.h>
#include <TlHelp32.h>
#include "workerthread.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),m_procDlg(parent)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

// 选择目标进程
void Widget::on_pushButton_2_clicked()
{
    // 模态展示 进程对话框
    if( m_procDlg.exec() )
    {
        ui->procName->setText(  m_procDlg.getCurPName() );
    }
}

// 注入Code
void Widget::on_pushButton_3_clicked()
{

    WorkerThread *worker = new WorkerThread(m_procDlg.getCurPID());
    connect(worker,&WorkerThread::doInjectFinish,this,[=](){
        // 注入dll完毕
        worker->exit();
        delete worker;
    });
    worker->start();
}
