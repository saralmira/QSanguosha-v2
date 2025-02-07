﻿#include "nativesocket.h"
#include "settings.h"
#include "src/pch.h"
#include <QLibrary>

QLibrary *util_win = NULL;
typedef int (*utilwin_IsPortOccupied)(ulong);

NativeServerSocket::NativeServerSocket()
{
    server = new QTcpServer(this);
    daemon = NULL;
    connect(server, SIGNAL(newConnection()), this, SLOT(processNewConnection()));
}

bool NativeServerSocket::listen()
{
    return server->listen(QHostAddress::Any, Config.ServerPort);
}

void NativeServerSocket::daemonize()
{
    daemon = new QUdpSocket(this);
    daemon->bind(Config.ServerPort, QUdpSocket::ShareAddress);
    connect(daemon, SIGNAL(readyRead()), this, SLOT(processNewDatagram()));
}

bool NativeServerSocket::isPortOccupied(ushort port)
{
    bool res = false;
    if (!util_win) {
        util_win = new QLibrary("utils_win.dll");
    }
    if (!util_win->isLoaded() && !util_win->load())
        return res;

    utilwin_IsPortOccupied qfunc = (utilwin_IsPortOccupied)util_win->resolve("utilwin_IsPortOccupied");
    if(qfunc)
    {
        res = qfunc(port);
    }
    return res;
}

void NativeServerSocket::processNewDatagram()
{
    while (daemon->hasPendingDatagrams()) {
        QHostAddress from;
        char ask_str[256];

        daemon->readDatagram(ask_str, sizeof(ask_str), &from);

        QByteArray data = Config.ServerName.toUtf8();
        daemon->writeDatagram(data, from, Config.DetectorPort);
        daemon->flush();
    }
}

void NativeServerSocket::processNewConnection()
{
    QTcpSocket *socket = server->nextPendingConnection();
    NativeClientSocket *connection = new NativeClientSocket(socket);
    emit new_connection(connection);
}

// ---------------------------------

NativeClientSocket::NativeClientSocket()
    : socket(new QTcpSocket(this))
{
    init();
}

NativeClientSocket::NativeClientSocket(QTcpSocket *socket)
    : socket(socket)
{
    socket->setParent(this);
    init();
    timerSignup.setSingleShot(true);
    connect(&timerSignup,SIGNAL(timeout()),this,SLOT(disconnectFromHost()));
}

void NativeClientSocket::init()
{
    connect(socket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(getMessage()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
        this, SLOT(raiseError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(connected()), this, SIGNAL(connected()));
}

void NativeClientSocket::connectToHost()
{
    QString address = "127.0.0.1";
    ushort port = 9527u;

    if (Config.HostAddress.contains(QChar(':'))) {
        QStringList texts = Config.HostAddress.split(QChar(':'));
        address = texts.value(0);
        port = texts.value(1).toUShort();
    } else {
        address = Config.HostAddress;
        if (address == "127.0.0.1")
            port = Config.value("ServerPort", "9527").toString().toUShort();
    }

    socket->connectToHost(address, port);
}

void NativeClientSocket::getMessage()
{
    while (socket->canReadLine()) {
        buffer_t msg;
        socket->readLine(msg, sizeof(msg));
#ifndef QT_NO_DEBUG
        printf("RX: %s", msg);
#endif
        emit message_got(msg);
    }
}

void NativeClientSocket::disconnectFromHost()
{
    socket->disconnectFromHost();
}

void NativeClientSocket::send(const QString &message)
{
    socket->write(message.toLatin1());
    if (!message.endsWith("\n"))
        socket->write("\n");
#ifndef QT_NO_DEBUG
    printf("TX: %s\n", message.toLatin1().constData());
#endif
    socket->flush();
}

bool NativeClientSocket::isConnected() const
{
    return socket->state() == QTcpSocket::ConnectedState;
}

QString NativeClientSocket::peerName() const
{
    QString peer_name = socket->peerName();
    if (peer_name.isEmpty())
        peer_name = QString("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort());

    return peer_name;
}

QString NativeClientSocket::peerAddress() const
{
    return socket->peerAddress().toString();
}

void NativeClientSocket::raiseError(QAbstractSocket::SocketError socket_error)
{
    // translate error message
    QString reason;
    switch (socket_error) {
    case QAbstractSocket::ConnectionRefusedError:
        reason = tr("Connection was refused or timeout"); break;
    case QAbstractSocket::RemoteHostClosedError:
        reason = tr("Remote host close this connection"); break;
    case QAbstractSocket::HostNotFoundError:
        reason = tr("Host not found"); break;
    case QAbstractSocket::SocketAccessError:
        reason = tr("Socket access error"); break;
    case QAbstractSocket::NetworkError:
        return; // this error is ignored ...
    default: reason = tr("Unknow error"); break;
    }

    emit error_message(tr("Connection failed, error code = %1\n reason:\n %2").arg(socket_error).arg(reason));
}

