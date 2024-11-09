#include "filetransfer.h"

FileTransfer::FileTransfer(TCPClient* tcpClient, QString fileName, QObject *parent)
    : QObject{parent}
{
    m_payloadSize = 64*1024;
    m_totalBytes = 0;
    m_bytesWritten = 0;
    m_bytesToWrite = 0;
    m_tcpClient = tcpClient;
    m_fileName = fileName;

    // 执行m_tcpClient->write(m_outBlock)会触发此函数进行文件传输
//    connect(m_tcpClient,SIGNAL(bytesWritten(qint64)),this,SLOT(updateClientProgress(qint64)));
    connect(m_tcpClient, &TCPClient::s_bytesWritten, this, &FileTransfer::updateClientProgress);
//    connect(m_tcpClient,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayError(QAbstractSocket::SocketError)));
}

void FileTransfer::startTransfer()
{
    // 打开需要传输的文件
    m_localFile = new QFile(m_fileName);
    m_localFile->open(QFile::ReadOnly);

    // 初始化总大小字节数
    m_totalBytes = m_localFile->size();

    // 定义数据流
    QDataStream sendOut(&m_outBlock,QIODevice::WriteOnly);
    sendOut.setVersion(QDataStream::Qt_5_9);

    QString strFileName = m_fileName.right(m_fileName.size()- m_fileName.lastIndexOf('/')-1);

    //数据流中写入文件总大小、文件名大小、文件名，qint64(0)占位用
    sendOut << qint64(0) << qint64(0) << strFileName;

    // 修改总大小字节数
    m_totalBytes += m_outBlock.size();

    //修改数据流中写入的文件总大小、文件名大小
    sendOut.device()->seek(0);
    sendOut << m_totalBytes << qint64(m_outBlock.size()-sizeof(qint64)*2);

    // 先调用m_tcpClient->write(m_outBlock)把m_outBlock中的文件总大小qint64、文件名大小qint64、文件名QString发出去
    m_bytesToWrite = m_totalBytes - m_tcpClient->sendToServer(m_outBlock);

    m_outBlock.resize(0);
}

void FileTransfer::updateClientProgress(qint64 numBytes)
{
    m_bytesWritten += (int)numBytes;

    if(m_bytesToWrite > 0) {
        // 一次最多发送64*1024byte的内容
        m_outBlock = m_localFile->read(qMin(m_bytesToWrite, m_payloadSize));

        m_bytesToWrite -= (int)m_tcpClient->sendToServer(m_outBlock);

        m_outBlock.resize(0);
    } else {
        if(m_localFile->isOpen()) {
            m_localFile->close();
        }
    }

    if(m_bytesWritten == m_totalBytes) {
        if(m_localFile->isOpen()) {
            m_localFile->close();
        }
//        m_tcpClient->close();
    }
}
