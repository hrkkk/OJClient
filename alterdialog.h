#ifndef ALTERDIALOG_H
#define ALTERDIALOG_H

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
class AlterDialog;
}

class AlterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AlterDialog(TCPClient* tcpClient, int pid, int category, QString content, int score, QWidget *parent = nullptr);
    ~AlterDialog();

signals:
    void sig_altered(CATEGORY category);

private:
    void setContentEditable(int index, bool flag);
    void showProblem(int category, QString content, int score);
    void deleteProblem();
    void updateProblem();
    void addCaseWidget(QString input, QString output);

private:
    Ui::AlterDialog *ui;

    TCPClient* m_tcpClient;
    QEventLoop* eventLoop;
    QByteArray recvMsg;

    int m_pid;
    int m_category;
};

#endif // ALTERDIALOG_H
