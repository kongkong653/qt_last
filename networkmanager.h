#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QByteArray>
#include <QDataStream>

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    enum MessageType {
        MSG_LOGIN = 1,
        MSG_REGISTER = 2,
        MSG_TEXT = 3,
        MSG_HEARTBEAT = 4,
        MSG_ACK = 5,
        MSG_GET_CONTACTS = 6,
        MSG_ADD_CONTACT = 7,
        MSG_GROUP_MESSAGE = 8
    };

    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    bool connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    // 发送消息
    void sendLogin(int userId, const QString& username);
    void sendRegister(const QString& username, const QString& password, const QString& nickname);
    void sendTextMessage(int fromUserId, int toUserId, const QString& content, bool isGroup = false);
    void sendHeartbeat();
    void sendGetContacts(int userId);
    void sendAddContact(int userId, int contactId, const QString& contactName);

signals:
    void connected();
    void disconnected();
    void loginSuccess(int userId, const QString& username);
    void loginFailed(const QString& reason);
    void registerSuccess(int userId);
    void registerFailed(const QString& reason);
    void messageReceived(int fromUserId, int toUserId, const QString& content, const QDateTime& timestamp, bool isGroup);
    void contactsReceived(const QJsonArray& contacts);
    void errorOccurred(const QString& error);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);
    void reconnect();

private:
    void parseMessage(const QByteArray& data);
    QByteArray createMessage(MessageType type, const QJsonObject& data);
    
    QTcpSocket* m_socket;
    QString m_host;
    quint16 m_port;
    QTimer* m_reconnectTimer;
    QTimer* m_heartbeatTimer;
    bool m_autoReconnect;
    QByteArray m_buffer;
    int m_currentUserId;
};

#endif // NETWORKMANAGER_H
