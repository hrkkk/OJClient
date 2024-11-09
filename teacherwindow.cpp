#include "teacherwindow.h"
#include "ui_teacherwindow.h"
#include <QFormLayout>

TeacherWindow::TeacherWindow(TCPClient* tcpClient, int _uid, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TeacherWindow)
{
    // 界面初始化
    ui->setupUi(this);

    // 隐藏原始标题框
    this->setWindowFlag(Qt::FramelessWindowHint);

    // 控件初始化
    QButtonGroup* btnGroup = new QButtonGroup(this);
    btnGroup->addButton(ui->toolBtn_profile, 0);
    btnGroup->addButton(ui->toolBtn_userManage, 1);
    btnGroup->addButton(ui->toolBtn_quesManage, 2);
    btnGroup->addButton(ui->toolBtn_quesUpload, 3);
    btnGroup->addButton(ui->toolBtn_createPaper, 4);

    // 绑定左侧按钮与右侧页面
    connect(btnGroup, QOverload<int>::of(&QButtonGroup::idClicked), ui->stackedWidget, &QStackedWidget::setCurrentIndex);
    emit ui->toolBtn_profile->clicked();

    // 设置默认显示的页面
    ui->stackedWidget->setCurrentIndex(0);

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

    connect(ui->btn_add1, &QPushButton::clicked, this, [=]() {
        ui->btn_add1->hide();
        ui->label_blank2->show();
        ui->lineEdit_blank2->show();
        ui->btn_add2->show();
    });
    connect(ui->btn_add2, &QPushButton::clicked, this, [=]() {
        ui->btn_add2->hide();
        ui->label_blank3->show();
        ui->lineEdit_blank3->show();
        ui->btn_add3->show();
    });
    connect(ui->btn_add3, &QPushButton::clicked, this, [=]() {
        ui->btn_add3->hide();
        ui->label_blank4->show();
        ui->lineEdit_blank4->show();
        ui->btn_add4->show();
    });
    connect(ui->btn_add4, &QPushButton::clicked, this, [=]() {
        ui->btn_add4->hide();
        ui->label_blank5->show();
        ui->lineEdit_blank5->show();
    });
    connect(ui->tBtn_addCase, &QToolButton::clicked, this, [=]() {
        int widgetNum = ui->vLayout_caseContainer->count();
        int visibleNum = 0;
        for(int i = 0; i < widgetNum; i++) {
            if(ui->vLayout_caseContainer->itemAt(i)->widget()->isVisible()) {
                visibleNum++;
            }
        }
        QTabWidget* tabWidget = new QTabWidget();
        QWidget* tab = new QWidget();
        tabWidget->addTab(tab, QString("案例%1").arg(visibleNum + 1));
        tabWidget->setTabsClosable(true);
        QFormLayout* formLayout = new QFormLayout(tab);
        QLineEdit* lineEdit_input = new QLineEdit();
        QLineEdit* lineEdit_output = new QLineEdit();
        lineEdit_input->setObjectName("input");
        lineEdit_output->setObjectName("output");
        formLayout->addRow("输入:", lineEdit_input);
        formLayout->addRow("输出:", lineEdit_output);
        ui->vLayout_caseContainer->insertWidget(widgetNum, tabWidget);

        connect(tabWidget, &QTabWidget::tabCloseRequested, this, [=]() {
            ui->vLayout_caseContainer->itemAt(widgetNum)->widget()->hide();

            for(int i = 0, j = 0; i < ui->vLayout_caseContainer->count(); i++) {
                if(ui->vLayout_caseContainer->itemAt(i)->widget()->isVisible()) {
                    j++;
                    QTabWidget* tabWidget = static_cast<QTabWidget*>(ui->vLayout_caseContainer->itemAt(i)->widget());
                    tabWidget->setTabText(0, QString("案例%1").arg(j));
                }
            }
        });
    });
    emit ui->tBtn_addCase->clicked();

    initChoiceUI();
    initCompletionUI();

    connect(ui->btn_choice_clear, &QPushButton::clicked, this, [=]() {
        initChoiceUI();
    });
    connect(ui->btn_completion_clear, &QPushButton::clicked, this, [=]() {
        initCompletionUI();
    });
    connect(ui->btn_program_clear, &QPushButton::clicked, this, [=]() {
        ui->textEdit_program_describe->clear();
    });

    connect(ui->btn_choice_upload, &QPushButton::clicked, this, [=]() {
        if(ui->textEdit_choice_question->toPlainText().isEmpty()) {
            QMessageBox::warning(this, "警告", "请输入内容");
        } else {
            uploadProblem(CATEGORY::CHOICE);
        }
    });
    connect(ui->btn_completion_upload, &QPushButton::clicked, this, [=]() {
        if(ui->textEdit_completion_question->toPlainText().isEmpty()) {
            QMessageBox::warning(this, "警告", "请输入内容");
        } else {
            uploadProblem(CATEGORY::COMPLETION);
        }
    });
    connect(ui->btn_program_upload, &QPushButton::clicked, this, [=]() {
        if(ui->textEdit_program_describe->toPlainText().isEmpty()) {
            QMessageBox::warning(this, "警告", "请输入内容");
        } else {
            uploadProblem(CATEGORY::PROGRAM);
        }
    });
    connect(ui->btn_userFolder, &QToolButton::clicked, this, [=]() {
        QDesktopServices::openUrl(QUrl(QCoreApplication::applicationDirPath() +  "./UserFolder/", QUrl::TolerantMode));
    });

    connect(ui->btn_createExam, &QPushButton::clicked, this, [=]() {
        std::vector<std::pair<int, std::string>> choiceData, completionData, programData;

        for(int i = 0; i < problemList_choice.size(); i++) {
            choiceData.push_back(std::make_pair(problemList_choice.at(i).pid, problemList_choice.at(i).content));
        }
        for(int i = 0; i < problemList_completion.size(); i++) {
            completionData.push_back(std::make_pair(problemList_completion.at(i).pid, problemList_completion.at(i).content));
        }
        for(int i = 0; i < problemList_program.size(); i++) {
            programData.push_back(std::make_pair(problemList_program.at(i).pid, problemList_program.at(i).content));
        }

        if(createExamDialog == nullptr) {
            createExamDialog = new CreateExamDialog(m_tcpClient, choiceData, completionData, programData);

            connect(createExamDialog, &CreateExamDialog::sig_created, this, [=]() {
                updateProblemTable(CATEGORY::EXAM);
            });

            createExamDialog->exec();
        }

        delete createExamDialog;
        createExamDialog = nullptr;
    });

    connect(ui->toolBtn_addUser, &QToolButton::clicked, this, [=]() {
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

    m_tcpClient = tcpClient;
    eventLoop = new QEventLoop(this);

    connect(m_tcpClient, &TCPClient::s_recvData, this, [=](QByteArray msg) {
        recvMsg = msg;
        if(eventLoop->isRunning()) {
            eventLoop->quit();
        }
    });

    QDir dir(QCoreApplication::applicationDirPath() +  "./UserFolder");
    if(!dir.exists()) {
        dir.mkdir(QCoreApplication::applicationDirPath() +  "./UserFolder");
    }
    QStringList fileType;
    fileType << "*.*";
    dir.setNameFilters(fileType);
    for(int i = 0; i < dir.entryInfoList().size(); i++) {
        QFile::remove(dir.entryInfoList().at(i).filePath());
    }

    v_layout = new QVBoxLayout(ui->widget_examList);
    getUserInfo(_uid);
    RequestStudentList();
    fillStudentTable();
    updateProblemTable(CATEGORY::CHOICE);
    updateProblemTable(CATEGORY::COMPLETION);
    updateProblemTable(CATEGORY::PROGRAM);
    updateProblemTable(CATEGORY::EXAM);
}

TeacherWindow::~TeacherWindow()
{
    delete ui;
}

void TeacherWindow::updateProblemTable(int category)
{
    switch(category) {
    case CATEGORY::CHOICE:
        getProblemDetail(CATEGORY::CHOICE);
        fillProblemTable(ui->table_choice, CATEGORY::CHOICE);
        break;
    case CATEGORY::COMPLETION:
        getProblemDetail(CATEGORY::COMPLETION);
        fillProblemTable(ui->table_completion, CATEGORY::COMPLETION);
        break;
    case CATEGORY::PROGRAM:
        getProblemDetail(CATEGORY::PROGRAM);
        fillProblemTable(ui->table_program, CATEGORY::PROGRAM);
        break;
    case CATEGORY::EXAM:
        getProblemDetail(CATEGORY::EXAM);
        fillExamList();
        break;
    }
}

void TeacherWindow::initChoiceUI()
{
    ui->textEdit_choice_question->clear();
    ui->textEdit_choice_optionA->clear();
    ui->textEdit_choice_optionB->clear();
    ui->textEdit_choice_optionC->clear();
    ui->textEdit_choice_optionD->clear();
    ui->textEdit_choice_analysis->clear();
    ui->combo_choice->setCurrentIndex(0);
}

void TeacherWindow::initCompletionUI()
{
    ui->label_blank2->hide();
    ui->label_blank3->hide();
    ui->label_blank4->hide();
    ui->label_blank5->hide();
    ui->lineEdit_blank2->hide();
    ui->lineEdit_blank3->hide();
    ui->lineEdit_blank4->hide();
    ui->lineEdit_blank5->hide();
    ui->btn_add1->show();
    ui->btn_add2->hide();
    ui->btn_add3->hide();
    ui->btn_add4->hide();

    ui->textEdit_completion_question->clear();
    ui->textEdit_completion_analysis->clear();
    ui->lineEdit_blank1->clear();
    ui->lineEdit_blank2->clear();
    ui->lineEdit_blank3->clear();
    ui->lineEdit_blank4->clear();
    ui->lineEdit_blank5->clear();
}

void TeacherWindow::getUserInfo(int uid)
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "RequireUserInfo";
    send_json["UID"] = uid;

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    m_tcpClient->sendToServer(send_msg);

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容
    if(recv_json.at("Reply") == "UserInfo") {
        std::string account = recv_json.at("Account");
        std::string date = recv_json.at("Date");

        ui->label_userID->setText(QString::fromStdString(account));
        ui->label_profileUID->setText(QString::number(uid));
        ui->label_profileAccount->setText(QString::fromStdString(account));
        ui->label_profileIdentity->setText("教师");
        ui->label_profileDate->setText(QString::fromStdString(date));
    }
}

void TeacherWindow::uploadProblem(int category)
{
    if(category == CATEGORY::CHOICE) {   // 选择题
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
        send_json["Event"] = "AddProblem";
        send_json["Category"] = category;
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
        if(recv_json.at("Reply") == "AddProblemSuccess") {
            QMessageBox::information(this, "information", "题目添加成功");
        }
    } else if(category == CATEGORY::COMPLETION) {  // 填空题
        QString describe = ui->textEdit_completion_question->toPlainText();
        QString answer = ui->lineEdit_blank1->text();
        if(!ui->lineEdit_blank2->isHidden() && !ui->lineEdit_blank2->text().isEmpty()) {
            answer += "|" + ui->lineEdit_blank2->text();
        }
        if(!ui->lineEdit_blank3->isHidden() && !ui->lineEdit_blank3->text().isEmpty()) {
            answer += "|" + ui->lineEdit_blank3->text();
        }
        if(!ui->lineEdit_blank4->isHidden() && !ui->lineEdit_blank4->text().isEmpty()) {
            answer += "|" + ui->lineEdit_blank4->text();
        }
        if(!ui->lineEdit_blank5->isHidden() && !ui->lineEdit_blank5->text().isEmpty()) {
            answer += "|" + ui->lineEdit_blank5->text();
        }
        QString analysis = ui->textEdit_completion_analysis->toPlainText();
        QString score = ui->spinBox_completion_score->text();

        // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
        json send_json;
        send_json["Event"] = "AddProblem";
        send_json["Category"] = category;
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
        if(recv_json.at("Reply") == "AddProblemSuccess") {
            QMessageBox::information(this, "information", "题目添加成功");
        }
    } else if(category == CATEGORY::PROGRAM) {  // 编程题
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
        send_json["Event"] = "AddProblem";
        send_json["Category"] = category;
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
        if(recv_json.at("Reply") == "AddProblemSuccess") {
            QMessageBox::information(this, "information", "题目添加成功");
            updateProblemTable(category);
        }
    }
}

void TeacherWindow::getProblemDetail(int category)
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "RequestProblemList";
    send_json["Identity"] = IDENTITY::TEACHER;
    send_json["Category"] = category;

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    m_tcpClient->sendToServer(send_msg);

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容，判断是否允许登录
    if(recv_json.at("Reply") == "ProblemList" && recv_json.at("Category") == category) {
        std::vector<std::string> problemList_db = recv_json.at("ProblemList");
        std::vector<int> scoreList_db = recv_json.at("ScoreList");
        std::vector<float> averageScoreList_db = recv_json.at("AverageScoreList");
        std::vector<int> answerNumList_db = recv_json.at("AnswerNumList");
        std::vector<int> pidList_db = recv_json.at("PIDList");

        switch(category) {
        case CATEGORY::CHOICE:
            problemList_choice.clear();
            break;
        case CATEGORY::COMPLETION:
            problemList_completion.clear();
            break;
        case CATEGORY::PROGRAM:
            problemList_program.clear();
            break;
        case CATEGORY::EXAM:
            problemList_exam.clear();
            break;
        }

        for(size_t x = 0; x < problemList_db.size(); x++) {
            ProblemDetail_Struct problem;
            problem.pid = pidList_db.at(x);
            problem.score = scoreList_db.at(x);
            problem.content = problemList_db.at(x);
            problem.averageScore = averageScoreList_db.at(x);
            problem.answerNum = answerNumList_db.at(x);

            switch(category) {
            case CATEGORY::CHOICE:
                problemList_choice.append(problem);
                break;
            case CATEGORY::COMPLETION:
                problemList_completion.append(problem);
                break;
            case CATEGORY::PROGRAM:
                problemList_program.append(problem);
                break;
            case CATEGORY::EXAM:
                problemList_exam.append(problem);
                break;
            }
        }
    }
}

void TeacherWindow::fillProblemTable(QTableWidget* tableWidget, int category)
{
    int N = 0;
    if(category == CATEGORY::CHOICE) {
        N = problemList_choice.size();
    } else if(category == CATEGORY::COMPLETION) {
        N = problemList_completion.size();
    } else if(category == CATEGORY::PROGRAM) {
        N = problemList_program.size();
    }

    // 清楚表格数据区中的所有内容，但是不清除表头
    tableWidget->clearContents();
    tableWidget->setRowCount(N);

    // 填入数据
    for(int i = 0; i < N; i++) {
        tableWidget->setRowHeight(i, 40);
        tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        tableWidget->item(i, 0)->setTextAlignment(Qt::AlignCenter);

        if(category == CATEGORY::CHOICE) {
            tableWidget->setItem(i, 1, new QTableWidgetItem(tr("选择题%1").arg(i + 1)));
            tableWidget->item(i, 1)->setTextAlignment(Qt::AlignCenter);

            tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(problemList_choice.at(i).score)));
            tableWidget->item(i, 2)->setTextAlignment(Qt::AlignCenter);

            tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(problemList_choice.at(i).answerNum)));
            tableWidget->item(i, 3)->setTextAlignment(Qt::AlignCenter);

            tableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(problemList_choice.at(i).averageScore)));
            tableWidget->item(i, 4)->setTextAlignment(Qt::AlignCenter);
        } else if(category == CATEGORY::COMPLETION) {
            tableWidget->setItem(i, 1, new QTableWidgetItem(tr("填空题%1").arg(i + 1)));
            tableWidget->item(i, 1)->setTextAlignment(Qt::AlignCenter);

            tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(problemList_completion.at(i).score)));
            tableWidget->item(i, 2)->setTextAlignment(Qt::AlignCenter);

            tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(problemList_completion.at(i).answerNum)));
            tableWidget->item(i, 3)->setTextAlignment(Qt::AlignCenter);

            tableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(problemList_completion.at(i).averageScore)));
            tableWidget->item(i, 4)->setTextAlignment(Qt::AlignCenter);
        } else if(category == CATEGORY::PROGRAM) {
            tableWidget->setItem(i, 1, new QTableWidgetItem(tr("编程题%1").arg(i + 1)));
            tableWidget->item(i, 1)->setTextAlignment(Qt::AlignCenter);

            tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(problemList_program.at(i).score)));
            tableWidget->item(i, 2)->setTextAlignment(Qt::AlignCenter);

            tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(problemList_program.at(i).answerNum)));
            tableWidget->item(i, 3)->setTextAlignment(Qt::AlignCenter);

            tableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(problemList_program.at(i).averageScore)));
            tableWidget->item(i, 4)->setTextAlignment(Qt::AlignCenter);
        }

        QPushButton* btn = new QPushButton("查看题目");
        tableWidget->setCellWidget(i, 5, btn);

        // 绑定按钮响应事件，通过位置找到按钮所在行
        connect(btn, &QPushButton::clicked, this, [=]() {
            QPushButton* btn = (QPushButton*)sender();
            int x = btn->frameGeometry().x();
            int y = btn->frameGeometry().y();
            QModelIndex index = tableWidget->indexAt(QPoint(x, y));
            int row = index.row();
            // int col = index.column();
            QString content;
            int category = 0;
            int pid = 0;
            int score = 0;
            if(ui->tabWidget_2->currentIndex() == 0) {
                content = QString::fromStdString(problemList_choice.at(row).content);
                pid = problemList_choice.at(row).pid;
                category = CATEGORY::CHOICE;
                score = problemList_choice.at(row).score;
            } else if(ui->tabWidget_2->currentIndex() == 1) {
                content = QString::fromStdString(problemList_completion.at(row).content);
                pid = problemList_completion.at(row).pid;
                category = CATEGORY::COMPLETION;
                score = problemList_completion.at(row).score;
            } else if(ui->tabWidget_2->currentIndex() == 2) {
                content = QString::fromStdString(problemList_program.at(row).content);
                pid = problemList_program.at(row).pid;
                category = CATEGORY::PROGRAM;
                score = problemList_program.at(row).score;
            }

            if(alterDialog == nullptr) {
                alterDialog = new AlterDialog(m_tcpClient, pid, category, content, score);

                connect(alterDialog, &AlterDialog::sig_altered, this, [=](CATEGORY i) {
                    switch(i) {
                    case CATEGORY::CHOICE:
                        updateProblemTable(CATEGORY::CHOICE);
                        break;
                    case CATEGORY::COMPLETION:
                        updateProblemTable(CATEGORY::COMPLETION);
                        break;
                    case CATEGORY::PROGRAM:
                        updateProblemTable(CATEGORY::PROGRAM);
                        break;
                    default:
                        break;
                    }
                });

                alterDialog->exec();
            }

            delete alterDialog;
            alterDialog = nullptr;
        });
    }
}

void TeacherWindow::fillExamList()
{
    // 首先清空布局中的所有控件
    while(v_layout->count()) {
        QWidget* pWidget = v_layout->itemAt(0)->widget();
        QSpacerItem* pSpacer = v_layout->itemAt(1)->spacerItem();
        pWidget->setParent(NULL);
        v_layout->removeWidget(pWidget);
        v_layout->removeItem(pSpacer);
        delete pWidget;
        delete pSpacer;
    }

    // 往布局中添加控件
    for(int i = 0; i < problemList_exam.size(); i++) {
        json content = json::parse(problemList_exam.at(i).content);
        QString title = QString::fromStdString(content.at("Title"));
        int time = content.at("Time");
        int time_h = time / 60;
        int time_m = time % 60;

        QWidget* cardWidget = new QWidget();
        cardWidget->setMinimumHeight(100);
        cardWidget->setMaximumHeight(200);
        QGridLayout* gridLayout = new QGridLayout();
        QLabel* label_title = new QLabel(title);
        label_title->setStyleSheet("font: 12pt bold;");
        QLabel* label_1 = new QLabel("总时长:");
        QLabel* label_duration = new QLabel(QString::number(time_h) + "小时" + QString::number(time_m) + "分");
        QLabel* label_2 = new QLabel("答卷人数:");
        QLabel* label_answerNum = new QLabel(QString::number(problemList_exam.at(i).answerNum));
        QLabel* label_3 = new QLabel("总分值:");
        QLabel* label_score = new QLabel(QString::number(problemList_exam.at(i).score));
        QLabel* label_4 = new QLabel("平均得分:");
        QLabel* label_point = new QLabel(QString::number(problemList_exam.at(i).averageScore));
        QPushButton* btn = new QPushButton("删除试卷");
        gridLayout->addWidget(label_title, 0, 0);
        gridLayout->addWidget(label_1, 1, 0);
        gridLayout->addWidget(label_duration, 1, 1);
        gridLayout->addWidget(label_2, 1, 2);
        gridLayout->addWidget(label_answerNum, 1, 3);
        gridLayout->addWidget(label_3, 2, 0);
        gridLayout->addWidget(label_score, 2, 1);
        gridLayout->addWidget(label_4, 2, 2);
        gridLayout->addWidget(label_point, 2, 3);
        gridLayout->addWidget(btn, 3, 3);
        cardWidget->setLayout(gridLayout);
        cardWidget->setObjectName("cardWidget");
        cardWidget->setStyleSheet("QWidget #cardWidget{border: 1px solid #000;border-radius: 10px;font-size: 10pt;}");
        v_layout->addWidget(cardWidget);
        v_layout->addStretch();

        connect(btn, &QPushButton::clicked, this, [=]() {
            // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
            json send_json;
            send_json["Event"] = "DeleteProblem";
            send_json["PID"] = problemList_exam.at(i).pid;

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
                QMessageBox::information(this, "information", "试卷删除成功");
                updateProblemTable(CATEGORY::EXAM);
            }
        });
    }
}

void TeacherWindow::addNewUser(QString account, QString password)
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
        RequestStudentList();
        fillStudentTable();
    } else if(recv_json.at("Reply") == "RefuseRegister") {
        QMessageBox::warning(this, "提示", "添加用户失败：该账号已存在！");
    }
}

void TeacherWindow::RequestStudentList()
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "RequestStudentList";

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    m_tcpClient->sendToServer(send_msg);

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容，判断是否允许登录
    if(recv_json.at("Reply") == "StudentList") {
        std::vector<std::string> lists = recv_json.at("List");

        studentList.clear();
        for(size_t i = 0; i < lists.size(); i++) {
            studentList.append(QString::fromStdString(lists.at(i)));
        }
    }
}

void TeacherWindow::fillStudentTable()
{
    int N = studentList.size();

    // 清楚表格数据区中的所有内容，但是不清除表头
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(N);

    // 填入数据
    for(int i = 0; i < N; i++) {
        QStringList stringList = studentList.at(i).split("|");

        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        ui->tableWidget->item(i, 0)->setTextAlignment(Qt::AlignCenter);

        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(stringList.at(0)));
        ui->tableWidget->item(i, 1)->setTextAlignment(Qt::AlignCenter);

        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(stringList.at(1)));
        ui->tableWidget->item(i, 2)->setTextAlignment(Qt::AlignCenter);

        QPushButton* btn_delete = new QPushButton("删除");
        ui->tableWidget->setCellWidget(i, 3, btn_delete);

        // 绑定按钮响应事件，通过位置找到按钮所在行
        connect(btn_delete, &QPushButton::clicked, this, [=]() {
            QPushButton* btn = (QPushButton*)sender();
            int x = btn->frameGeometry().x();
            int y = btn->frameGeometry().y();
            QModelIndex index = ui->tableWidget->indexAt(QPoint(x, y));
            int row = index.row();
            //int col = index.column();

            QStringList stringList = studentList.at(row).split("|");
            QString account = stringList.at(1);
            QString msg = QString("确认删除用户<b>%1</b>？").arg(account);
            QMessageBox::StandardButton result = QMessageBox::question(this, "Verify", msg, QMessageBox::Yes | QMessageBox::No);
            if(result == QMessageBox::StandardButton::Yes) {
                deleteUser(stringList.at(0).toInt());
            }
        });
    }
}

void TeacherWindow::deleteUser(int uid)
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
        RequestStudentList();
        fillStudentTable();
    }
}
