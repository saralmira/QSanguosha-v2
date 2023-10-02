#include "updateservice.h"
#include "engine.h"
#include "protocol.h"
#include "nativesocket.h"

using namespace QSanProtocol;

UpdateService::UpdateService(QObject *parent) : QObject(parent), Port(0)
{
    
}

UpdateService::~UpdateService()
{
    if (!pProcess) {
        pProcess->kill();
        pProcess = NULL;
    }
}

void UpdateService::processUpdateRequest()
{
    if (pProcess) {
        return;
    }

    QStringList args;
    args << "-s" << QString::number(Port);

    pProcess = new QProcess(this);
    pProcess->start(QDir::currentPath() + "/Updater.exe", args);
}

void UpdateService::updateClient(const QString &ip, int port)
{
    QStringList args;
    args << "-c" << ip << QString::number(port);

    pProcess = new QProcess(this);
    pProcess->start(QDir::currentPath() + "/Updater.exe", args);
}

void UpdateService::RequestFileList()
{

}

