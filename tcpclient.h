#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QWidget>
#include <QTcpSocket>

class TCPClient : public QWidget
{
    Q_OBJECT
public:
    explicit TCPClient(QWidget *parent = nullptr);
    ~TCPClient();

public:
    int sendToServer(QString msg);
    int sendToServer(QByteArray msg);

signals:
    void s_recvData(const QByteArray& msg);

private:
    QString serverIP;
    uint16_t serverPort;
    QTcpSocket* tcpSocket;
    QByteArray recvMsg;
    QByteArray totalMsg;
    int restBytes;
};

#endif // TCPCLIENT_H
