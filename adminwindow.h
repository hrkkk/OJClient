#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QDialog>
#include <QButtonGroup>
#include <QAction>
#include <QHBoxLayout>
#include <QEventLoop>
#include <QTableWidget>

#include "tcpclient.h"
#include "adduserdialog.h"
#include "json.hpp"
using json = nlohmann::json;

namespace Ui {
class AdminWindow;
}

class AdminWindow : public QDialog
{
    Q_OBJECT

public:
    explicit AdminWindow(TCPClient* tcpClient, QWidget *parent = nullptr);
    ~AdminWindow();

private:
    void fillTable(QTableWidget* tableWidget);
    void RequestUserList();
    void alterUserIdentity(int uid, int newIdentity);
    void deleteUser(int uid);
    void addNewUser(QString account, QString password);
    void updateAndShow();

private:
    Ui::AdminWindow *ui;
    QEventLoop* eventLoop;
    QByteArray recvMsg;
    TCPClient* m_tcpClient;
    AddUserDialog* addUserDialog = nullptr;

    QList<QString> userList;
};

#endif // ADMINWINDOW_H
