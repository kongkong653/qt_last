#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QListView>
#include <QLabel>
#include "messagemodel.h"
#include "networkmanager.h"
#include "databasemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ChatWindow; }
QT_END_NAMESPACE

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWindow(int contactId, const QString& contactName, bool isGroup, 
                       int currentUserId, QWidget* parent = nullptr);
    ~ChatWindow();
    
    void setNetworkManager(NetworkManager* networkManager);
    void addMessage(const MessageInfo& message);
    int getContactId() const { return m_contactId; }
    QString getContactName() const { return m_contactName; }

private slots:
    void onSendClicked();
    void onTextChanged();

private:
    Ui::ChatWindow* ui;
    int m_contactId;
    QString m_contactName;
    bool m_isGroup;
    int m_currentUserId;
    
    QLabel* m_titleLabel;
    QTextEdit* m_messageList;
    MessageModel* m_messageModel;
    QLineEdit* m_inputEdit;
    QPushButton* m_sendButton;
    NetworkManager* m_networkManager;
    
    void setupUI();
    void loadHistoryMessages();
    QString formatMessage(const MessageInfo& message, bool isOwn);
};

#endif // CHATWINDOW_H
