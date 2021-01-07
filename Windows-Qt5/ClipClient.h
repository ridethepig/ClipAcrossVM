#pragma once

#include <QtWidgets/QMainWindow>
#include <QClipboard>
#include <QApplication>
#include <QEvent>
#include <QDebug>
#include <QAction>
#include <QMenu>
#include <QString>
#include <QSettings>
#include <QTcpSocket>
#include <QDateTime>
#include <QIcon>
#include <QSystemTrayIcon>
#include "ui_ClipClient.h"

class ClipClient : public QMainWindow
{
    Q_OBJECT

public:
    ClipClient(QWidget *parent = Q_NULLPTR);
    ~ClipClient();

    QClipboard* clipboard = QApplication::clipboard();    
    
    QSettings* settings = new QSettings("ClipClient.ini", QSettings::IniFormat);
    
    QSystemTrayIcon* sysTray = new QSystemTrayIcon(this);
    QMenu* sysTray_menu = nullptr;
    QAction* sysTray_actionState = nullptr;
    QAction* sysTray_actionExit = nullptr;

    bool on_listen = false;
    QString last_clip = "";

signals:
    void socket_complete_signal(int resp);

private:
    Ui::ClipClientClass ui;    
    void changeEvent(QEvent* e);
    void closeEvent(QCloseEvent* e);
    void saveData();
    void beforeCloseProgram();
    void changeState();
    void sendData(const QString ip, int port, const QString data);

private slots:
    void clipboardChanged(QClipboard::Mode);
    void pushButtonClicked();
    void systemTrayIconClicked(QSystemTrayIcon::ActivationReason reason);
    void socketCompleted(int resp);
};
