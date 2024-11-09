#ifndef TEACHERWINDOW_H
#define TEACHERWINDOW_H

#include <QDialog>
#include <QButtonGroup>
#include <QMessageBox>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QDesktopServices>
#include <QTableWidget>
#include <QDate>
#include <QGroupBox>
#include <QVBoxLayout>

#include "define.h"
#include "tcpclient.h"
#include "alterdialog.h"
#include "adduserdialog.h"
#include "createexamdialog.h"
#include "json.hpp"
using json = nlohmann::json;

namespace Ui {
class TeacherWindow;
}

class TeacherWindow : public QDialog
{
    Q_OBJECT

public:
    explicit TeacherWindow(TCPClient* tcpClient, int uid, QWidget *parent = nullptr);
    ~TeacherWindow();

private:
    void initChoiceUI();
    void initCompletionUI();
    void uploadProblem(int category);
    void getUserInfo(int uid);
    void RequestStudentList();
    void getProblemDetail(int category);
    void fillProblemTable(QTableWidget* tableWidget, int category);
    void fillExamList();
    void addNewUser(QString account, QString password);
    void fillStudentTable();
    void deleteUser(int uid);
    void updateProblemTable(int category);

private:
    Ui::TeacherWindow *ui;
    QEventLoop* eventLoop;
    QByteArray recvMsg;
    TCPClient* m_tcpClient;
    AlterDialog* alterDialog = nullptr;
    CreateExamDialog* createExamDialog = nullptr;
    AddUserDialog* addUserDialog = nullptr;
    QVBoxLayout* v_layout;

    QList<QString> studentList;

    struct ProblemDetail_Struct {
        int pid;
        int answerNum;
        int score;
        float averageScore;
        std::string content;
    };

    QList<ProblemDetail_Struct> problemList_choice;
    QList<ProblemDetail_Struct> problemList_completion;
    QList<ProblemDetail_Struct> problemList_program;
    QList<ProblemDetail_Struct> problemList_exam;
};

#endif // TEACHERWINDOW_H
