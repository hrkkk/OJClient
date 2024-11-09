#ifndef CLIENTEVENT_H
#define CLIENTEVENT_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>

class ClientEvent : public QObject
{
    Q_OBJECT
public:
    explicit ClientEvent(QObject *parent = nullptr);

public:
    QByteArray loginEvent(QString account, QString password);
    QByteArray registerEvent(QString account, QString password);
    QByteArray problemAddEvent(QString question, QString optionA, QString optionB,
                               QString optionC, QString optionD, int socre, QString answer, QString analysis); // 增加选择题
    QByteArray problemAddEvent(QString question, int blank, QString answer,
                               int socre, QString analysis); // 增加填空题
    QByteArray problemAddEvent(QString question); // 增加编程题
    QByteArray problemDeleteEvent(int identity, QString method);
    QByteArray problemAnswerEvent(int identity, QString method);
    QByteArray userEvent(int identity, QString method);
};

#endif // CLIENTEVENT_H
