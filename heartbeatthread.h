#ifndef HEARTBEATTHREAD_H
#define HEARTBEATTHREAD_H

#include <QThread>
#include <QTimer>
#include <QObject>

class HeartbeatThread : public QThread
{
    Q_OBJECT

public:
    explicit HeartbeatThread(QObject* parent = nullptr);
    ~HeartbeatThread();

    void setInterval(int msec);
    void stop();

signals:
    void heartbeat();

protected:
    void run() override;

private:
    int m_interval;
    bool m_running;
};

#endif // HEARTBEATTHREAD_H
