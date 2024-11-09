#include "adduserdialog.h"
#include "ui_adduserdialog.h"

AddUserDialog::AddUserDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddUserDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("添加新用户");
    this->setWindowFlags(Qt::WindowCloseButtonHint);

    ui->label_warning->hide();
    ui->comboBox_identity->setEnabled(false);

    connect(ui->btn_addUser, &QPushButton::clicked, this, [=]() {
        if(ui->lineEdit_account->text().isEmpty() || ui->lineEdit_password->text().isEmpty()) {
            ui->label_warning->show();
        } else {
            ui->label_warning->hide();
            QString account = ui->lineEdit_account->text();
            QString password = ui->lineEdit_password->text();
            emit s_addNewUser(account, password);
            this->close();
        }
    });
}

AddUserDialog::~AddUserDialog()
{
    delete ui;
}
