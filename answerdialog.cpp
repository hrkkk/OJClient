#include "answerdialog.h"
#include "ui_answerdialog.h"

AnswerDialog::AnswerDialog(TCPClient* tcpClient, int uid, int pid, int num, int category, int score, QString content, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AnswerDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("答题界面");
    this->setWindowFlags(Qt::WindowCloseButtonHint);

    ui->stackedWidget->setCurrentIndex(category);
    if(category == CATEGORY::PROGRAM) {
        ui->btn_showAnswer->setEnabled(false);
    }

    connect(ui->btn_userFolder, &QToolButton::clicked, this, [=]() {
        QDesktopServices::openUrl(QUrl(QCoreApplication::applicationDirPath() +  "./UserFolder/", QUrl::TolerantMode));
    });

    if(category == CATEGORY::CHOICE) {
        json content_json = json::parse(content.toStdString());

        QString describe = QString::fromStdString(content_json["Describe"]);
        QStringList options = QString::fromStdString(content_json["Option"]).split("|");
        QString optionA = options.at(0);
        QString optionB = options.at(1);
        QString optionC = options.at(2);
        QString optionD = options.at(3);
        QString answer = QString::fromStdString(content_json["Answer"]);
        QString analysis = QString::fromStdString(content_json["Analysis"]);

        ui->label_choice_num->setText(QString::number(num + 1) + "、");
        ui->label_choice_describe->setText(describe);
        ui->label_optionA->setText(optionA);
        ui->label_optionB->setText(optionB);
        ui->label_optionC->setText(optionC);
        ui->label_optionD->setText(optionD);
        ui->label_choice_answer->setText(answer);
        ui->label_choice_analysis->setText(analysis);

        ui->label_choice_answer->hide();
        ui->label_choice_analysis->hide();
    } else if(category == CATEGORY::COMPLETION) {
        json content_json = json::parse(content.toStdString());

        QString describe = QString::fromStdString(content_json["Describe"]);
        QStringList blankAnswer = QString::fromStdString(content_json["Answer"]).split("|");
        int blankNum = blankAnswer.size();

        for(int i = blankNum + 1; i <= 5; i++) {
            QWidget::findChild<QLineEdit*>("lineEdit_blank" + QString::number(i))->hide();
        }

        QString answer = QString::fromStdString(content_json["Answer"]).replace("|", "\n");
        QString analysis = QString::fromStdString(content_json["Analysis"]);

        ui->label_completion_num->setText(QString::number(num + 1) + "、");
        ui->label_completion_describe->setText(describe);

        ui->label_completion_answer->setText(answer);
        ui->label_completion_analysis->setText(analysis);

        ui->label_completion_answer->hide();
        ui->label_completion_analysis->hide();
    } else if(category == CATEGORY::PROGRAM) {
        json content_json = json::parse(content.toStdString());

        QString describe = QString::fromStdString(content_json["Describe"]);
        QString templateFile = QString::fromStdString(content_json["Template"]);

        ui->label_program_num->setText(QString::number(num + 1) + "、");
        ui->label_program_describe->setText(describe);

        QFile file(QCoreApplication::applicationDirPath() +  "./UserFolder/test.c");
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        file.write(templateFile.toUtf8());
        file.close();
    }

    connect(ui->btn_showAnswer, &QPushButton::clicked, this, [=]() {
        if(ui->stackedWidget->currentIndex() == 0) {
            ui->label_choice_answer->show();
            ui->label_choice_analysis->show();
        } else if(ui->stackedWidget->currentIndex() == 1) {
            ui->label_completion_answer->show();
            ui->label_completion_analysis->show();
        }});

    connect(ui->btn_submit, &QPushButton::clicked, this, [=]() {
        if(ui->stackedWidget->currentIndex() == 0) {
            if(ui->radioBtn_A->isChecked()) {
                ui->label_choice_response->setText("A");
            } else if(ui->radioBtn_B->isChecked()) {
                ui->label_choice_response->setText("B");
            } else if(ui->radioBtn_C->isChecked()) {
                ui->label_choice_response->setText("C");
            } else if(ui->radioBtn_D->isChecked()) {
                ui->label_choice_response->setText("D");
            }

            if(ui->label_choice_response->text() == ui->label_choice_answer->text()) {
                ui->label_choice_response->setStyleSheet("color:green");
            } else {
                ui->label_choice_response->setStyleSheet("color:red");
            }

            emit ui->btn_showAnswer->clicked();
            submitResponse(CATEGORY::CHOICE);
        } else if(ui->stackedWidget->currentIndex() == 1) {
            emit ui->btn_showAnswer->clicked();
            submitResponse(CATEGORY::COMPLETION);
        } else if(ui->stackedWidget->currentIndex() == 2) {
            ui->label_program_compileResult->setText("程序编译中");
            ui->label_program_compileOut->clear();
            ui->label_program_testResult->clear();
            ui->label_program_detail->clear();
            submitProgram();
        }
    });

    m_tcpClient = tcpClient;
    m_uid = uid;
    m_pid = pid;
    m_category = category;
    m_score = score;

    eventLoop = new QEventLoop(this);

    connect(m_tcpClient, &TCPClient::s_recvData, this, [=](QByteArray msg) {
        recvMsg = msg;
        if(eventLoop->isRunning()) {
            eventLoop->quit();
        }
    });
}

AnswerDialog::~AnswerDialog()
{
    delete ui;
}

void AnswerDialog::submitResponse(int category)
{
    QString response;
    int point = 0;

    if(category == CATEGORY::CHOICE) {
        // 提取回答内容
        response = ui->label_choice_response->text();
        // 计算得分
        if(ui->label_choice_response->text() == ui->label_choice_answer->text()) {
            point = m_score;
        } else {
            point = 0;
        }
    } else if(category == CATEGORY::COMPLETION) {
        // 提取回答内容
        response = ui->lineEdit_blank1->text();
        if(!ui->lineEdit_blank2->isHidden()) {
            response += "|" + ui->lineEdit_blank2->text();
        }
        if(!ui->lineEdit_blank3->isHidden()) {
            response += "|" + ui->lineEdit_blank3->text();
        }
        if(!ui->lineEdit_blank4->isHidden()) {
            response += "|" + ui->lineEdit_blank4->text();
        }
        if(!ui->lineEdit_blank5->isHidden()) {
            response += "|" + ui->lineEdit_blank5->text();
        }
        // 计算得分
        QString res = QString(response).replace("|", "\n");
        if(res == ui->label_completion_answer->text()) {
            point = m_score;
        } else {
            point = 0;
        }
    }

    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "SubmitResponse";
    send_json["Category"] = category;
    send_json["UID"] = m_uid;
    send_json["PID"] = m_pid;
    send_json["Point"] = point;
    send_json["Content"] = response.toStdString();

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    m_tcpClient->sendToServer(send_msg);

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容
    if(recv_json.at("Reply") == "SubmitSuccess") {
        QMessageBox::information(this, "information", "题目提交成功");
        emit sig_submitted(CATEGORY(category));
    }
}

void AnswerDialog::submitProgram()
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "SubmitResponse";
    send_json["Category"] = CATEGORY::PROGRAM;
    send_json["UID"] = m_uid;
    send_json["PID"] = m_pid;
    send_json["Score"] = m_score;

    QFile file(QCoreApplication::applicationDirPath() +  "./UserFolder/test.c");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString code_content = file.readAll();
    file.close();

    send_json["Content"] = code_content.toStdString();

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    m_tcpClient->sendToServer(send_msg);

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容
    if(recv_json.at("Reply") == "ProgramVerify") {
        QString compileOut = QString::fromStdString(recv_json.at("CompileResult"));
        QString compileFlag = QString::fromStdString(recv_json.at("Compile"));
        QString testFlag = QString::fromStdString(recv_json.at("Test"));
        QString runStatus = QString::fromStdString(recv_json.at("RunStatus"));
        int passCaseNum = recv_json.at("PassCaseNums");
        int failCaseNum = recv_json.at("FailCaseNums");

        compileOut.replace("error", "<font color = red>error</font>");
        compileOut.replace("warning", "<font color = #FF52EF>warning</font>");
        compileOut.replace('\n', "<br>");   // 文本中含有'\n'换行符时将以纯文本的格式解析，而不是HTML格式

        ui->label_program_compileOut->setText(compileOut);
        if(compileFlag == "Pass") {
            ui->label_program_compileResult->setText("<font color = green>编译成功！</font>");

            if(testFlag == "Pass") {
                ui->label_program_testResult->setText("<font color = green>运行成功！</font>");
                ui->label_program_detail->setText(QString("通过测试案例数：%1 / 未通过测试案例数：%2").arg(passCaseNum).arg(failCaseNum));
            } else if(testFlag == "Fail") {
                ui->label_program_testResult->setText("<font color = red>运行失败！</font>");
                if(runStatus == "OVER_MEM") {
                    ui->label_program_detail->setText("程序运行超内存！");
                } else if(runStatus == "OVER_TIME") {
                    ui->label_program_detail->setText("程序运行超时！");
                } else {
                    ui->label_program_detail->setText(QString("通过测试案例数：%1 / 未通过测试案例数：%2").arg(passCaseNum).arg(failCaseNum));
                }
            }
        } else if(compileFlag == "Fail") {
            ui->label_program_compileResult->setText("<font color = red>编译失败！</font>");
            ui->label_program_testResult->clear();
            ui->label_program_detail->clear();
        }
        emit sig_submitted(CATEGORY::PROGRAM);
    }
}
