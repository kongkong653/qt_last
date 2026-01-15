#ifndef MESSAGEMODEL_H
#define MESSAGEMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "databasemanager.h"

class MessageModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum MessageRoles {
        MessageIdRole = Qt::UserRole + 1,
        FromUserIdRole,
        ToUserIdRole,
        ContentRole,
        TimestampRole,
        MessageTypeRole,
        IsGroupRole,
        IsOwnMessageRole
    };

    explicit MessageModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void loadMessages(int userId, int contactId, bool isGroup = false);
    void addMessage(const MessageInfo& message, int currentUserId);
    void clear();

private:
    QList<MessageInfo> m_messages;
    int m_currentUserId;
};

#endif // MESSAGEMODEL_H
