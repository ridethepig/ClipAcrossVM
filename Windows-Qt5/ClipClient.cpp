#include "ClipClient.h"
#include <QFuture>
#include <QtConcurrent/qtconcurrentrun.h>
#pragma warning(disable:26812)

ClipClient::ClipClient(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    this->setWindowFlags(Qt::WindowMinimizeButtonHint);
    
    connect(clipboard, SIGNAL(changed(QClipboard::Mode)), this, SLOT(clipboardChanged(QClipboard::Mode)));       
    
    settings->beginGroup("conn_data");
    ui.lineEdit->setText(settings->value("ip").toString());
    ui.spinBox->setValue(settings->value("port").toInt());
    settings->endGroup();

    sysTray_menu = new QMenu(this);
    sysTray_actionState = new QAction(sysTray_menu);
    sysTray_actionExit = new QAction(sysTray_menu);
    sysTray_actionState->setText("Enable");
    sysTray_actionExit->setText("Exit");   
    sysTray_menu->addAction(sysTray_actionState);
    sysTray_menu->addAction(sysTray_actionExit);
    connect(sysTray_actionState, &QAction::triggered, this, &ClipClient::changeState);
    connect(sysTray_actionExit, &QAction::triggered, this, &ClipClient::close);


    sysTray->setIcon(QIcon(":/ClipClient/clipper.svg"));
    sysTray->setToolTip("ClipClient");
    sysTray->setContextMenu(sysTray_menu);
    sysTray->show();
    connect(sysTray, &QSystemTrayIcon::activated, this, &ClipClient::systemTrayIconClicked);

    connect(this, &ClipClient::socket_complete_signal, this, &ClipClient::socketCompleted);
}

void ClipClient::clipboardChanged(QClipboard::Mode mode) {
    if (!on_listen) return;
    QString str = clipboard->text();
    if (str == last_clip) return;
    QString ip = ui.lineEdit->text(); // I just don't want to waste my time validating it
    int port = ui.spinBox->value();
    QtConcurrent::run(this, &ClipClient::sendData, ip, port, str);
    last_clip = str;
}

void ClipClient::pushButtonClicked() {
    changeState();
}

void ClipClient::closeEvent(QCloseEvent* e) {    
    beforeCloseProgram();
    e->accept();
}

void ClipClient::saveData()
{
    settings->beginGroup("conn_data");
    settings->setValue("ip", ui.lineEdit->text());
    settings->setValue("port", ui.spinBox->value());
    settings->endGroup();
}

void ClipClient::beforeCloseProgram()
{
    saveData();    
}

void ClipClient::changeState()
{
    if (on_listen) {
        ui.pushButton->setText("Enable");
        sysTray_actionState->setText("Enable");
        on_listen = false;
    }
    else {
        ui.pushButton->setText("Disable");
        sysTray_actionState->setText("Disable");
        on_listen = true;
    }
    ui.spinBox->setDisabled(on_listen);
    ui.lineEdit->setDisabled(on_listen);
    
}

void ClipClient::sendData(QString ip, int port, QString data)
{    
    QTcpSocket* tcpSocket = new QTcpSocket();
    if (tcpSocket != nullptr) {
        tcpSocket->connectToHost(ip, port);
        if (tcpSocket->waitForConnected(3000)) {
            tcpSocket->write(data.toStdString().c_str());
            tcpSocket->flush();
            tcpSocket->disconnectFromHost();
            emit socket_complete_signal(0);
        }
        else {
            tcpSocket->disconnectFromHost();
            emit socket_complete_signal(1);
        }
    }
    else {
        emit socket_complete_signal(2);
    }
}

void ClipClient::systemTrayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
    case QSystemTrayIcon::Unknown:
        break;
    case QSystemTrayIcon::Context:
        break;
    case QSystemTrayIcon::DoubleClick:
        break;
    case QSystemTrayIcon::Trigger:
        showNormal();
        activateWindow();
        break;
    case QSystemTrayIcon::MiddleClick:
        break;
    default:
        break;
    }
}

void ClipClient::socketCompleted(int resp)
{
    switch (resp)
    {
    case 0: 
    {
        QDateTime* datetime = new QDateTime(QDateTime::currentDateTime());
        ui.plainTextEdit->appendPlainText(datetime->toString("MM-dd hh:mm:ss") + tr("OK.\n"));
    }
    break;
    case 1:
    {
        QDateTime* datetime = new QDateTime(QDateTime::currentDateTime());
        ui.plainTextEdit->appendPlainText(
            datetime->toString("MM-dd hh:mm:ss") + tr("Connection Timeout.\n"));
    }
    break;
    case 2:
    {
        ui.plainTextEdit->appendPlainText(tr("Socket not Initialized. IGNORE this copy\n"));
    }
    break;
    default:
        break;
    }
}

ClipClient::~ClipClient()
{
    delete sysTray;
    delete sysTray_menu;
    delete sysTray_actionState;
    delete sysTray_actionExit;
}

void ClipClient::changeEvent(QEvent* e) {
    if (e->type() != QEvent::WindowStateChange) return;
    if (this->windowState() == Qt::WindowMinimized) {
        this->hide();
    }
}