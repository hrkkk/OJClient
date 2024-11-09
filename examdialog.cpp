#include "examdialog.h"
#include "ui_examdialog.h"

ExamDialog::ExamDialog(TCPClient* tcpClient, int uid, int pid, std::string content, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExamDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("模拟考试");
    this->setWindowFlags(Qt::WindowCloseButtonHint);
    this->showMaximized();

    m_uid = uid;
    m_pid = pid;
    m_tcpClient = tcpClient;
    eventLoop = new QEventLoop(this);

    connect(m_tcpClient, &TCPClient::s_recvData, this, [=](QByteArray msg) {
        recvMsg = msg;
        if(eventLoop->isRunning()) {
            eventLoop->quit();
        }
    });

    json content_json = json::parse(content);
    std::vector<int> scoreList = content_json.at("ScoreList");
    std::vector<int> choiceList = content_json.at("ChoiceList");
    std::vector<int> completionList = content_json.at("CompletionList");
    std::vector<int> programList = content_json.at("ProgramList");
    m_time = content_json.at("Time");
    m_time *= 60;

    m_choiceScore = scoreList.at(0);
    m_completionScore = scoreList.at(1);
    m_programScore = scoreList.at(2);

    m_choiceNum = choiceList.size();
    m_completionNum = completionList.size();
    m_programNum = programList.size();

    int totalNum = m_choiceNum + m_completionNum + m_programNum;
    for(int i = 0; i < totalNum; i++) {
        m_responseList.append("");
    }

    for(size_t i = 0; i < choiceList.size(); i++) {
        m_choiceProblem.append({static_cast<int>(i + 1), choiceList.at(i), ""});
    }
    for(size_t i = 0; i < completionList.size(); i++) {
        m_completionProblem.append({static_cast<int>(m_choiceNum + i + 1), completionList.at(i), ""});
    }
    for(size_t i = 0; i < programList.size(); i++) {
        m_programProblem.append({static_cast<int>(m_choiceNum + m_completionNum + i + 1), programList.at(i), ""});
    }

    m_lastPointer[0] = -1;
    m_lastPointer[1] = -1;
    m_curPointer[0] = 0;
    m_curPointer[1] = 0;

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

    getProblemContent(choiceList, completionList, programList);
    fillTable();
    writeToFile();
    changeProblem();

    connect(ui->btn_last, &QPushButton::clicked, this, [=]() {
        if(m_curPointer[0] == 0 && m_curPointer[1] == 0) {
            m_lastPointer[0] = m_curPointer[0];
            m_lastPointer[1] = m_curPointer[1];
            m_curPointer[0] = 2;
            m_curPointer[1] = m_programNum - 1;
        } else if(m_curPointer[0] == 1 && m_curPointer[1] == 0) {
            m_lastPointer[0] = m_curPointer[0];
            m_lastPointer[1] = m_curPointer[1];
            m_curPointer[0] = 0;
            m_curPointer[1] = m_choiceNum - 1;
        } else if(m_curPointer[0] == 2 && m_curPointer[1] == 0) {
            m_lastPointer[0] = m_curPointer[0];
            m_lastPointer[1] = m_curPointer[1];
            m_curPointer[0] = 1;
            m_curPointer[1] = m_completionNum - 1;
        } else {
            m_lastPointer[0] = m_curPointer[0];
            m_lastPointer[1] = m_curPointer[1];
            m_curPointer[1]--;
        }
        changeProblem();
    });

    connect(ui->btn_next, &QPushButton::clicked, this, [=]() {
        if(m_curPointer[0] == 0 && m_curPointer[1] == m_choiceNum - 1) {
            m_lastPointer[0] = m_curPointer[0];
            m_lastPointer[1] = m_curPointer[1];
            m_curPointer[0] = 1;
            m_curPointer[1] = 0;
        } else if(m_curPointer[0] == 1 && m_curPointer[1] == m_completionNum - 1) {
            m_lastPointer[0] = m_curPointer[0];
            m_lastPointer[1] = m_curPointer[1];
            m_curPointer[0] = 2;
            m_curPointer[1] = 0;
        } else if(m_curPointer[0] == 2 && m_curPointer[1] == m_programNum - 1) {
            m_lastPointer[0] = m_curPointer[0];
            m_lastPointer[1] = m_curPointer[1];
            m_curPointer[0] = 0;
            m_curPointer[1] = 0;
        } else {
            m_lastPointer[0] = m_curPointer[0];
            m_lastPointer[1] = m_curPointer[1];
            m_curPointer[1]++;
        }
        changeProblem();
    });

    connect(ui->btn_submit, &QPushButton::clicked, this, [=]() {
        QMessageBox::StandardButton result = QMessageBox::question(this, "Verify", "是否确认交卷？", QMessageBox::Yes | QMessageBox::No);
        if(result == QMessageBox::StandardButton::Yes) {
            m_lastPointer[0] = m_curPointer[0];
            m_lastPointer[1] = m_curPointer[1];
            saveResponse();
            submitExamResponse();
        }
    });

    connect(ui->tBtn_userFolder, &QToolButton::clicked, this, [=]() {
        QDesktopServices::openUrl(QUrl(QCoreApplication::applicationDirPath() +  "./UserFolder/", QUrl::TolerantMode));
    });

    timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, [=]() {
        if(this->m_time == 0) {
            QMessageBox::information(this, "information", "考试时间到，已自动交卷。");
            this->close();
        }
        this->m_time -= 1;
        int seconds = m_time;
        int time_h = seconds / 3600;
        seconds %= 3600;
        int time_m = seconds / 60;
        int time_s = seconds % 60;
        QString time_str = QString("%1:%2:%3").arg(time_h, 2, 10, QLatin1Char('0'))
                .arg(time_m, 2, 10, QLatin1Char('0')).arg(time_s, 2, 10, QLatin1Char('0'));
        ui->label_time->setText(time_str);
    });

    int seconds = m_time;
    int time_h = seconds / 3600;
    seconds %= 3600;
    int time_m = seconds / 60;
    int time_s = seconds % 60;
    QString time_str = QString("%1:%2:%3").arg(time_h, 2, 10, QLatin1Char('0'))
            .arg(time_m, 2, 10, QLatin1Char('0')).arg(time_s, 2, 10, QLatin1Char('0'));
    ui->label_time->setText(time_str);

    timer->start(1000);
}

ExamDialog::~ExamDialog()
{
    delete ui;
}

void ExamDialog::getProblemContent(const std::vector<int>& choiceList, const std::vector<int>& completionList, const std::vector<int>& programList)
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "RequireExamContent";
    send_json["ChoiceNum"] = choiceList.size();
    send_json["CompletionNum"] = completionList.size();
    send_json["ProgramNum"] = programList.size();
    send_json["ChoicePIDList"] = choiceList;
    send_json["CompletionPIDList"] = completionList;
    send_json["ProgramPIDList"] = programList;

    QByteArray send_msg = QString::fromStdString(send_json.dump()).toUtf8();

    // 发送数据到服务器
    m_tcpClient->sendToServer(send_msg);

    // 事件循环，阻塞当前函数，等待服务器响应
    eventLoop->exec();

    // 将服务器返回的二进制字节流解析为JSON格式
    std::string recv_msg = QString(recvMsg).toStdString();

    json recv_json = json::parse(recv_msg.c_str());

    // 解析JSON内容
    if(recv_json.at("Reply") == "ExamContent") {
        std::vector<std::string> choiceContent, completionContent, programContent;

        if(recv_json.at("ChoiceNum") != 0) {
            choiceContent = recv_json.at("ChoiceContentList");
        }
        if(recv_json.at("CompletionNum") != 0) {
            completionContent = recv_json.at("CompletionContentList");
        }
        if(recv_json.at("ProgramNum") != 0) {
            programContent = recv_json.at("ProgramContentList");
        }

        for(size_t i = 0; i < choiceContent.size(); i++) {
            m_choiceProblem[i].content = choiceContent.at(i);
        }

        for(size_t i = 0; i < completionContent.size(); i++) {
            m_completionProblem[i].content = completionContent.at(i);
        }

        for(size_t i = 0; i < programContent.size(); i++) {
            m_programProblem[i].content = programContent.at(i);
        }
    }
}

void ExamDialog::writeToFile()
{
    for(int i = 0; i < m_programNum; i++) {
        QFile file(QCoreApplication::applicationDirPath() +  QString("./UserFolder/test_%1.c").arg(i + 1));
        file.open(QIODevice::ReadWrite | QIODevice::Text);

        std::string content = m_programProblem.at(i).content;
        json content_json = json::parse(content);
        QString templateCode = QString::fromStdString(content_json.at("Template"));
        file.write(templateCode.toLocal8Bit());
        file.close();
    }
}

void ExamDialog::fillTable()
{
    ui->tableWidget_choice->clearContents();
    ui->tableWidget_completion->clearContents();
    ui->tableWidget_program->clearContents();

    for(int i = 0; i < m_choiceNum; i++) {
        int row = i / 5;
        int col = i % 5;
        QPushButton* btn = new QPushButton(QString::number(m_choiceProblem.at(i).order));
        btn->setStyleSheet("background-color:#f0f0f0;");
        ui->tableWidget_choice->setCellWidget(row, col, btn);

        connect(btn, &QPushButton::clicked, this, [=]() {
            QPushButton* btn = (QPushButton*)sender();
            int x = btn->frameGeometry().x();
            int y = btn->frameGeometry().y();
            QModelIndex index = ui->tableWidget_choice->indexAt(QPoint(x, y));
            int row = index.row();
            int col = index.column();

            m_lastPointer[0] = m_curPointer[0];
            m_lastPointer[1] = m_curPointer[1];
            m_curPointer[0] = 0;
            m_curPointer[1] = row * 5 + col;

            changeProblem();
        });
    }

    for(int i = 0; i < m_completionNum; i++) {
        int row = i / 5;
        int col = i % 5;
        QPushButton* btn = new QPushButton(QString::number(m_completionProblem.at(i).order));
        btn->setStyleSheet("background-color:#f0f0f0;");
        ui->tableWidget_completion->setCellWidget(row, col, btn);

        connect(btn, &QPushButton::clicked, this, [=]() {
            QPushButton* btn = (QPushButton*)sender();
            int x = btn->frameGeometry().x();
            int y = btn->frameGeometry().y();
            QModelIndex index = ui->tableWidget_completion->indexAt(QPoint(x, y));
            int row = index.row();
            int col = index.column();

            m_lastPointer[0] = m_curPointer[0];
            m_lastPointer[1] = m_curPointer[1];
            m_curPointer[0] = 1;
            m_curPointer[1] = row * 5 + col;

            changeProblem();
        });
    }

    for(int i = 0; i < m_programNum; i++) {
        int row = i / 5;
        int col = i % 5;
        QPushButton* btn = new QPushButton(QString::number(m_programProblem.at(i).order));
        btn->setStyleSheet("background-color:#f0f0f0;");
        ui->tableWidget_program->setCellWidget(row, col, btn);

        connect(btn, &QPushButton::clicked, this, [=]() {
            QPushButton* btn = (QPushButton*)sender();
            int x = btn->frameGeometry().x();
            int y = btn->frameGeometry().y();
            QModelIndex index = ui->tableWidget_program->indexAt(QPoint(x, y));
            int row = index.row();
            int col = index.column();

            m_lastPointer[0] = m_curPointer[0];
            m_lastPointer[1] = m_curPointer[1];
            m_curPointer[0] = 2;
            m_curPointer[1] = row * 5 + col;

            changeProblem();
        });
    }
}

void ExamDialog::clearResponseArea(int index)
{
    if(index == 0) {
        ui->rBtn_A->setAutoExclusive(false);
        ui->rBtn_A->setChecked(false);
        ui->rBtn_A->setAutoExclusive(true);
        ui->rBtn_B->setAutoExclusive(false);
        ui->rBtn_B->setChecked(false);
        ui->rBtn_B->setAutoExclusive(true);
        ui->rBtn_C->setAutoExclusive(false);
        ui->rBtn_C->setChecked(false);
        ui->rBtn_C->setAutoExclusive(true);
        ui->rBtn_D->setAutoExclusive(false);
        ui->rBtn_D->setChecked(false);
        ui->rBtn_D->setAutoExclusive(true);
    } else if(index == 1) {
        ui->lineEdit_blank1->clear();
        ui->lineEdit_blank2->clear();
        ui->lineEdit_blank3->clear();
        ui->lineEdit_blank4->clear();
        ui->lineEdit_blank5->clear();
    } else if(index == 2) {

    }
}

void ExamDialog::saveResponse()
{
    if(m_lastPointer[0] == 0) {
        if(ui->rBtn_A->isChecked()) {
            m_responseList[m_lastPointer[1]] = "A";
        } else if(ui->rBtn_B->isChecked()) {
            m_responseList[m_lastPointer[1]] = "B";
        } else if(ui->rBtn_C->isChecked()) {
            m_responseList[m_lastPointer[1]] = "C";
        } else if(ui->rBtn_D->isChecked()) {
            m_responseList[m_lastPointer[1]] = "D";
        } else {
            m_responseList[m_lastPointer[1]] = "";
        }
    } else if(m_lastPointer[0] == 1) {
        int offset = m_choiceNum;
        m_responseList[offset + m_lastPointer[1]] = "";
        if(!ui->lineEdit_blank1->text().isEmpty()) {
            m_responseList[offset + m_lastPointer[1]] = ui->lineEdit_blank1->text();
        }
        if(!ui->lineEdit_blank2->text().isEmpty()) {
            m_responseList[offset + m_lastPointer[1]] += "|" + ui->lineEdit_blank2->text();
        }
        if(!ui->lineEdit_blank3->text().isEmpty()) {
            m_responseList[offset + m_lastPointer[1]] += "|" + ui->lineEdit_blank3->text();
        }
        if(!ui->lineEdit_blank4->text().isEmpty()) {
            m_responseList[offset + m_lastPointer[1]] += "|" + ui->lineEdit_blank4->text();
        }
        if(!ui->lineEdit_blank5->text().isEmpty()) {
            m_responseList[offset + m_lastPointer[1]] += "|" + ui->lineEdit_blank5->text();
        }
    } else if(m_lastPointer[0] == 2) {
        int offset = m_choiceNum + m_completionNum;
        m_responseList[offset + m_lastPointer[1]] = "";
        // 获取文件内容
        QFile file(QCoreApplication::applicationDirPath() +  QString("./UserFolder/test_%1.c").arg(m_lastPointer[1] + 1));
        if(file.exists()) {
            file.open(QIODevice::ReadWrite | QIODevice::Text);
            QString content = file.readAll();
            file.close();
            m_responseList[offset + m_lastPointer[1]] = content;
        }
    }
}

void ExamDialog::changeProblem()
{
    // 在切换界面内容前将上一题的回答保存下来
    saveResponse();

    // 当前item变成蓝色
    if(m_curPointer[0] == 0) {
        int row = m_curPointer[1] / 5;
        int col = m_curPointer[1] % 5;
        ui->tableWidget_choice->cellWidget(row, col)->setStyleSheet("background-color:#23aaf2;");
    } else if(m_curPointer[0] == 1) {
        int row = m_curPointer[1] / 5;
        int col = m_curPointer[1] % 5;
        ui->tableWidget_completion->cellWidget(row, col)->setStyleSheet("background-color:#23aaf2;");
    } else if(m_curPointer[0] == 2) {
        int row = m_curPointer[1] / 5;
        int col = m_curPointer[1] % 5;
        ui->tableWidget_program->cellWidget(row, col)->setStyleSheet("background-color:#23aaf2;");
    }

    // 上一item如果已答题，则变为绿色，否则变回灰色
    if(m_lastPointer[0] == 0) {
        int row = m_lastPointer[1] / 5;
        int col = m_lastPointer[1] % 5;
        if(m_responseList[m_lastPointer[1]] != "") {
            ui->tableWidget_choice->cellWidget(row, col)->setStyleSheet("background-color:#4acf5a;");
        } else {
            ui->tableWidget_choice->cellWidget(row, col)->setStyleSheet("background-color:#f0f0f0;");
        }
    } else if(m_lastPointer[0] == 1) {
        int row = m_lastPointer[1] / 5;
        int col = m_lastPointer[1] % 5;
        if(m_responseList[m_choiceNum + m_lastPointer[1]] != "") {
            ui->tableWidget_completion->cellWidget(row, col)->setStyleSheet("background-color:#4acf5a;");
        } else {
            ui->tableWidget_completion->cellWidget(row, col)->setStyleSheet("background-color:#f0f0f0;");
        }
    } else if(m_lastPointer[0] == 2) {
        int row = m_lastPointer[1] / 5;
        int col = m_lastPointer[1] % 5;
        if(m_responseList[m_choiceNum + m_completionNum + m_lastPointer[1]] != "") {
            ui->tableWidget_program->cellWidget(row, col)->setStyleSheet("background-color:#4acf5a;");
        } else {
            ui->tableWidget_program->cellWidget(row, col)->setStyleSheet("background-color:#f0f0f0;");
        }
    }

    // 切换界面内容
    if(m_curPointer[0] == 0) {
        ui->stackedWidget->setCurrentIndex(0);
        ui->label_choiceScore->setText(QString("(共%1题，每题%2分)").arg(m_choiceNum).arg(m_choiceScore));
        clearResponseArea(0);

        std::string content = m_choiceProblem.at(m_curPointer[1]).content;
        json content_json = json::parse(content);

        QString describe = QString::fromStdString(content_json.at("Describe"));
        QString options = QString::fromStdString(content_json.at("Option"));
        QStringList optionList = options.split("|");
        QString optionA = optionList.at(0);
        QString optionB = optionList.at(1);
        QString optionC = optionList.at(2);
        QString optionD = optionList.at(3);
        ui->label_choiceDescribe->setText(QString("%1. ").arg(m_choiceProblem.at(m_curPointer[1]).order) + describe);
        ui->label_optionA->setText(optionA);
        ui->label_optionB->setText(optionB);
        ui->label_optionC->setText(optionC);
        ui->label_optionD->setText(optionD);

    } else if(m_curPointer[0] == 1) {
        ui->stackedWidget->setCurrentIndex(1);
        ui->label_completionScore->setText(QString("(共%1题，每题%2分)").arg(m_completionNum).arg(m_completionScore));
        clearResponseArea(1);

        std::string content = m_completionProblem.at(m_curPointer[1]).content;
        json content_json = json::parse(content);

        QString describe = QString::fromStdString(content_json.at("Describe"));
        ui->label_completionDescribe->setText(QString("%1. ").arg(m_completionProblem.at(m_curPointer[1]).order) + describe);
    } else if(m_curPointer[0] == 2) {
        ui->stackedWidget->setCurrentIndex(2);
        ui->label_programScore->setText(QString("(共%1题，每题%2分)").arg(m_programNum).arg(m_programScore));

        std::string content = m_programProblem.at(m_curPointer[1]).content;
        json content_json = json::parse(content);

        QString describe = QString::fromStdString(content_json.at("Describe"));
        std::string templateCode = content_json.at("Template");
        ui->label_programDescribe->setText(QString("%1. ").arg(m_programProblem.at(m_curPointer[1]).order) + describe);
    }
}

void ExamDialog::submitExamResponse()
{
    std::vector<std::string> resList;
    for(int i = 0; i < m_responseList.size(); i++) {
        resList.push_back(m_responseList.at(i).toStdString());
    }
    int point = judgeExam();

    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "SubmitResponse";
    send_json["Category"] = CATEGORY::EXAM;
    send_json["UID"] = m_uid;
    send_json["PID"] = m_pid;
    send_json["Point"] = point;
    json content_json;
    content_json["ResponseList"] = resList;
    std::string str = content_json.dump();
    send_json["Content"] = str;

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
        emit sig_examSubmitted();
        this->close();
    }
}

int ExamDialog::judgeExam()
{
    int point = 0;

    for(int i = 0; i < m_responseList.size(); i++) {
        if(i < m_choiceNum) {
            std::string content = m_choiceProblem.at(i).content;
            json content_json = json::parse(content);
            QString answer = QString::fromStdString(content_json.at("Answer"));
            if(answer == m_responseList.at(i)) {
                point += m_choiceScore;
            }
        } else if(i >= m_choiceNum && i < (m_choiceNum + m_completionNum)) {
            std::string content = m_completionProblem.at(i - m_choiceNum).content;
            json content_json = json::parse(content);
            QString answer = QString::fromStdString(content_json.at("Answer"));
            if(answer == m_responseList.at(i)) {
                point += m_completionScore;
            }
        } else if(i >= (m_choiceNum + m_completionNum)) {
            int pid = m_programProblem.at(i - m_choiceNum - m_completionNum).pid;
            QString code_content = m_responseList.at(i);
            point += verifyProgram(pid, m_programScore, code_content.toStdString());
        }
    }

    return point;
}

int ExamDialog::verifyProgram(int pid, int score, std::string code_content)
{
    // 提取用户输入的信息，封装为JSON格式，并转为二进制字节流
    json send_json;
    send_json["Event"] = "SubmitResponse";
    send_json["Category"] = CATEGORY::PROGRAM;
    send_json["UID"] = -1;
    send_json["PID"] = pid;
    send_json["Score"] = score;
    send_json["Content"] = code_content;

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
        QString compileFlag = QString::fromStdString(recv_json.at("Compile"));
        QString testFlag = QString::fromStdString(recv_json.at("Test"));

        if(compileFlag == "Pass" && testFlag == "Pass") {
            return score;
        } else {
            return 0;
        }
    }

    return 0;
}
