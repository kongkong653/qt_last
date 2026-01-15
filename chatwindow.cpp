#include "chatwindow.h"
#include "ui_chatwindow.h"
#include <QScrollBar>
#include <QDateTime>
#include <QDebug>
#include <QMessageBox>
#include <QtGlobal>

ChatWindow::ChatWindow(int contactId, const QString& contactName, bool isGroup, 
                       int currentUserId, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ChatWindow)
    , m_contactId(contactId)
    , m_contactName(contactName)
    , m_isGroup(isGroup)
    , m_currentUserId(currentUserId)
    , m_networkManager(nullptr)
{
    setupUI();
    loadHistoryMessages();
}

ChatWindow::~ChatWindow()
{
    delete ui;
}

void ChatWindow::setupUI()
{
    // UI已在.ui文件中定义，这里只需要获取控件指针并设置标题
    ui->setupUi(this);
    
    // 获取UI中的控件
    m_titleLabel = ui->titleLabel;
    m_messageList = ui->messageList;
    m_inputEdit = ui->inputEdit;
    m_sendButton = ui->sendButton;
    
    // 设置标题
    QString title = m_contactName;
    if (m_isGroup) {
        title = "[群聊] " + title;
    }
    m_titleLabel->setText(title);
    
    // 连接信号槽
    connect(m_sendButton, &QPushButton::clicked, this, &ChatWindow::onSendClicked);
    connect(m_inputEdit, &QLineEdit::textChanged, this, &ChatWindow::onTextChanged);
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &ChatWindow::onSendClicked);
    
    // 消息模型（用于数据管理，但显示使用QTextEdit）
    m_messageModel = new MessageModel(this);
    m_messageModel->loadMessages(m_currentUserId, m_contactId, m_isGroup);
}

void ChatWindow::setNetworkManager(NetworkManager* networkManager)
{
    m_networkManager = networkManager;
}

void ChatWindow::loadHistoryMessages()
{
    QList<MessageInfo> messages = DatabaseManager::instance().getMessages(
        m_currentUserId, m_contactId, 50, m_isGroup);
    
    m_messageList->clear();
    for (const MessageInfo& msg : qAsConst(messages)) {
        bool isOwn = (msg.fromUserId == m_currentUserId);
        QString formattedMsg = formatMessage(msg, isOwn);
        m_messageList->append(formattedMsg);
    }
    
    // 滚动到底部
    QScrollBar* scrollBar = m_messageList->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void ChatWindow::addMessage(const MessageInfo& message)
{
    // 保存到数据库
    DatabaseManager::instance().saveMessage(message);
    
    // 添加到模型
    m_messageModel->addMessage(message, m_currentUserId);
    
    // 显示消息
    bool isOwn = (message.fromUserId == m_currentUserId);
    QString formattedMsg = formatMessage(message, isOwn);
    m_messageList->append(formattedMsg);
    
    // 滚动到底部
    QScrollBar* scrollBar = m_messageList->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

QString ChatWindow::formatMessage(const MessageInfo& message, bool isOwn)
{
    QString timeStr = message.timestamp.toString("hh:mm:ss");
    QString align = isOwn ? "right" : "left";
    QString bgColor = isOwn ? "#95ec69" : "#ffffff";
    QString senderName = QString::number(message.fromUserId);
    
    if (m_isGroup && !isOwn) {
        senderName = "用户" + senderName;
    } else if (isOwn) {
        senderName = "我";
    }
    
    QString html = QString(
        "<div style='text-align: %1; margin: 5px;'>"
        "<div style='display: inline-block; background-color: %2; padding: 8px 12px; "
        "border-radius: 5px; max-width: 70%%;'>"
        "<div style='font-size: 10px; color: #666; margin-bottom: 3px;'>%3</div>"
        "<div style='word-wrap: break-word;'>%4</div>"
        "</div>"
        "</div>"
    ).arg(align).arg(bgColor).arg(senderName + " " + timeStr).arg(message.content.toHtmlEscaped());
    
    return html;
}

void ChatWindow::onSendClicked()
{
    QString content = m_inputEdit->text().trimmed();
    if (content.isEmpty()) {
        return;
    }
    
    if (!m_networkManager || !m_networkManager->isConnected()) {
        QMessageBox::warning(this, "提示", "未连接到服务器，消息将仅保存到本地");
    }
    
    // 创建消息
    MessageInfo message;
    message.fromUserId = m_currentUserId;
    message.toUserId = m_contactId;
    message.content = content;
    message.timestamp = QDateTime::currentDateTime();
    message.messageType = 0;
    message.isGroup = m_isGroup;
    
    // 保存到数据库
    DatabaseManager::instance().saveMessage(message);
    
    // 发送到服务器
    if (m_networkManager && m_networkManager->isConnected()) {
        m_networkManager->sendTextMessage(m_currentUserId, m_contactId, content, m_isGroup);
    }
    
    // 显示消息
    addMessage(message);
    
    // 清空输入框
    m_inputEdit->clear();
}

void ChatWindow::onTextChanged()
{
    m_sendButton->setEnabled(!m_inputEdit->text().trimmed().isEmpty());
}
