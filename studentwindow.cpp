#include "studentwindow.h"
#include "ui_studentwindow.h"

#include <QDebug>

StudentWindow::StudentWindow(TCPClient* tcpClient, int uid, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StudentWindow)
{
    // 界面初始化
    ui->setupUi(this);

    this->setWindowFlag(Qt::FramelessWindowHint);
    ui->stackedWidget->setCurrentIndex(0);
    ui->stackedWidget_2->setCurrentIndex(0);

    // 控件初始化
    QButtonGroup* btnGroup = new QButtonGroup(this);
    btnGroup->addButton(ui->toolBtn_profile, 0);
    btnGroup->addButton(ui->toolBtn_practice, 1);
    btnGroup->addButton(ui->toolBtn_exam, 2);

    // 变量赋值
    m_tcpClient = tcpClient;
    m_uid = uid;
    eventLoop = new QEventLoop(this);

    // 信号绑定
    // 绑定左侧按钮与右侧页面
    connect(btnGroup, QOverload<int>::of(&QButtonGroup::idClicked), ui->stackedWidget, &QStackedWidget::setCurrentIndex);
    emit ui->toolBtn_profile->clicked();

    connect(ui->btn_min, &QPushButton::clicked, this, [=]() {
        this->showMinimized();
    });

    connect(ui->btn_resize, &QPushButton::clicked, this, [=]() {
        if(this->isMaximized()) {
            this->showNormal();
        } else {
            this->showMaximized();
        }
    });

    connect(ui->btn_close, &QPushButton::clicked, this, [=]() {
        this->close();
    });

    connect(ui->comboBox_showCategory, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
        ui->stackedWidget_2->setCurrentIndex(index);
    });

    connect(ui->btn_expand_1, &QToolButton::clicked, this, [=](){
        if(ui->scrollArea->isVisible()) {
            ui->btn_expand_1->setIcon(QIcon(":/resource/left.png"));
            ui->scrollArea->setVisible(false);
            if(!ui->scrollArea_2->isVisible()) {
                ui->widget_9->setVisible(true);
            }
        } else {
            ui->btn_expand_1->setIcon(QIcon(":/resource/down.png"));
            ui->scrollArea->setVisible(true);
            ui->widget_9->setVisible(false);
        }
    });

    connect(ui->btn_expand_2, &QToolButton::clicked, this, [=](){
        if(ui->scrollArea_2->isVisible()) {
            ui->btn_expand_2->setIcon(QIcon(":/resource/left.png"));
            ui->scrollArea_2->setVisible(false);
            if(!ui->scrollArea->isVisible()) {
                ui->widget_9->setVisible(true);
            }
        } else {
            ui->btn_expand_2->setIcon(QIcon(":/resource/down.png"));
            ui->scrollArea_2->setVisible(true);
            ui->widget_9->setVisible(false);
        }
    });
    emit ui->btn_expand_1->clicked();
    emit ui->btn_expand_2->clicked();

    connect(m_tcpClient, &TCPClient::s_recvData, this, [=](QByteArray msg) {
        recvMsg = msg;
        if(eventLoop->isRunning()) {
            eventLoop->quit();
        }
    });

    connect(ui->btn_clearRecord, &QPushButton::clicked, this, [=]() {
        QMessageBox::StandardButton result = QMessageBox::question(this, "Verify", "确认删除个人答题记录？", QMessageBox::Yes | QMessageBox::No);
        if(result == QMessageBox::StandardButton::Yes) {
            clearResponseRecord();
        }
    });

    // 创建用户文件夹，清除文件夹下的所有文件
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
    v_layout_res = new QVBoxLayout(ui->widget_examList_res);

    getUserInfo(m_uid);
    updateProblemTable(CATEGORY::CHOICE);
    updateProblemTable(CATEGORY::COMPLETION);
    updateProblemTable(CATEGORY::PROGRAM);
    updateProblemTable(CATEGORY::EXAM);
}

StudentWindow::~StudentWindow()
{
    delete ui;
}

void StudentWindow::updateProblemTable(int category)
{
    requestProblemList(category);
    getResponseRecord();
    switch(category) {
    case CATEGORY::CHOICE:
        fillTable(ui->table_choice, 0);
        break;
    case CATEGORY::COMPLETION:
        fillTable(ui->table_completion, 1);
        break;
    case CATEGORY::PROGRAM:
        fillTable(ui->table_program, 2);
        break;
    case CATEGORY::EXAM:
        fillExamList();
        break;
    }
}

void StudentWindow::initTable(QTableWidget* tableWidget)
{
    tableWidget->resizeRowsToContents();
    tableWidget->horizontalHeader()->setFixedHeight(30);
    tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section{background:white;}");
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    tableWidget->setColumnWidth(0, 60);
    tableWidget->setColumnWidth(0, 160);
    tableWidget->setColumnWidth(0, 100);
    tableWidget->setColumnWidth(0, 100);
}

void StudentWindow::fillTable(QTableWidget* tableWidget, int category)
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

            if(problemList_choice.at(i).isResponse) {
                tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(problemList_choice.at(i).point)));
            } else {
                tableWidget->setItem(i, 3, new QTableWidgetItem("-"));
            }
            tableWidget->item(i, 3)->setTextAlignment(Qt::AlignCenter);
        } else if(category == CATEGORY::COMPLETION) {
            tableWidget->setItem(i, 1, new QTableWidgetItem(tr("填空题%1").arg(i + 1)));
            tableWidget->item(i, 1)->setTextAlignment(Qt::AlignCenter);

            tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(problemList_completion.at(i).score)));
            tableWidget->item(i, 2)->setTextAlignment(Qt::AlignCenter);

            if(problemList_completion.at(i).isResponse) {
                tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(problemList_completion.at(i).point)));
            } else {
                tableWidget->setItem(i, 3, new QTableWidgetItem("-"));
            }
            tableWidget->item(i, 3)->setTextAlignment(Qt::AlignCenter);
        } else if(category == CATEGORY::PROGRAM) {
            tableWidget->setItem(i, 1, new QTableWidgetItem(tr("编程题%1").arg(i + 1)));
            tableWidget->item(i, 1)->setTextAlignment(Qt::AlignCenter);

            tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(problemList_program.at(i).score)));
            tableWidget->item(i, 2)->setTextAlignment(Qt::AlignCenter);

            if(problemList_program.at(i).isResponse) {
                tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(problemList_program.at(i).point)));
            } else {
                tableWidget->setItem(i, 3, new QTableWidgetItem("-"));
            }
            tableWidget->item(i, 3)->setTextAlignment(Qt::AlignCenter);
        }

        QPushButton* btn = new QPushButton("答题");
        tableWidget->setCellWidget(i, 4, btn);

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
            if(ui->stackedWidget_2->currentIndex() == 0) {
                content = QString::fromStdString(problemList_choice.at(row).content);
                pid = problemList_choice.at(row).pid;
                score = problemList_choice.at(row).score;
                category = CATEGORY::CHOICE;
            } else if(ui->stackedWidget_2->currentIndex() == 1) {
                content = QString::fromStdString(problemList_completion.at(row).content);
                pid = problemList_completion.at(row).pid;
                score = problemList_completion.at(row).score;
                category = CATEGORY::COMPLETION;
            } else if(ui->stackedWidget_2->currentIndex() == 2) {
                content = QString::fromStdString(problemList_program.at(row).content);
                pid = problemList_program.at(row).pid;
                score = problemList_program.at(row).score;
                category = CATEGORY::PROGRAM;
            }

            if(answerDialog == nullptr) {
                answerDialog = new AnswerDialog(m_tcpClient, m_uid, pid, row, category, score, content);

                connect(answerDialog, &AnswerDialog::sig_submitted, this, [=](CATEGORY i) {
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

                answerDialog->exec();
            }

            delete answerDialog;
            answerDialog = nullptr;
        });
    }
}

void StudentWindow::getUserInfo(int uid)
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
        ui->label_profileIdentity->setText("学生");
        ui->label_profileDate->setText(QString::fromStdString(date));
    }
}

void StudentWindow::clearResponseRecord()
{
    std::vector<int> recordList;

    for(int i = 0; i < problemList_choice.size(); i++) {
        if(problemList_choice.at(i).isResponse) {
            recordList.push_back(problemList_choice.at(i).pid);
        }
    }

    for(int i = 0; i < problemList_completion.size(); i++) {
        if(problemList_completion.at(i).isResponse) {
            recordList.push_back(problemList_completion.at(i).pid);
        }
    }

    for(int i = 0; i < problemList_program.size(); i++) {
        if(problemList_program.at(i).isResponse) {
            recordList.push_back(problemList_program.at(i).pid);
        }
    }

    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "ClearResponseRecord";
    send_json["UID"] = m_uid;
    send_json["RecordList"] = recordList;

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    m_tcpClient->sendToServer(send_msg);

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容
    if(recv_json.at("Reply") == "ClearResponseSuccess") {
        QMessageBox::information(this, "提示", "删除个人答题记录成功！");
    }
}

void StudentWindow::requestProblemList(int category)
{
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

    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "RequestProblemList";
    send_json["Category"] = category;
    send_json["Identity"] = IDENTITY::STUDENT;

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
        std::vector<int> pidList_db = recv_json.at("PIDList");

        for(size_t x = 0; x < problemList_db.size(); x++) {
            Problem_Struct problem;
            problem.pid = pidList_db.at(x);
            problem.score = scoreList_db.at(x);
            problem.content = problemList_db.at(x);

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

void StudentWindow::getResponseRecord()
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "RequireRecord";
    send_json["UID"] = m_uid;

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    m_tcpClient->sendToServer(send_msg);

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容
    if(recv_json.at("Reply") == "Record") {
        std::vector<int> pidList = recv_json.at("PIDList");
        std::vector<int> pointList = recv_json.at("PointList");
        std::vector<std::string> responseList = recv_json.at("ResponseList");

        for(int i = 0; i < problemList_choice.size(); i++) {
            auto iter = std::find(pidList.begin(), pidList.end(), problemList_choice.at(i).pid);

            if(iter != pidList.end()) {     // 如果该题有答题记录
                int index = iter - pidList.begin();
                problemList_choice[i].isResponse = true;
                problemList_choice[i].point = pointList.at(index);
                problemList_choice[i].response = responseList.at(index);
            } else {
                problemList_choice[i].isResponse = false;
            }
        }

        for(int i = 0; i < problemList_completion.size(); i++) {
            auto iter = std::find(pidList.begin(), pidList.end(), problemList_completion.at(i).pid);

            if(iter != pidList.end()) {     // 如果该题有答题记录
                int index = iter - pidList.begin();
                problemList_completion[i].isResponse = true;
                problemList_completion[i].point = pointList.at(index);
                problemList_completion[i].response = responseList.at(index);
            } else {
                problemList_completion[i].isResponse = false;
            }
        }

        for(int i = 0; i < problemList_program.size(); i++) {
            auto iter = std::find(pidList.begin(), pidList.end(), problemList_program.at(i).pid);

            if(iter != pidList.end()) {     // 如果该题有答题记录
                int index = iter - pidList.begin();
                problemList_program[i].isResponse = true;
                problemList_program[i].point = pointList.at(index);
                problemList_program[i].response = responseList.at(index);
            } else {
                problemList_program[i].isResponse = false;
            }
        }

        for(int i = 0; i < problemList_exam.size(); i++) {
            auto iter = std::find(pidList.begin(), pidList.end(), problemList_exam.at(i).pid);

            if(iter != pidList.end()) {     // 如果该题有答题记录
                int index = iter - pidList.begin();
                problemList_exam[i].isResponse = true;
                problemList_exam[i].point = pointList.at(index);
                problemList_exam[i].response = responseList.at(index);
            } else {
                problemList_exam[i].isResponse = false;
            }
        }
    }
}

void StudentWindow::fillExamList()
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

    while(v_layout_res->count()) {
        QWidget* pWidget = v_layout_res->itemAt(0)->widget();
        QSpacerItem* pSpacer = v_layout_res->itemAt(1)->spacerItem();
        pWidget->setParent(NULL);
        v_layout_res->removeWidget(pWidget);
        v_layout_res->removeItem(pSpacer);
        delete pWidget;
        delete pSpacer;
    }

    for(int i = 0; i < problemList_exam.size(); i++) {
        json content = json::parse(problemList_exam.at(i).content);
        QString title = QString::fromStdString(content.at("Title"));
        int score = problemList_exam.at(i).score;
        int point = problemList_exam.at(i).point;
        int time = content.at("Time");
        int time_h = time / 60;
        int time_m = time % 60;

        if(problemList_exam.at(i).isResponse) {
            QWidget* cardWidget = new QWidget();
            cardWidget->setMinimumHeight(100);
            cardWidget->setMaximumHeight(200);
            QGridLayout* gridLayout = new QGridLayout();
            QLabel* label_title = new QLabel(title);
            label_title->setStyleSheet("font: 12pt bold;");
            QLabel* label_1 = new QLabel("总时长:");
            QLabel* label_duration = new QLabel(QString::number(time_h) + "小时" + QString::number(time_m) + "分");
            QLabel* label_2 = new QLabel("总分值:");
            QLabel* label_score = new QLabel(QString::number(score));
            QLabel* label_3 = new QLabel("得分:");
            QLabel* label_point = new QLabel(QString::number(point));
            QPushButton* btn = new QPushButton("重新答题");
            gridLayout->addWidget(label_title, 0, 0);
            gridLayout->addWidget(label_1, 1, 0);
            gridLayout->addWidget(label_duration, 1, 1);
            gridLayout->addWidget(label_2, 1, 2);
            gridLayout->addWidget(label_score, 1, 3);
            gridLayout->addWidget(label_3, 1, 4);
            gridLayout->addWidget(label_point, 1, 5);
            gridLayout->addWidget(btn, 2, 5);
            cardWidget->setLayout(gridLayout);
            cardWidget->setObjectName("cardWidget");
            cardWidget->setStyleSheet("QWidget #cardWidget{border: 1px solid #000;border-radius: 10px;font-size: 10pt;}");
            v_layout_res->addWidget(cardWidget);
            v_layout_res->addStretch();

            connect(btn, &QPushButton::clicked, this, [=](){
                if(examDialog == nullptr) {
                    examDialog = new ExamDialog(m_tcpClient, m_uid, problemList_exam.at(i).pid, problemList_exam.at(i).content);

                    connect(examDialog, &ExamDialog::sig_examSubmitted, this, [=]() {
                        updateProblemTable(CATEGORY::EXAM);
                    });

                    examDialog->exec();
                }

                delete examDialog;
                examDialog = nullptr;
            });
        } else {
            QWidget* cardWidget = new QWidget();
            cardWidget->setMinimumHeight(100);
            cardWidget->setMaximumHeight(200);
            QGridLayout* gridLayout = new QGridLayout();
            QLabel* label_title = new QLabel(title);
            label_title->setStyleSheet("font: 12pt bold;");
            QLabel* label_1 = new QLabel("总时长:");
            QLabel* label_duration = new QLabel(QString::number(time_h) + "小时" + QString::number(time_m) + "分");
            QLabel* label_2 = new QLabel("总分值:");
            QLabel* label_score = new QLabel(QString::number(score));
            QPushButton* btn = new QPushButton("开始考试");
            gridLayout->addWidget(label_title, 0, 0);
            gridLayout->addWidget(label_1, 1, 0);
            gridLayout->addWidget(label_duration, 1, 1);
            gridLayout->addWidget(label_2, 1, 2);
            gridLayout->addWidget(label_score, 1, 3);
            gridLayout->addWidget(btn, 2, 3);
            cardWidget->setLayout(gridLayout);
            cardWidget->setObjectName("cardWidget");
            cardWidget->setStyleSheet("QWidget #cardWidget{border: 1px solid #000;border-radius: 10px;font-size: 10pt;}");
            v_layout->addWidget(cardWidget);
            v_layout->addStretch();

            connect(btn, &QPushButton::clicked, this, [=](){
                if(examDialog == nullptr) {
                    examDialog = new ExamDialog(m_tcpClient, m_uid, problemList_exam.at(i).pid, problemList_exam.at(i).content);

                    connect(examDialog, &ExamDialog::sig_examSubmitted, this, [=]() {
                        updateProblemTable(CATEGORY::EXAM);
                    });

                    examDialog->exec();
                }

                delete examDialog;
                examDialog = nullptr;
            });
        }
    }
}
