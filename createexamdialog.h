#ifndef CREATEEXAMDIALOG_H
#define CREATEEXAMDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QEventLoop>
#include <QMessageBox>
#include <QDebug>
#include <QPushButton>

#include "define.h"
#include "tcpclient.h"
#include "json.hpp"
using json = nlohmann::json;

namespace Ui {
class CreateExamDialog;
}

class CreateExamDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateExamDialog(TCPClient* tcpClient,
                              const std::vector<std::pair<int, std::string>>& choiceList,
                              const std::vector<std::pair<int, std::string>>& completionList,
                              const std::vector<std::pair<int, std::string>>& programList, QWidget *parent = nullptr);
    ~CreateExamDialog();

signals:
    void sig_created();

private:
    void addCardWidget();
    void calcTotalScore();
    void uploadExam();

private:
    Ui::CreateExamDialog *ui;

    TCPClient* m_tcpClient;
    QEventLoop* eventLoop;
    QByteArray recvMsg;

    struct Problem_Struct {
        int pid;
        std::string describe;
    };

    std::vector<Problem_Struct> choiceProblem;
    std::vector<Problem_Struct> completionProblem;
    std::vector<Problem_Struct> programProblem;

    struct Choose_Struct {
        int pid;
        int orderNum;
    };

    std::vector<Choose_Struct> choiceList;
    std::vector<Choose_Struct> completionList;
    std::vector<Choose_Struct> programList;
};

#endif // CREATEEXAMDIALOG_H
