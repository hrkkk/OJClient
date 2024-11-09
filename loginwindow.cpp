#include "loginwindow.h"
#include "ui_loginwindow.h"

#include <QSettings>

LoginWindow::LoginWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginWindow)
{
    ui->setupUi(this);

    // 隐藏原始标题框
    this->setWindowFlag(Qt::FramelessWindowHint);

    ui->stackedWidget->setCurrentIndex(0);
    ui->comboBox_loginAccount->lineEdit()->setPlaceholderText("请输入账号");

    // 绑定界面按钮响应事件
    connect(ui->btn_close, &QPushButton::clicked, this, [=]() {
        this->close();
    });

    connect(ui->btn_gotoRegister, &QPushButton::clicked, this, [=]() {
        ui->stackedWidget->setCurrentIndex(1);  // 切换到注册页面
    });

    connect(ui->btn_returnLogin, &QPushButton::clicked, this, [=]() {
        ui->stackedWidget->setCurrentIndex(0);  // 返回到登录页面
    });

    connect(ui->btn_updatePassword, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(2);
    });

    connect(ui->btn_returnLogin2, &QPushButton::clicked, this, [=]() {
        ui->stackedWidget->setCurrentIndex(0);
    });

    connect(ui->btn_login, &QPushButton::clicked, this, &LoginWindow::onLogin);

    connect(ui->btn_register, &QPushButton::clicked, this, &LoginWindow::onRegister);

    connect(ui->btn_update, &QPushButton::clicked, this, [=]() {
        if(ui->lineEdit_oldPassword->text().isEmpty() || ui->lineEdit_newPassword->text().isEmpty()) {
            QMessageBox::warning(this, "Warning", "请正确输入旧密码与新密码!");
        } else {
            requestUpdatePassword();
        }
    });

    connect(ui->comboBox_loginAccount, &QComboBox::editTextChanged, this, [=]() {
        ui->lineEdit_loginPassword->clear();
        m_useRecord = false;
    });

    connect(ui->lineEdit_loginPassword, &QLineEdit::textEdited, this, [=]() {
        m_useRecord = false;
    });

    loadAccountBook();

    tcpClient = new TCPClient();

    eventLoop = new QEventLoop(this);

    connect(tcpClient, &TCPClient::s_recvData, this, [=](QByteArray msg) {
        recvMsg = msg;
        if(eventLoop->isRunning()) {
            eventLoop->quit();
        }
    });
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

///
/// \name   requestLogin
/// \brief  客户端向服务器传递账号和密码，申请登录
///
int LoginWindow::requestLogin(QString account, QString password, int& uid)
{
    // 密码加密
    std::string password_md5;
    if(m_useRecord) {
        password_md5 = m_password.toStdString();
    } else {
        password_md5 = MD5(password.toStdString()).toStr();
    }

    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "Login";
    send_json["Account"] = account.toStdString();
    send_json["Password"] = password_md5;

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    if(tcpClient->sendToServer(send_msg) == -1) {
        return -1;
    }

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容，判断是否允许登录
    if(recv_json.at("Reply") == "AllowLogin") {
        int identity = recv_json.at("Identity");
        uid = recv_json.at("UID");
        return identity;
    } else if(recv_json.at("Reply") == "RefuseLogin") {
        int reason = recv_json.at("Reason");
        return reason;
    }

    return -1;
}

///
/// \name   onLogin
/// \brief  登录按钮响应事件
///
void LoginWindow::onLogin()
{
    QString account = ui->comboBox_loginAccount->currentText();
    QString password = ui->lineEdit_loginPassword->text();

    // 密码加密
    QString password_md5;
    if(m_useRecord) {
        password_md5 = m_password;
    } else {
        password_md5 = QString::fromStdString(MD5(password.toStdString()).toStr());
    }

    int uid = 0;
    int flag = requestLogin(account, password, uid);

    if(flag == 0) {
        ui->btn_login->setText("登录中...");
        ui->btn_login->setEnabled(false);
        ui->btn_gotoRegister->setEnabled(false);
        updateAccountBook(account, password_md5);
        AdminWindow* adminWindow = new AdminWindow(tcpClient);
        adminWindow->show();
        this->close();
    } else if(flag == 1) {
        ui->btn_login->setText("登录中...");
        ui->btn_login->setEnabled(false);
        ui->btn_gotoRegister->setEnabled(false);
        updateAccountBook(account, password_md5);
        TeacherWindow* teacherWindow = new TeacherWindow(tcpClient, uid);
        teacherWindow->show();
        this->close();
    } else if(flag == 2) {
        ui->btn_login->setText("登录中...");
        ui->btn_login->setEnabled(false);
        ui->btn_gotoRegister->setEnabled(false);
        updateAccountBook(account, password_md5);
        StudentWindow* studentWindow = new StudentWindow(tcpClient, uid);
        studentWindow->show();
        this->close();
    } else {
        switch(flag) {
        case -1:
            QMessageBox::warning(this, "提示", "无法连接至服务器。");
            break;
        case -2:
            QMessageBox::warning(this, "提示", "该账号不存在，请检查输入或注册账号。");
            break;
        case -3:
            QMessageBox::warning(this, "提示", "密码错误，请检查输入。");
            break;
        default:
            break;
        }
    }
}

///
/// \name   requestRegister
/// \brief  客户端向服务器传递账号和密码，申请注册
///
int LoginWindow::requestRegister(QString account, QString password, QString date)
{
    // 密码加密
    std::string password_md5 = MD5(password.toStdString()).toStr();

    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "Register";
    send_json["Account"] = account.toStdString();
    send_json["Password"] = password_md5;
    send_json["Date"] = date.toStdString();

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    if(tcpClient->sendToServer(send_msg) == -1) {
        return -1;
    }

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容，判断是否允许登录
    if(recv_json.at("Reply") == "AllowRegister") {
        return 1;
    } else if(recv_json.at("Reply") == "RefuseRegister") {
        return -1;
    }

    return -2;
}

///
/// \name   onRegister
/// \brief  注册按钮响应事件
///
void LoginWindow::onRegister()
{
    QString account = ui->lineEdit_regAccount->text();
    QString password = ui->lineEdit_regPassword->text();
    QString date = QDate::currentDate().toString("yyyy-MM-dd");
    int flag = requestRegister(account, password, date);

    switch(flag) {
    case 1:
        QMessageBox::warning(this, "提示", "注册成功！");
        ui->lineEdit_regAccount->clear();
        ui->lineEdit_regPassword->clear();
        ui->stackedWidget->setCurrentIndex(0);
        break;
    case -1:
        QMessageBox::warning(this, "提示", "注册失败：该账号已存在！");
        break;
    default:
        break;
    }
}

void LoginWindow::requestUpdatePassword()
{
    QString account = ui->lineEdit_account->text();
    QString oldPassword = ui->lineEdit_oldPassword->text();
    QString newPassword = ui->lineEdit_newPassword->text();

    // 密码加密
    std::string oldPassword_md5 = MD5(oldPassword.toStdString()).toStr();
    std::string newPassword_md5 = MD5(newPassword.toStdString()).toStr();

    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "UpdatePassword";
    send_json["Account"] = account.toStdString();
    send_json["OldPassword"] = oldPassword_md5;
    send_json["NewPassword"] = newPassword_md5;

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    tcpClient->sendToServer(send_msg);

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容，判断是否允许登录
    if(recv_json.at("Reply") == "UpdatePasswordSuccess") {
        QMessageBox::information(this, "Information", "修改密码成功!");
        ui->lineEdit_account->clear();
        ui->lineEdit_oldPassword->clear();
        ui->lineEdit_newPassword->clear();
        emit ui->btn_returnLogin2->clicked();
    } else if(recv_json.at("Reply") == "UpdatePasswordFail") {
        if(recv_json.at("Reason") == "AccountNotExists") {
            QMessageBox::warning(this, "Warning", "要修改的账号不存在，请重新输入!");
            ui->lineEdit_account->clear();
            ui->lineEdit_oldPassword->clear();
            ui->lineEdit_newPassword->clear();
        } else if(recv_json.at("Reason") == "OldPasswordWrong") {
            QMessageBox::warning(this, "Warning", "旧密码与账号不匹配，请重新输入!");
            ui->lineEdit_oldPassword->clear();
            ui->lineEdit_newPassword->clear();
        }
    }
}

void LoginWindow::loadAccountBook()
{
    QString fileName = QCoreApplication::applicationDirPath() +  "./setting.ini";
    QSettings *settings = new QSettings(fileName, QSettings::IniFormat);

    QStringList keys = settings->allKeys();
    for(int i = 0; i < keys.count(); ++i)
    {
        QString key = keys[i];
        QString value = settings->value(key).toString();
        m_accountRecord.append(key);
        m_passwordRecord.append(value);
    }

    for(int i = 0; i < m_accountRecord.size(); i++) {
        ui->comboBox_loginAccount->addItem(m_accountRecord.at(i));
    }

    connect(ui->comboBox_loginAccount, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
        ui->checkBox->setChecked(true);
        ui->lineEdit_loginPassword->setText("12345678");
        m_useRecord = true;
        m_password = m_passwordRecord.at(index);
    });

    if(m_accountRecord.size() != 0) {
        ui->comboBox_loginAccount->setCurrentIndex(0);
    }
}

void LoginWindow::updateAccountBook(QString account, QString password)
{
    QString fileName = QCoreApplication::applicationDirPath() +  "./setting.ini";
    QSettings* settings = new QSettings(fileName, QSettings::IniFormat);
    if(ui->checkBox->isChecked()) {
        settings->setValue(account, password);
    } else {
        if(settings->contains(account)) {
            settings->remove(account);
        }
    }
}

