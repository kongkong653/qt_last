#include "messagemodel.h"
#include <QDebug>

MessageModel::MessageModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_currentUserId(0)
{
}

int MessageModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return m_messages.size();
}

QVariant MessageModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_messages.size()) {
        return QVariant();
    }
    
    const MessageInfo& message = m_messages.at(index.row());
    
    switch (role) {
    case MessageIdRole:
        return message.messageId;
    case FromUserIdRole:
        return message.fromUserId;
    case ToUserIdRole:
        return message.toUserId;
    case ContentRole:
        return message.content;
    case TimestampRole:
        return message.timestamp.toString("yyyy-MM-dd hh:mm:ss");
    case MessageTypeRole:
        return message.messageType;
    case IsGroupRole:
        return message.isGroup;
    case IsOwnMessageRole:
        return message.fromUserId == m_currentUserId;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> MessageModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[MessageIdRole] = "messageId";
    roles[FromUserIdRole] = "fromUserId";
    roles[ToUserIdRole] = "toUserId";
    roles[ContentRole] = "content";
    roles[TimestampRole] = "timestamp";
    roles[MessageTypeRole] = "messageType";
    roles[IsGroupRole] = "isGroup";
    roles[IsOwnMessageRole] = "isOwnMessage";
    return roles;
}

void MessageModel::loadMessages(int userId, int contactId, bool isGroup)
{
    beginResetModel();
    m_currentUserId = userId;
    m_messages = DatabaseManager::instance().getMessages(userId, contactId, 100, isGroup);
    endResetModel();
}

void MessageModel::addMessage(const MessageInfo& message, int currentUserId)
{
    m_currentUserId = currentUserId;
    
    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(message);
    endInsertRows();
}

void MessageModel::clear()
{
    beginResetModel();
    m_messages.clear();
    endResetModel();
}
