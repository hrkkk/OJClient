#ifndef ANSWERDIALOG_H
#define ANSWERDIALOG_H

#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QDesktopServices>
#include <QEventLoop>
#include <QMessageBox>

#include "define.h"
#include "tcpclient.h"
#include "json.hpp"
using json = nlohmann::json;

namespace Ui {
class AnswerDialog;
}

class AnswerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AnswerDialog(TCPClient* tcpClient, int uid, int pid, int num, int category, int score, QString content, QWidget *parent = nullptr);
    ~AnswerDialog();

signals:
    void sig_submitted(CATEGORY category);

private:
    void submitResponse(int category);
    void submitProgram();

private:
    Ui::AnswerDialog *ui;
    TCPClient* m_tcpClient;
    QEventLoop* eventLoop;
    QByteArray recvMsg;

    int m_uid;
    int m_pid;
    int m_category;
    int m_score;
};

#endif // ANSWERDIALOG_H
