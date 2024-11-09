#include "alterdialog.h"
#include "ui_alterdialog.h"

#include <QFormLayout>

AlterDialog::AlterDialog(TCPClient* tcpClient, int pid, int category, QString content, int score, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AlterDialog)
{
    ui->setupUi(this);

    m_pid = pid;
    m_category = category;
    m_tcpClient = tcpClient;

    ui->stackedWidget->setCurrentIndex(category);

    showProblem(category, content, score);
    setContentEditable(category, false);

    connect(ui->btn_update, &QPushButton::clicked, this, [=]() {
        if(ui->btn_update->text() == tr("修改")) {
            setContentEditable(category, true);
            ui->btn_update->setText("完成修改");
            ui->btn_delete->setEnabled(false);
        } else if(ui->btn_update->text() == tr("完成修改")) {
            QMessageBox::StandardButton result = QMessageBox::question(this, "Verify", "确认修改此问题？", QMessageBox::Yes | QMessageBox::No);
            if(result == QMessageBox::StandardButton::Yes) {
                updateProblem();
            } else {
                showProblem(category, content, score);
                setContentEditable(category, false);
            }
            ui->btn_update->setText("修改");
            ui->btn_delete->setEnabled(true);
        }
    });

    connect(ui->btn_delete, &QPushButton::clicked, this, [=]() {
        QMessageBox::StandardButton result = QMessageBox::question(this, "Verify", "确认删除此问题？", QMessageBox::Yes | QMessageBox::No);
        if(result == QMessageBox::StandardButton::Yes) {
            deleteProblem();
        }
    });

    connect(ui->btn_userFolder, &QToolButton::clicked, this, [=]() {
        QDesktopServices::openUrl(QUrl(QCoreApplication::applicationDirPath() +  "./UserFolder/", QUrl::TolerantMode));
    });

    connect(ui->tBtn_addCase, &QToolButton::clicked, this, [=]() {
        addCaseWidget("", "");
    });

    eventLoop = new QEventLoop(this);

    connect(m_tcpClient, &TCPClient::s_recvData, this, [=](QByteArray msg) {
        recvMsg = msg;
        if(eventLoop->isRunning()) {
            eventLoop->quit();
        }
    });
}

AlterDialog::~AlterDialog()
{
    delete ui;
}

void AlterDialog::setContentEditable(int index, bool flag)
{
    if(index == 0) {
        ui->textEdit_choice_question->setReadOnly(!flag);
        ui->textEdit_choice_optionA->setReadOnly(!flag);
        ui->textEdit_choice_optionB->setReadOnly(!flag);
        ui->textEdit_choice_optionC->setReadOnly(!flag);
        ui->textEdit_choice_optionD->setReadOnly(!flag);
        ui->textEdit_choice_analysis->setReadOnly(!flag);
        ui->spinBox_choice_score->setReadOnly(!flag);
        ui->combo_choice->setEnabled(flag);
    } else if(index == 1) {
        ui->textEdit_completion_question->setReadOnly(!flag);
        ui->textEdit_completion_analysis->setReadOnly(!flag);
        ui->lineEdit_blank1->setReadOnly(!flag);
        ui->lineEdit_blank2->setReadOnly(!flag);
        ui->lineEdit_blank3->setReadOnly(!flag);
        ui->lineEdit_blank4->setReadOnly(!flag);
        ui->lineEdit_blank5->setReadOnly(!flag);
        ui->spinBox_completion_score->setReadOnly(!flag);
    } else if(index == 2) {
        ui->textEdit_program_describe->setReadOnly(!flag);
        ui->tBtn_addCase->setEnabled(flag);

        for(int i = 0; i < ui->vLayout_caseContainer->count(); i++) {
            if(ui->vLayout_caseContainer->itemAt(i)->widget()->objectName() == "VISIBLE") {
                QTabWidget* tabWidget = qobject_cast<QTabWidget*>(ui->vLayout_caseContainer->itemAt(i)->widget());
                QLineEdit* input = tabWidget->findChild<QLineEdit*>("input");
                QLineEdit* output = tabWidget->findChild<QLineEdit*>("output");

                tabWidget->setTabsClosable(flag);
                input->setReadOnly(!flag);
                output->setReadOnly(!flag);
            }
        }

        ui->spinBox_program_score->setReadOnly(!flag);
    }
}

void AlterDialog::showProblem(int category, QString content, int score)
{
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

        ui->textEdit_choice_question->setText(describe);
        ui->textEdit_choice_optionA->setText(optionA);
        ui->textEdit_choice_optionB->setText(optionB);
        ui->textEdit_choice_optionC->setText(optionC);
        ui->textEdit_choice_optionD->setText(optionD);
        QStringList optionList = {"A", "B", "C", "D"};
        for(int i = 0; i < 4; i++) {
            if(answer == optionList.at(i)) {
                ui->combo_choice->setCurrentIndex(i);
                break;
            }
        }
        ui->textEdit_choice_analysis->setText(analysis);
        ui->spinBox_choice_score->setValue(score);
    } else if(category == CATEGORY::COMPLETION) {
        json content_json = json::parse(content.toStdString());

        QString describe = QString::fromStdString(content_json["Describe"]);
        QString analysis = QString::fromStdString(content_json["Analysis"]);
        QStringList blankAnswer = QString::fromStdString(content_json["Answer"]).split("|");
        int blankNum = blankAnswer.size();

        for(int i = 0; i < blankNum; i++) {
            QWidget::findChild<QLineEdit*>("lineEdit_blank" + QString::number(i + 1))->setText(blankAnswer.at(i));
        }

        ui->textEdit_completion_question->setText(describe);
        ui->textEdit_completion_analysis->setText(analysis);
        ui->spinBox_completion_score->setValue(score);
    } else if(category == CATEGORY::PROGRAM) {
        json content_json = json::parse(content.toStdString());

        QString describe = QString::fromStdString(content_json["Describe"]);
        QString templateFile = QString::fromStdString(content_json["Template"]);
        std::vector<std::string> caseList = content_json["CaseList"];

        // 首先删除布局中的所有控件
        while(ui->vLayout_caseContainer->count()){
            QWidget *pWidget = ui->vLayout_caseContainer->itemAt(0)->widget();//循环每次取第一个控件，依次删除下去
            pWidget->setParent(NULL);
            ui->vLayout_caseContainer->removeWidget(pWidget);
            delete pWidget;
        }

        // 随后向布局中添加控件
        for(size_t i = 0; i < caseList.size(); i++) {
            QString input = QString::fromStdString(caseList.at(i)).split("|").at(0);
            QString output = QString::fromStdString(caseList.at(i)).split("|").at(1);
            addCaseWidget(input, output);
        }

        ui->textEdit_program_describe->setText(describe);
        ui->spinBox_program_score->setValue(score);

        QFile file(QCoreApplication::applicationDirPath() +  "./UserFolder/template.c");
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        file.write(templateFile.toUtf8());
        file.close();
    }
}

void AlterDialog::deleteProblem()
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "DeleteProblem";
    send_json["PID"] = m_pid;

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    m_tcpClient->sendToServer(send_msg);

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容
    if(recv_json.at("Reply") == "DeleteProblemSuccess") {
        QMessageBox::information(this, "information", "题目删除成功");
        emit sig_altered(CATEGORY(m_category));
    }
}

void AlterDialog::updateProblem()
{
    if(m_category == CATEGORY::CHOICE) {   // 选择题
        QString describe = ui->textEdit_choice_question->toPlainText();
        QString optionA = ui->textEdit_choice_optionA->toPlainText();
        QString optionB = ui->textEdit_choice_optionB->toPlainText();
        QString optionC = ui->textEdit_choice_optionC->toPlainText();
        QString optionD = ui->textEdit_choice_optionD->toPlainText();
        QString answer = ui->combo_choice->currentText();
        QString analysis = ui->textEdit_choice_analysis->toPlainText();
        QString score = ui->spinBox_choice_score->text();

        // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
        json send_json;
        send_json["Event"] = "UpdateProblem";
        send_json["PID"] = m_pid;
        json content_json;
        content_json["Describe"] = describe.toStdString();
        content_json["Option"] = QString(optionA + "|" + optionB + "|" + optionC + "|" + optionD).toStdString();
        content_json["Answer"] = answer.toStdString();
        content_json["Analysis"] = analysis.toStdString();
        send_json["Content"] = content_json;
        send_json["Score"] = score.toInt();

        QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

        // 发送数据到服务器
        m_tcpClient->sendToServer(send_msg);

        // 事件循环，阻塞当前函数，等待服务器响应
        eventLoop->exec();

        // 将服务器返回的二进制字节流解析为JSON格式
        std::string recv_msg = QString(recvMsg).toStdString();

        json recv_json = json::parse(recv_msg.c_str());

        // 解析JSON内容
        if(recv_json.at("Reply") == "UpdateProblemSuccess") {
            QMessageBox::information(this, "information", "题目修改成功");
            emit sig_altered(CATEGORY(m_category));
        }
    } else if(m_category == CATEGORY::COMPLETION) {  // 填空题
        QString describe = ui->textEdit_completion_question->toPlainText();
        QString answer = ui->lineEdit_blank1->text();
        if(!ui->lineEdit_blank2->text().isEmpty()) {
            answer += "|" + ui->lineEdit_blank2->text();
        }
        if(!ui->lineEdit_blank3->text().isEmpty()) {
            answer += "|" + ui->lineEdit_blank3->text();
        }
        if(!ui->lineEdit_blank4->text().isEmpty()) {
            answer += "|" + ui->lineEdit_blank4->text();
        }
        if(!ui->lineEdit_blank5->text().isEmpty()) {
            answer += "|" + ui->lineEdit_blank5->text();
        }
        QString analysis = ui->textEdit_completion_analysis->toPlainText();
        QString score = ui->spinBox_completion_score->text();

        // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
        json send_json;
        send_json["Event"] = "UpdateProblem";
        send_json["PID"] = m_pid;
        send_json["Category"] = m_category;
        json content_json;
        content_json["Describe"] = describe.toStdString();
        content_json["Answer"] = answer.toStdString();
        content_json["Analysis"] = analysis.toStdString();
        QString content_string = QString::fromStdString(content_json.dump());
        content_string.replace("'", "''");
        content_json = json::parse(content_string.toStdString());
        send_json["Content"] = content_json;
        send_json["Score"] = score.toInt();

        QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

        // 发送数据到服务器
        m_tcpClient->sendToServer(send_msg);

        // 事件循环，阻塞当前函数，等待服务器响应
        eventLoop->exec();

        // 将服务器返回的二进制字节流解析为JSON格式
        std::string recv_msg = QString(recvMsg).toStdString();

        json recv_json = json::parse(recv_msg.c_str());

        // 解析JSON内容
        if(recv_json.at("Reply") == "UpdateProblemSuccess") {
            QMessageBox::information(this, "information", "题目修改成功");
            emit sig_altered(CATEGORY(m_category));
        }
    } else if(m_category == CATEGORY::PROGRAM) {  // 编程题
        QString describe = ui->textEdit_program_describe->toPlainText();
        QString score = ui->spinBox_program_score->text();
        std::vector<std::string> caseList;

        for(int i = 0; i < ui->vLayout_caseContainer->count(); i++) {
            if(ui->vLayout_caseContainer->itemAt(i)->widget()->isVisible()) {
                QTabWidget* groupBox = qobject_cast<QTabWidget*>(ui->vLayout_caseContainer->itemAt(i)->widget());
                QString input = groupBox->findChild<QLineEdit*>("input")->text();
                QString output = groupBox->findChild<QLineEdit*>("output")->text();
                if(!input.isEmpty() && !output.isEmpty()) {
                    QString case_i = input + "|" + output;
                    caseList.push_back(case_i.toStdString());
                }
            }
        }

        QFile file(QCoreApplication::applicationDirPath() +  "./UserFolder/template.c");
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QString content = file.readAll();
        file.close();

        // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
        json send_json;
        send_json["Event"] = "UpdateProblem";
        send_json["PID"] = m_pid;
        send_json["Category"] = m_category;
        send_json["Score"] = score.toInt();
        json content_json;
        content_json["Describe"] = describe.toStdString();
        content_json["Template"] = content.toStdString();
        json caseList_json(caseList);
        content_json["CaseList"] = caseList_json;
        send_json["Content"] = content_json;

        QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

        // 发送数据到服务器
        m_tcpClient->sendToServer(send_msg);

        // 事件循环，阻塞当前函数，等待服务器响应
        eventLoop->exec();

        // 将服务器返回的二进制字节流解析为JSON格式
        std::string recv_msg = QString(recvMsg).toStdString();

        json recv_json = json::parse(recv_msg.c_str());

        // 解析JSON内容
        if(recv_json.at("Reply") == "UpdateProblemSuccess") {
            QMessageBox::information(this, "information", "题目修改成功");
            emit sig_altered(CATEGORY(m_category));
        }
    }
}

void AlterDialog::addCaseWidget(QString input, QString output)
{
    int widgetNum = ui->vLayout_caseContainer->count();
    int visibleNum = 0;
    for(int i = 0; i < widgetNum; i++) {
        if(ui->vLayout_caseContainer->itemAt(i)->widget()->objectName() == "VISIBLE") {
            visibleNum++;
        }
    }
    QTabWidget* tabWidget = new QTabWidget();
    QWidget* tab = new QWidget();
    tabWidget->addTab(tab, QString("案例%1").arg(visibleNum + 1));
    QFormLayout* formLayout = new QFormLayout(tab);
    QLineEdit* lineEdit_input = new QLineEdit();
    QLineEdit* lineEdit_output = new QLineEdit();
    lineEdit_input->setObjectName("input");
    lineEdit_output->setObjectName("output");
    formLayout->addRow("输入:", lineEdit_input);
    formLayout->addRow("输出:", lineEdit_output);
    ui->vLayout_caseContainer->insertWidget(widgetNum, tabWidget);
    lineEdit_input->setText(input);
    lineEdit_output->setText(output);
    ui->vLayout_caseContainer->itemAt(widgetNum)->widget()->setObjectName("VISIBLE");

    connect(tabWidget, &QTabWidget::tabCloseRequested, this, [=]() {
        ui->vLayout_caseContainer->itemAt(widgetNum)->widget()->hide();
        ui->vLayout_caseContainer->itemAt(widgetNum)->widget()->setObjectName("HIDDEN");

        for(int i = 0, j = 0; i < ui->vLayout_caseContainer->count(); i++) {
            if(!ui->vLayout_caseContainer->itemAt(i)->widget()->isHidden()) {
                j++;
                QTabWidget* tabWidget = static_cast<QTabWidget*>(ui->vLayout_caseContainer->itemAt(i)->widget());
                tabWidget->setTabText(0, QString("案例%1").arg(j));
            }
        }
    });
}
