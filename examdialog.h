#ifndef EXAMDIALOG_H
#define EXAMDIALOG_H

#include <QDialog>
#include <QTableWidgetItem>
#include <QEventLoop>
#include <QTimer>
#include <QMessageBox>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>

#include "define.h"
#include "tcpclient.h"
#include "json.hpp"
using json = nlohmann::json;

namespace Ui {
class ExamDialog;
}

class ExamDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExamDialog(TCPClient* tcpClient, int uid, int pid, std::string content, QWidget *parent = nullptr);
    ~ExamDialog();

signals:
    void sig_examSubmitted();

private:
    void getProblemContent(const std::vector<int>& choiceList, const std::vector<int>& completionList, const std::vector<int>& programList);
    void fillTable();
    void changeProblem();
    void saveResponse();
    void clearResponseArea(int index);
    void submitExamResponse();
    void writeToFile();
    int judgeExam();
    int verifyProgram(int pid, int score, std::string code_content);

private:
    Ui::ExamDialog *ui;

    TCPClient* m_tcpClient;
    QEventLoop* eventLoop;
    QByteArray recvMsg;
    QTimer* timer;

    int m_uid;
    int m_pid;
    int m_choiceNum;
    int m_choiceScore;
    int m_completionNum;
    int m_completionScore;
    int m_programNum;
    int m_programScore;
    int m_time;
    int m_curPointer[2];
    int m_lastPointer[2];

    struct Problem_Struct {
        int order;
        int pid;
        std::string content;
    };

    QList<Problem_Struct> m_choiceProblem;
    QList<Problem_Struct> m_completionProblem;
    QList<Problem_Struct> m_programProblem;
    QList<QString> m_responseList;
};

#endif // EXAMDIALOG_H
