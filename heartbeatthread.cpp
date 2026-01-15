#include "heartbeatthread.h"
#include <QDebug>

HeartbeatThread::HeartbeatThread(QObject* parent)
    : QThread(parent)
    , m_interval(30000)
    , m_running(false)
{
}

HeartbeatThread::~HeartbeatThread()
{
    stop();
    wait();
}

void HeartbeatThread::setInterval(int msec)
{
    m_interval = msec;
}

void HeartbeatThread::stop()
{
    m_running = false;
}

void HeartbeatThread::run()
{
    m_running = true;
    
    while (m_running) {
        msleep(m_interval);
        if (m_running) {
            emit heartbeat();
        }
    }
}
