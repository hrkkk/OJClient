#include "tcpclient.h"

TCPClient::TCPClient(QWidget *parent)
    : QWidget{parent}
{   
    serverIP = "192.168.148.24";
    serverPort = 8888;

    tcpSocket = new QTcpSocket(this);
    restBytes = 0;

    connect(tcpSocket, &QTcpSocket::connected, this, [=]() {
//        QString msg = "hello world";
//        tcpSocket->write(msg.toUtf8().data());
    });

    connect(tcpSocket, &QTcpSocket::readyRead, this, [=]() {
        recvMsg = tcpSocket->readAll();
//        qDebug() << "Recv Size: " << recvMsg.size();
        if(recvMsg.size() == 4) {
            restBytes = recvMsg.toInt();
            totalMsg.clear();
//            qDebug() << "Recv Data Size: " << restBytes;
        } else if(recvMsg.size() == 1024 && restBytes != 0) {
            if(restBytes > 1024) {
                totalMsg += recvMsg;
                restBytes -= 1024;
            } else if(restBytes <= 1024) {
                totalMsg += recvMsg.left(restBytes);
                restBytes = 0;
//                qDebug() << "recive from server: " << totalMsg;
                emit s_recvData(totalMsg);
            }
        }
    });

    // 与服务器建立连接
    tcpSocket->connectToHost(serverIP, serverPort);
    if(!tcpSocket->waitForConnected(3000)) {
        qDebug() << tcpSocket->errorString();
    }
}

TCPClient::~TCPClient()
{
    if(tcpSocket == nullptr)
        return;

    // 断开与服务器的连接
    tcpSocket->disconnectFromHost();
    // 关闭通信套接字
    tcpSocket->close();
}

int TCPClient::sendToServer(QString msg)
{
    if(tcpSocket->state() == QAbstractSocket::ConnectedState) {
        tcpSocket->write(msg.toUtf8().data());
        return 1;
    } else {
        return -1;
    }
}

int TCPClient::sendToServer(QByteArray msg)
{
    if(tcpSocket->state() == QAbstractSocket::ConnectedState) {
        tcpSocket->write(msg);
//        qDebug() << "send to server: " << msg;
        return 1;
    } else {
        return -1;
    }
}
