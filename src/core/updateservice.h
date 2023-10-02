#ifndef UPDATESERVICE_H
#define UPDATESERVICE_H

#include <QObject>
#include <QProcess>

class UpdateService : public QObject
{
    Q_OBJECT
public:
    explicit UpdateService(QObject *parent = 0);
    ~UpdateService();

    void processUpdateRequest();
    void updateClient(const QString &ip, int port);

    ushort Port;

signals:

public slots:
    void RequestFileList();

private:
    QProcess *pProcess;
};

#endif // UPDATESERVICE_H
