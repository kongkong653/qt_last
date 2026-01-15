#include "networkmanager.h"
#include <QDebug>
#include <QHostAddress>
#include <QDateTime>

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_port(8888)
    , m_autoReconnect(true)
    , m_currentUserId(0)
{
    m_socket = new QTcpSocket(this);
    
    connect(m_socket, &QTcpSocket::connected, this, &NetworkManager::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &NetworkManager::onError);
#else
    connect(m_socket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            this, &NetworkManager::onError);
#endif
    
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(3000);
    connect(m_reconnectTimer, &QTimer::timeout, this, &NetworkManager::reconnect);
    
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(30000); // 30秒心跳
    connect(m_heartbeatTimer, &QTimer::timeout, this, &NetworkManager::sendHeartbeat);
}

NetworkManager::~NetworkManager()
{
    disconnectFromServer();
}

bool NetworkManager::connectToServer(const QString& host, quint16 port)
{
    m_host = host;
    m_port = port;
    
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        return true;
    }
    
    m_socket->connectToHost(host, port);
    return m_socket->waitForConnected(3000);
}

void NetworkManager::disconnectFromServer()
{
    m_autoReconnect = false;
    m_heartbeatTimer->stop();
    m_reconnectTimer->stop();
    
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}

bool NetworkManager::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkManager::sendLogin(int userId, const QString& username)
{
    QJsonObject data;
    data["user_id"] = userId;
    data["username"] = username;
    
    QByteArray msg = createMessage(MSG_LOGIN, data);
    m_socket->write(msg);
    m_currentUserId = userId;
}

void NetworkManager::sendRegister(const QString& username, const QString& password, const QString& nickname)
{
    QJsonObject data;
    data["username"] = username;
    data["password"] = password;
    data["nickname"] = nickname;
    
    QByteArray msg = createMessage(MSG_REGISTER, data);
    m_socket->write(msg);
}

void NetworkManager::sendTextMessage(int fromUserId, int toUserId, const QString& content, bool isGroup)
{
    QJsonObject data;
    data["from_user_id"] = fromUserId;
    data["to_user_id"] = toUserId;
    data["content"] = content;
    data["is_group"] = isGroup;
    data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QByteArray msg = createMessage(isGroup ? MSG_GROUP_MESSAGE : MSG_TEXT, data);
    m_socket->write(msg);
}

void NetworkManager::sendHeartbeat()
{
    QJsonObject data;
    data["user_id"] = m_currentUserId;
    
    QByteArray msg = createMessage(MSG_HEARTBEAT, data);
    m_socket->write(msg);
}

void NetworkManager::sendGetContacts(int userId)
{
    QJsonObject data;
    data["user_id"] = userId;
    
    QByteArray msg = createMessage(MSG_GET_CONTACTS, data);
    m_socket->write(msg);
}

void NetworkManager::sendAddContact(int userId, int contactId, const QString& contactName)
{
    QJsonObject data;
    data["user_id"] = userId;
    data["contact_id"] = contactId;
    data["contact_name"] = contactName;
    
    QByteArray msg = createMessage(MSG_ADD_CONTACT, data);
    m_socket->write(msg);
}

void NetworkManager::onConnected()
{
    m_autoReconnect = true;
    m_reconnectTimer->stop();
    m_heartbeatTimer->start();
    emit connected();
    qDebug() << "已连接到服务器";
}

void NetworkManager::onDisconnected()
{
    m_heartbeatTimer->stop();
    emit disconnected();
    qDebug() << "与服务器断开连接";
    
    if (m_autoReconnect) {
        m_reconnectTimer->start();
    }
}

void NetworkManager::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    m_buffer.append(data);
    
    // 解析消息（简单协议：前4字节为消息长度）
    while (m_buffer.size() >= 4) {
        quint32 msgLength;
        QDataStream stream(&m_buffer, QIODevice::ReadOnly);
        stream >> msgLength;
        
        if (m_buffer.size() < msgLength + 4) {
            break; // 数据不完整，等待更多数据
        }
        
        QByteArray messageData = m_buffer.mid(4, msgLength);
        m_buffer.remove(0, msgLength + 4);
        
        parseMessage(messageData);
    }
}

void NetworkManager::onError(QAbstractSocket::SocketError error)
{
    QString errorString = m_socket->errorString();
    emit errorOccurred(errorString);
    qDebug() << "网络错误:" << errorString;
    
    if (m_autoReconnect && m_socket->state() != QAbstractSocket::ConnectedState) {
        m_reconnectTimer->start();
    }
}

void NetworkManager::reconnect()
{
    if (!m_autoReconnect) {
        return;
    }
    
    qDebug() << "尝试重连服务器...";
    m_socket->connectToHost(m_host, m_port);
}

void NetworkManager::parseMessage(const QByteArray& data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << error.errorString();
        return;
    }
    
    QJsonObject obj = doc.object();
    int type = obj["type"].toInt();
    QJsonObject payload = obj["data"].toObject();
    
    switch (type) {
    case MSG_LOGIN:
        if (payload["success"].toBool()) {
            emit loginSuccess(payload["user_id"].toInt(), payload["username"].toString());
        } else {
            emit loginFailed(payload["reason"].toString());
        }
        break;
        
    case MSG_REGISTER:
        if (payload["success"].toBool()) {
            emit registerSuccess(payload["user_id"].toInt());
        } else {
            emit registerFailed(payload["reason"].toString());
        }
        break;
        
    case MSG_TEXT:
    case MSG_GROUP_MESSAGE: {
        int fromUserId = payload["from_user_id"].toInt();
        int toUserId = payload["to_user_id"].toInt();
        QString content = payload["content"].toString();
        QDateTime timestamp = QDateTime::fromString(payload["timestamp"].toString(), Qt::ISODate);
        bool isGroup = payload["is_group"].toBool();
        
        emit messageReceived(fromUserId, toUserId, content, timestamp, isGroup);
        break;
    }
    
    case MSG_GET_CONTACTS:
        if (payload.contains("contacts")) {
            emit contactsReceived(payload["contacts"].toArray());
        }
        break;
        
    case MSG_ACK:
        // 确认消息，可以用于消息送达确认
        break;
        
    default:
        qDebug() << "未知消息类型:" << type;
    }
}

QByteArray NetworkManager::createMessage(MessageType type, const QJsonObject& data)
{
    QJsonObject message;
    message["type"] = type;
    message["data"] = data;
    
    QJsonDocument doc(message);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 添加消息长度前缀
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream << static_cast<quint32>(jsonData.size());
    result.append(jsonData);
    
    return result;
}
