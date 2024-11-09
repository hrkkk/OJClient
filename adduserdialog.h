#ifndef ADDUSERDIALOG_H
#define ADDUSERDIALOG_H

#include <QDialog>

namespace Ui {
class AddUserDialog;
}

class AddUserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddUserDialog(QWidget *parent = nullptr);
    ~AddUserDialog();

signals:
    void s_addNewUser(QString account, QString password);

private:
    Ui::AddUserDialog *ui;
};

#endif // ADDUSERDIALOG_H
