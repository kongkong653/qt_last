#include "databasemanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir;
    if (!dir.exists(dataPath)) {
        dir.mkpath(dataPath);
    }
    m_dbPath = dataPath + "/chat.db";
}

DatabaseManager::~DatabaseManager()
{
    close();
}

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::init()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(m_dbPath);
    
    if (!m_db.open()) {
        qDebug() << "无法打开数据库:" << m_db.lastError().text();
        return false;
    }
    
    return createTables();
}

void DatabaseManager::close()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_db);
    
    // 用户表
    query.exec("CREATE TABLE IF NOT EXISTS users ("
               "user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "username TEXT UNIQUE NOT NULL,"
               "password TEXT NOT NULL,"
               "nickname TEXT NOT NULL,"
               "avatar TEXT,"
               "is_online INTEGER DEFAULT 0,"
               "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)");
    
    // 联系人表
    query.exec("CREATE TABLE IF NOT EXISTS contacts ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "user_id INTEGER NOT NULL,"
               "contact_id INTEGER NOT NULL,"
               "contact_name TEXT NOT NULL,"
               "group_name TEXT DEFAULT '默认分组',"
               "is_group INTEGER DEFAULT 0,"
               "last_message_time DATETIME,"
               "FOREIGN KEY(user_id) REFERENCES users(user_id))");
    
    // 消息表
    query.exec("CREATE TABLE IF NOT EXISTS messages ("
               "message_id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "from_user_id INTEGER NOT NULL,"
               "to_user_id INTEGER NOT NULL,"
               "content TEXT NOT NULL,"
               "message_type INTEGER DEFAULT 0,"
               "is_group INTEGER DEFAULT 0,"
               "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
               "FOREIGN KEY(from_user_id) REFERENCES users(user_id))");
    
    // 分组表
    query.exec("CREATE TABLE IF NOT EXISTS groups ("
               "group_id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "group_name TEXT NOT NULL,"
               "user_id INTEGER NOT NULL,"
               "FOREIGN KEY(user_id) REFERENCES users(user_id))");
    
    return query.lastError().type() == QSqlError::NoError;
}

bool DatabaseManager::registerUser(const QString& username, const QString& password, const QString& nickname)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO users (username, password, nickname) VALUES (?, ?, ?)");
    query.addBindValue(username);
    query.addBindValue(password); // 实际应用中应该加密
    query.addBindValue(nickname);
    
    return query.exec();
}

bool DatabaseManager::loginUser(const QString& username, const QString& password, int& userId, QString& nickname)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT user_id, nickname FROM users WHERE username = ? AND password = ?");
    query.addBindValue(username);
    query.addBindValue(password);
    
    if (query.exec() && query.next()) {
        userId = query.value(0).toInt();
        nickname = query.value(1).toString();
        updateUserStatus(userId, true);
        return true;
    }
    
    return false;
}

bool DatabaseManager::updateUserStatus(int userId, bool isOnline)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE users SET is_online = ? WHERE user_id = ?");
    query.addBindValue(isOnline ? 1 : 0);
    query.addBindValue(userId);
    return query.exec();
}

UserInfo DatabaseManager::getUserInfo(int userId)
{
    UserInfo info;
    QSqlQuery query(m_db);
    query.prepare("SELECT user_id, username, nickname, avatar, is_online FROM users WHERE user_id = ?");
    query.addBindValue(userId);
    
    if (query.exec() && query.next()) {
        info.userId = query.value(0).toInt();
        info.username = query.value(1).toString();
        info.nickname = query.value(2).toString();
        info.avatar = query.value(3).toString();
        info.isOnline = query.value(4).toBool();
    }
    
    return info;
}

bool DatabaseManager::addContact(int userId, int contactId, const QString& contactName, const QString& groupName, bool isGroup)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO contacts (user_id, contact_id, contact_name, group_name, is_group) "
                  "VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(userId);
    query.addBindValue(contactId);
    query.addBindValue(contactName);
    query.addBindValue(groupName);
    query.addBindValue(isGroup ? 1 : 0);
    
    return query.exec();
}

bool DatabaseManager::removeContact(int userId, int contactId)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM contacts WHERE user_id = ? AND contact_id = ?");
    query.addBindValue(userId);
    query.addBindValue(contactId);
    return query.exec();
}

QList<ContactInfo> DatabaseManager::getContacts(int userId)
{
    QList<ContactInfo> contacts;
    QSqlQuery query(m_db);
    query.prepare("SELECT contact_id, contact_name, group_name, is_group, last_message_time "
                  "FROM contacts WHERE user_id = ? ORDER BY group_name, last_message_time DESC");
    query.addBindValue(userId);
    
    while (query.next()) {
        ContactInfo info;
        info.contactId = query.value(0).toInt();
        info.contactName = query.value(1).toString();
        info.groupName = query.value(2).toString();
        info.userId = userId;
        info.isGroup = query.value(3).toBool();
        info.lastMessageTime = query.value(4).toDateTime();
        contacts.append(info);
    }
    
    return contacts;
}

bool DatabaseManager::updateContactLastMessage(int userId, int contactId, const QDateTime& time)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE contacts SET last_message_time = ? WHERE user_id = ? AND contact_id = ?");
    query.addBindValue(time);
    query.addBindValue(userId);
    query.addBindValue(contactId);
    return query.exec();
}

bool DatabaseManager::saveMessage(const MessageInfo& message)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO messages (from_user_id, to_user_id, content, message_type, is_group, timestamp) "
                  "VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(message.fromUserId);
    query.addBindValue(message.toUserId);
    query.addBindValue(message.content);
    query.addBindValue(message.messageType);
    query.addBindValue(message.isGroup ? 1 : 0);
    query.addBindValue(message.timestamp);
    
    bool success = query.exec();
    
    if (success) {
        updateContactLastMessage(message.fromUserId, message.toUserId, message.timestamp);
        if (!message.isGroup) {
            updateContactLastMessage(message.toUserId, message.fromUserId, message.timestamp);
        }
    }
    
    return success;
}

QList<MessageInfo> DatabaseManager::getMessages(int userId, int contactId, int limit, bool isGroup)
{
    QList<MessageInfo> messages;
    QSqlQuery query(m_db);
    
    if (isGroup) {
        query.prepare("SELECT message_id, from_user_id, to_user_id, content, message_type, timestamp "
                      "FROM messages WHERE to_user_id = ? AND is_group = 1 "
                      "ORDER BY timestamp DESC LIMIT ?");
        query.addBindValue(contactId);
    } else {
        query.prepare("SELECT message_id, from_user_id, to_user_id, content, message_type, timestamp "
                      "FROM messages WHERE ((from_user_id = ? AND to_user_id = ?) OR "
                      "(from_user_id = ? AND to_user_id = ?)) AND is_group = 0 "
                      "ORDER BY timestamp DESC LIMIT ?");
        query.addBindValue(userId);
        query.addBindValue(contactId);
        query.addBindValue(contactId);
        query.addBindValue(userId);
    }
    query.addBindValue(limit);
    
    if (query.exec()) {
        while (query.next()) {
            MessageInfo msg;
            msg.messageId = query.value(0).toInt();
            msg.fromUserId = query.value(1).toInt();
            msg.toUserId = query.value(2).toInt();
            msg.content = query.value(3).toString();
            msg.messageType = query.value(4).toInt();
            msg.timestamp = query.value(5).toDateTime();
            msg.isGroup = isGroup;
            messages.prepend(msg); // 反转顺序，使时间正序
        }
    }
    
    return messages;
}

QList<MessageInfo> DatabaseManager::getRecentMessages(int userId, int limit)
{
    QList<MessageInfo> messages;
    QSqlQuery query(m_db);
    query.prepare("SELECT DISTINCT m.message_id, m.from_user_id, m.to_user_id, m.content, "
                  "m.message_type, m.timestamp, m.is_group "
                  "FROM messages m "
                  "WHERE m.from_user_id = ? OR m.to_user_id = ? "
                  "ORDER BY m.timestamp DESC LIMIT ?");
    query.addBindValue(userId);
    query.addBindValue(userId);
    query.addBindValue(limit);
    
    if (query.exec()) {
        while (query.next()) {
            MessageInfo msg;
            msg.messageId = query.value(0).toInt();
            msg.fromUserId = query.value(1).toInt();
            msg.toUserId = query.value(2).toInt();
            msg.content = query.value(3).toString();
            msg.messageType = query.value(4).toInt();
            msg.timestamp = query.value(5).toDateTime();
            msg.isGroup = query.value(6).toBool();
            messages.append(msg);
        }
    }
    
    return messages;
}

bool DatabaseManager::addGroup(const QString& groupName, int userId)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO groups (group_name, user_id) VALUES (?, ?)");
    query.addBindValue(groupName);
    query.addBindValue(userId);
    return query.exec();
}

QList<QString> DatabaseManager::getGroups(int userId)
{
    QList<QString> groups;
    QSqlQuery query(m_db);
    query.prepare("SELECT DISTINCT group_name FROM contacts WHERE user_id = ? ORDER BY group_name");
    query.addBindValue(userId);
    
    while (query.next()) {
        groups.append(query.value(0).toString());
    }
    
    return groups;
}
