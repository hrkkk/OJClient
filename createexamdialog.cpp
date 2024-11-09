#include "createexamdialog.h"
#include "ui_createexamdialog.h"

CreateExamDialog::CreateExamDialog(TCPClient* tcpClient,
                                   const std::vector<std::pair<int, std::string>>& choiceList,
                                   const std::vector<std::pair<int, std::string>>& completionList,
                                   const std::vector<std::pair<int, std::string>>& programList, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateExamDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("选题组卷");
    this->setWindowFlags(Qt::WindowCloseButtonHint);
    this->showMaximized();

    for(size_t i = 0; i < choiceList.size(); i++) {
        json content_json = json::parse(choiceList.at(i).second);
        std::string des = content_json["Describe"];
        Problem_Struct temp = {choiceList.at(i).first, des};
        choiceProblem.push_back(temp);
    }
    for(size_t i = 0; i < completionList.size(); i++) {
        json content_json = json::parse(completionList.at(i).second);
        std::string des = content_json["Describe"];
        Problem_Struct temp = {completionList.at(i).first, des};
        completionProblem.push_back(temp);
    }
    for(size_t i = 0; i < programList.size(); i++) {
        json content_json = json::parse(programList.at(i).second);
        std::string des = content_json["Describe"];
        Problem_Struct temp = {programList.at(i).first, des};
        programProblem.push_back(temp);
    }

    connect(ui->spinBox_choiceScore, QOverload<int>::of(&QSpinBox::valueChanged), this, [=]() {
        calcTotalScore();
    });
    connect(ui->spinBox_completionScore, QOverload<int>::of(&QSpinBox::valueChanged), this, [=]() {
        calcTotalScore();
    });
    connect(ui->spinBox_programScore, QOverload<int>::of(&QSpinBox::valueChanged), this, [=]() {
        calcTotalScore();
    });
    connect(ui->btn_upload, &QPushButton::clicked, this, [=]() {
        uploadExam();
    });

    addCardWidget();

    m_tcpClient = tcpClient;

    eventLoop = new QEventLoop(this);

    connect(m_tcpClient, &TCPClient::s_recvData, this, [=](QByteArray msg) {
        recvMsg = msg;
        if(eventLoop->isRunning()) {
            eventLoop->quit();
        }
    });
}

CreateExamDialog::~CreateExamDialog()
{
    delete ui;
}

void CreateExamDialog::addCardWidget()
{
    for(int category = 0; category < 3; category++) {
        QVBoxLayout* main_layout;
        int n = 0;
        if(category == CATEGORY::CHOICE) {
            main_layout = new QVBoxLayout(ui->widget_choiceProblem);
            n = choiceProblem.size();
        } else if(category == CATEGORY::COMPLETION) {
            main_layout = new QVBoxLayout(ui->widget_completionProblem);
            n = completionProblem.size();
        } else if(category == CATEGORY::PROGRAM) {
            main_layout = new QVBoxLayout(ui->widget_programProblem);
            n = programProblem.size();
        }

        for(int i = 0; i < n; i++) {
            QWidget* cardWidget = new QWidget();
            QVBoxLayout* layout = new QVBoxLayout();
            QLabel* des = new QLabel();
            if(category == CATEGORY::CHOICE) {
                des->setText(QString::number(i + 1) + ". " + QString::fromStdString(choiceProblem.at(i).describe));
            } else if(category == CATEGORY::COMPLETION) {
                des->setText(QString::number(i + 1) + ". " + QString::fromStdString(completionProblem.at(i).describe));
            } else if(category == CATEGORY::PROGRAM) {
                des->setText(QString::number(i + 1) + ". " + QString::fromStdString(programProblem.at(i).describe));
            }
            QPushButton* btn = new QPushButton("+选题");
            layout->addWidget(des);
            layout->addWidget(btn, 1, Qt::AlignRight);
            cardWidget->setLayout(layout);
            cardWidget->setStyleSheet("QWidget{border:1px solid #000; border-radius:3px; background-color:#fff;} QLabel{border:none;} QPushButton{border:none; background-color:#4498ee;}");
            main_layout->addWidget(cardWidget);

            connect(btn, &QPushButton::clicked, this, [=]() {
                if(btn->text() == "+选题") {
                    btn->setText("-取消选题");
                    btn->setStyleSheet("QPushButton{background-color:red;}");

                    if(ui->tabWidget->currentIndex() == 0) {
                        Choose_Struct temp = { choiceProblem.at(i).pid, i};
                        choiceList.push_back(temp);
                    } else if(ui->tabWidget->currentIndex() == 1) {
                        Choose_Struct temp = { completionProblem.at(i).pid, i};
                        completionList.push_back(temp);
                    } else if(ui->tabWidget->currentIndex() == 2) {
                        Choose_Struct temp = { programProblem.at(i).pid, i};
                        programList.push_back(temp);
                    }
                } else {
                    btn->setText("+选题");
                    btn->setStyleSheet("QPushButton{background-color:#4498ee;}");

                    if(ui->tabWidget->currentIndex() == 0) {
                        auto iter = std::find_if(choiceList.begin(), choiceList.end(), [=](const Choose_Struct& x) {
                            return x.orderNum == i;
                        });
                        choiceList.erase(iter);
                    } else if(ui->tabWidget->currentIndex() == 1) {
                        auto iter = std::find_if(completionList.begin(), completionList.end(), [=](const Choose_Struct& x) {
                            return x.orderNum == i;
                        });
                        completionList.erase(iter);
                    } else if(ui->tabWidget->currentIndex() == 2) {
                        auto iter = std::find_if(programList.begin(), programList.end(), [=](const Choose_Struct& x) {
                            return x.orderNum == i;
                        });
                        programList.erase(iter);
                    }
                }

                int N = 0;
                if(ui->tabWidget->currentIndex() == 0) {
                    ui->tableWidget_choice->clearContents();
                    N = choiceList.size();
                } else if(ui->tabWidget->currentIndex() == 1) {
                    ui->tableWidget_completion->clearContents();
                    N = completionList.size();
                } else if(ui->tabWidget->currentIndex() == 2) {
                    ui->tableWidget_program->clearContents();
                    N = programList.size();
                }

                for(int i = 0; i < N; i++) {
                    int row = i / 5;
                    int col = i % 5;
                    QTableWidgetItem* item;
                    if(ui->tabWidget->currentIndex() == 0) {
                        item = new QTableWidgetItem(QString::number(choiceList.at(i).orderNum));
                        item->setBackground(QBrush(QColor("#55aaff")));
                        ui->tableWidget_choice->setItem(row, col, item);
                        ui->tableWidget_choice->setRowHeight(row, 40);
                        ui->tableWidget_choice->setColumnWidth(col, 40);
                        ui->tableWidget_choice->item(row, col)->setTextAlignment(Qt::AlignCenter);
                    } else if(ui->tabWidget->currentIndex() == 1) {
                        item = new QTableWidgetItem(QString::number(completionList.at(i).orderNum));
                        item->setBackground(QBrush(QColor("#55aaff")));
                        ui->tableWidget_completion->setItem(row, col, item);
                        ui->tableWidget_completion->setRowHeight(row, 40);
                        ui->tableWidget_completion->setColumnWidth(col, 40);
                        ui->tableWidget_completion->item(row, col)->setTextAlignment(Qt::AlignCenter);
                    } else if(ui->tabWidget->currentIndex() == 2) {
                        item = new QTableWidgetItem(QString::number(programList.at(i).orderNum));
                        item->setBackground(QBrush(QColor("#55aaff")));
                        ui->tableWidget_program->setItem(row, col, item);
                        ui->tableWidget_program->setRowHeight(row, 40);
                        ui->tableWidget_program->setColumnWidth(col, 40);
                        ui->tableWidget_program->item(row, col)->setTextAlignment(Qt::AlignCenter);
                    }
                }
                calcTotalScore();
            });
        }
    }
}

void CreateExamDialog::calcTotalScore()
{
    int choiceScore = ui->spinBox_choiceScore->value();
    int completionScore = ui->spinBox_completionScore->value();
    int programScore = ui->spinBox_programScore->value();

    int choiceNum = choiceList.size();
    int completionNum = completionList.size();
    int programNum = programList.size();

    int totalScore = choiceScore * choiceNum + completionScore * completionNum + programScore * programNum;
    ui->lineEdit_totalScore->setText(QString::number(totalScore));
}

void CreateExamDialog::uploadExam()
{
    int time = ui->timeEdit->time().hour() * 60 + ui->timeEdit->time().minute();
    int score = ui->lineEdit_totalScore->text().toInt();
    std::string title = ui->lineEdit_title->text().toStdString();
    std::vector<int> scoreList;
    scoreList.push_back(ui->spinBox_choiceScore->value());
    scoreList.push_back(ui->spinBox_completionScore->value());
    scoreList.push_back(ui->spinBox_programScore->value());

    std::vector<int> _choiceList, _completionList, _programList;
    for(size_t i = 0; i < choiceList.size(); i++) {
        _choiceList.push_back(choiceList.at(i).pid);
    }
    for(size_t i = 0; i < completionList.size(); i++) {
        _completionList.push_back(completionList.at(i).pid);
    }
    for(size_t i = 0; i < programList.size(); i++) {
        _programList.push_back(programList.at(i).pid);
    }
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "AddProblem";
    send_json["Category"] = CATEGORY::EXAM;
    send_json["Score"] = score;
    json content_json;
    content_json["Title"] = title;
    content_json["ScoreList"] = scoreList;
    content_json["ChoiceList"] = _choiceList;
    content_json["CompletionList"] = _completionList;
    content_json["ProgramList"] = _programList;
    content_json["Time"] = time;
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
        QMessageBox::information(this, "information", "试卷上传成功");
        emit sig_created();
    }
}
