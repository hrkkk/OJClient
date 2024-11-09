#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>
#include <QMessageBox>
#include <QEventLoop>

#include "adminwindow.h"
#include "teacherwindow.h"
#include "studentwindow.h"
#include "tcpclient.h"
#include "md5.h"

#include <QDate>

#include "json.hpp"
using json = nlohmann::json;

namespace Ui {
class LoginWindow;
}

class LoginWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

private:
    int requestLogin(QString account, QString password, int& uid);
    int requestRegister(QString account, QString password, QString date);
    void requestUpdatePassword();
    void loadAccountBook();
    void updateAccountBook(QString account, QString password);

private slots:
    void onLogin();
    void onRegister();

private:
    Ui::LoginWindow *ui;
    TCPClient* tcpClient;
    QEventLoop* eventLoop;
    QByteArray recvMsg;

    QList<QString> m_accountRecord;
    QList<QString> m_passwordRecord;
    bool m_useRecord;
    QString m_password;
};

#endif // LOGINWINDOW_H
