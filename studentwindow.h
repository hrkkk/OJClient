#ifndef STUDENTWINDOW_H
#define STUDENTWINDOW_H

#include <QDialog>
#include <QButtonGroup>
#include <QTableWidget>
#include <QEventLoop>
#include <QComboBox>
#include <QVBoxLayout>

#include "define.h"
#include "tcpclient.h"
#include "answerdialog.h"
#include "examdialog.h"
#include "json.hpp"
using json = nlohmann::json;

namespace Ui {
class StudentWindow;
}

class StudentWindow : public QDialog
{
    Q_OBJECT

public:
    explicit StudentWindow(TCPClient* tcpClient, int _uid, QWidget *parent = nullptr);
    ~StudentWindow();

private:
    void initTable(QTableWidget* tableWidget);
    void fillTable(QTableWidget* tableWidget, int category);
    void getUserInfo(int uid);
    void requestProblemList(int category);
    void submitProgram(int uid, int pid);
    void getResponseRecord();
    void fillExamList();
    void clearResponseRecord();
    void updateProblemTable(int category);

private:
    Ui::StudentWindow *ui;
    QEventLoop* eventLoop;
    QByteArray recvMsg;
    TCPClient* m_tcpClient;
    AnswerDialog* answerDialog = nullptr;
    ExamDialog* examDialog = nullptr;
    QVBoxLayout* v_layout;
    QVBoxLayout* v_layout_res;

    int m_uid;
    struct Problem_Struct {
        bool isResponse;
        int pid;
        int score;
        int point;
        std::string response;
        std::string content;
    };

    QList<Problem_Struct> problemList_choice;
    QList<Problem_Struct> problemList_completion;
    QList<Problem_Struct> problemList_program;
    QList<Problem_Struct> problemList_exam;
};

#endif // STUDENTWINDOW_H
