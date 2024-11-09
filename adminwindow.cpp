#include "adminwindow.h"
#include "ui_adminwindow.h"

#include <QDate>
#include <QMessageBox>

AdminWindow::AdminWindow(TCPClient* tcpClient, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AdminWindow)
{
    ui->setupUi(this);

    // 隐藏原始标题框
    this->setWindowFlag(Qt::FramelessWindowHint);

    // 控件初始化
    QButtonGroup* btnGroup = new QButtonGroup(this);
    btnGroup->addButton(ui->toolBtn_profile, 0);
    btnGroup->addButton(ui->toolBtn_manage, 1);

    // 绑定左侧按钮与右侧页面
    connect(btnGroup, QOverload<int>::of(&QButtonGroup::idClicked), ui->stackedWidget, &QStackedWidget::setCurrentIndex);

    // 绑定最小化按钮响应事件
    connect(ui->btn_min, &QPushButton::clicked, this, [=]() {
        this->showMinimized();
    });

    // 绑定最大化按钮响应事件
    connect(ui->btn_resize, &QPushButton::clicked, this, [=]() {
        if(this->isMaximized()) {
            this->showNormal();
        } else {
            this->showMaximized();
        }
    });

    // 绑定关闭按钮响应事件
    connect(ui->btn_close, &QPushButton::clicked, this, [=]() {
        this->close();
    });

    connect(ui->tBtn_addUser, &QToolButton::clicked, this, [=]() {
        if(addUserDialog == nullptr) {
            addUserDialog = new AddUserDialog();

            connect(addUserDialog, &AddUserDialog::s_addNewUser, this, [=](QString _account, QString _password) {
                addNewUser(_account, _password);
            });
            addUserDialog->exec();
        }

        delete addUserDialog;
        addUserDialog = nullptr;
    });

    // 设置默认显示的页面
    btnGroup->button(0)->setChecked(true);
    ui->stackedWidget->setCurrentIndex(0);

    m_tcpClient = tcpClient;

    eventLoop = new QEventLoop(this);

    connect(m_tcpClient, &TCPClient::s_recvData, this, [=](QByteArray msg) {
        recvMsg = msg;
        if(eventLoop->isRunning()) {
            eventLoop->quit();
        }
    });

    updateAndShow();
}

AdminWindow::~AdminWindow()
{
    delete ui;
}

void AdminWindow::updateAndShow()
{
    RequestUserList();
    fillTable(ui->tableWidget);
}

void AdminWindow::fillTable(QTableWidget* tableWidget)
{
    // 清楚表格数据区中的所有内容，但是不清除表头
    tableWidget->clearContents();

    int N = userList.size();

    tableWidget->setRowCount(N);

    // 填入数据
    for(int i = 0; i < N; i++) {
        QStringList stringList = userList.at(i).split("|");

        tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        tableWidget->item(i, 0)->setTextAlignment(Qt::AlignCenter);

        tableWidget->setItem(i, 1, new QTableWidgetItem(stringList.at(0)));
        tableWidget->item(i, 1)->setTextAlignment(Qt::AlignCenter);

        tableWidget->setItem(i, 2, new QTableWidgetItem(stringList.at(1)));
        tableWidget->item(i, 2)->setTextAlignment(Qt::AlignCenter);

        QString str;
        if(stringList.at(2) == "1") {
            str = "教师";
        } else if(stringList.at(2) == "2") {
            str = "学生";
        }
        tableWidget->setItem(i, 3, new QTableWidgetItem(str));
        tableWidget->item(i, 3)->setTextAlignment(Qt::AlignCenter);

        QPushButton* btn_alter = new QPushButton("修改");
        tableWidget->setCellWidget(i, 4, btn_alter);

        QPushButton* btn_delete = new QPushButton("删除");
        tableWidget->setCellWidget(i, 5, btn_delete);

        // 绑定按钮响应事件，通过位置找到按钮所在行
        connect(btn_alter, &QPushButton::clicked, this, [=]() {
            QPushButton* btn = (QPushButton*)sender();
            int x = btn->frameGeometry().x();
            int y = btn->frameGeometry().y();
            QModelIndex index = tableWidget->indexAt(QPoint(x, y));
            int row = index.row();
            //int col = index.column();

            QStringList stringList = userList.at(row).split("|");
            QString account = stringList.at(1);
            QString str = "";
            int newIdentity = stringList.at(2).toInt();
            if(stringList.at(2) == "1") {
                str = "学生";
                newIdentity = 2;
            } else if(stringList.at(2) == "2") {
                str = "教师";
                newIdentity = 1;
            }
            QString msg = QString("确认修改用户<b>%1</b>身份为<b>%2</b>？").arg(account).arg(str);
            QMessageBox::StandardButton result = QMessageBox::question(this, "Verify", msg, QMessageBox::Yes | QMessageBox::No);
            if(result == QMessageBox::StandardButton::Yes) {
                alterUserIdentity(stringList.at(0).toInt(), newIdentity);
            }
        });

        // 绑定按钮响应事件，通过位置找到按钮所在行
        connect(btn_delete, &QPushButton::clicked, this, [=]() {
            QPushButton* btn = (QPushButton*)sender();
            int x = btn->frameGeometry().x();
            int y = btn->frameGeometry().y();
            QModelIndex index = tableWidget->indexAt(QPoint(x, y));
            int row = index.row();
            //int col = index.column();

            QStringList stringList = userList.at(row).split("|");
            QString account = stringList.at(1);
            QString msg = QString("确认删除用户<b>%1</b>？").arg(account);
            QMessageBox::StandardButton result = QMessageBox::question(this, "Verify", msg, QMessageBox::Yes | QMessageBox::No);
            if(result == QMessageBox::StandardButton::Yes) {
                deleteUser(stringList.at(0).toInt());
            }
        });
    }
}

void AdminWindow::RequestUserList()
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "RequestUserList";

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    userList.clear();

    // 发送数据到服务器
    m_tcpClient->sendToServer(send_msg);

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容，判断是否允许登录
    if(recv_json.at("Reply") == "UserList") {
        std::vector<std::string> lists = recv_json.at("List");

        for(size_t i = 0; i < lists.size(); i++) {
            userList.append(QString::fromStdString(lists.at(i)));
        }
    }
}

void AdminWindow::alterUserIdentity(int uid, int newIdentity)
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "AlterUserIdentity";
    send_json["UID"] = uid;
    send_json["Identity"] = newIdentity;

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    m_tcpClient->sendToServer(send_msg);

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容，判断是否允许登录
    if(recv_json.at("Reply") == "AlterUserSuccess") {
        QMessageBox::information(this, "Information", "修改用户身份成功");
        updateAndShow();
    }
}

void AdminWindow::deleteUser(int uid)
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "DeleteUser";
    send_json["UID"] = uid;

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    m_tcpClient->sendToServer(send_msg);

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容，判断是否允许登录
    if(recv_json.at("Reply") == "DeleteUserSuccess") {
        QMessageBox::information(this, "Information", "删除成功");
        updateAndShow();
    }
}

void AdminWindow::addNewUser(QString account, QString password)
{
    QString date = QDate::currentDate().toString("yyyy-MM-dd");

    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "Register";
    send_json["Account"] = account.toStdString();
    send_json["Password"] = password.toStdString();
    send_json["Date"] = date.toStdString();

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    m_tcpClient->sendToServer(send_msg);

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容，判断是否允许登录
    if(recv_json.at("Reply") == "AllowRegister") {
        QMessageBox::warning(this, "提示", "添加新用户成功！");
        updateAndShow();
    } else if(recv_json.at("Reply") == "RefuseRegister") {
        QMessageBox::warning(this, "提示", "添加用户失败：该账号已存在！");
    }
}
