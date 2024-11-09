#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <QObject>
#include <QFile>

#include "tcpclient.h"

class FileTransfer : public QObject
{
    Q_OBJECT
public:
    explicit FileTransfer(TCPClient* tcpClient, QString fileName, QObject *parent = nullptr);

public:
    void startTransfer();

private slots:
    void updateClientProgress(qint64 numBytes);

private:
    TCPClient* m_tcpClient;
    QFile* m_localFile;
    qint64 m_totalBytes;
    qint64 m_bytesWritten;
    qint64 m_bytesToWrite;
    qint64 m_payloadSize;
    QString m_fileName;
    QByteArray m_outBlock;
};

#endif // FILETRANSFER_H
