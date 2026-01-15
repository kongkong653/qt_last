#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QLabel>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_currentUserId(0)
{
    ui->setupUi(this);
    setWindowTitle("即时通讯系统");
    setMinimumSize(1000, 700);
    
    setupUI();
    setupNetwork();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUI()
{
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setStyleSheet(
        "QSplitter::handle {"
        "    background: #ddd;"
        "    width: 3px;"
        "}"
        "QSplitter::handle:hover {"
        "    background: #0078d4;"
        "}"
    );
    
    // 左侧联系人列表
    m_contactList = new ContactListWidget(this);
    m_contactList->setMinimumWidth(250);
    m_contactList->setMaximumWidth(300);
    splitter->addWidget(m_contactList);
    
    connect(m_contactList, &ContactListWidget::contactSelected, 
            this, &MainWindow::onContactSelected);
    
    // 右侧聊天窗口标签页
    m_chatTabs = new QTabWidget(this);
    m_chatTabs->setTabsClosable(true);
    m_chatTabs->setMovable(true);
    m_chatTabs->setStyleSheet(
        "QTabWidget::pane {"
        "    border: 1px solid #ddd;"
        "    background: white;"
        "    border-radius: 3px;"
        "}"
        "QTabBar::tab {"
        "    background: #f0f0f0;"
        "    color: #333;"
        "    padding: 8px 15px;"
        "    margin-right: 2px;"
        "    border-top-left-radius: 3px;"
        "    border-top-right-radius: 3px;"
        "}"
        "QTabBar::tab:selected {"
        "    background: white;"
        "    color: #0078d4;"
        "    font-weight: bold;"
        "}"
        "QTabBar::tab:hover {"
        "    background: #e8e8e8;"
        "}"
    );
    splitter->addWidget(m_chatTabs);
    
    connect(m_chatTabs, &QTabWidget::tabCloseRequested, [this](int index) {
        QWidget* widget = m_chatTabs->widget(index);
        if (widget) {
            ChatWindow* chatWindow = qobject_cast<ChatWindow*>(widget);
            if (chatWindow) {
                int contactId = chatWindow->getContactId();
                m_chatWindows.remove(contactId);
            }
            m_chatTabs->removeTab(index);
            widget->deleteLater();
        }
    });
    
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    
    setCentralWidget(splitter);
    
    // 状态栏
    statusBar()->showMessage("就绪");
    
    // 菜单栏
    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui->actionAbout, &QAction::triggered, [this]() {
        QMessageBox::about(this, "关于", 
            "即时通讯系统 v1.0\n\n"
            "基于Qt开发的多人即时通讯客户端\n"
            "支持单聊、群聊、联系人管理等功能");
    });
}

void MainWindow::setupNetwork()
{
    m_networkManager = new NetworkManager(this);
    connect(m_networkManager, &NetworkManager::messageReceived, 
            this, &MainWindow::onMessageReceived);
    connect(m_networkManager, &NetworkManager::connected, 
            this, &MainWindow::onNetworkConnected);
    connect(m_networkManager, &NetworkManager::disconnected, 
            this, &MainWindow::onNetworkDisconnected);
    connect(m_networkManager, &NetworkManager::errorOccurred, 
            this, &MainWindow::onNetworkError);
    
    // 尝试连接服务器
    m_networkManager->connectToServer("127.0.0.1", 8888);
}

void MainWindow::onContactSelected(int contactId, const QString& contactName, bool isGroup)
{
    ChatWindow* chatWindow = getOrCreateChatWindow(contactId, contactName, isGroup);
    
    // 如果窗口已经在标签页中，切换到该标签
    int tabIndex = m_chatTabs->indexOf(chatWindow);
    if (tabIndex >= 0) {
        m_chatTabs->setCurrentIndex(tabIndex);
    } else {
        // 添加到标签页
        tabIndex = m_chatTabs->addTab(chatWindow, contactName);
        m_chatTabs->setCurrentIndex(tabIndex);
    }
}

ChatWindow* MainWindow::getOrCreateChatWindow(int contactId, const QString& contactName, bool isGroup)
{
    if (m_chatWindows.contains(contactId)) {
        return m_chatWindows[contactId];
    }
    
    ChatWindow* chatWindow = new ChatWindow(contactId, contactName, isGroup, m_currentUserId, this);
    chatWindow->setNetworkManager(m_networkManager);
    m_chatWindows[contactId] = chatWindow;
    
    return chatWindow;
}

void MainWindow::onMessageReceived(int fromUserId, int toUserId, const QString& content, 
                                    const QDateTime& timestamp, bool isGroup)
{
    // 确定消息应该显示在哪个聊天窗口
    int targetContactId = (fromUserId == m_currentUserId) ? toUserId : fromUserId;
    
    ChatWindow* chatWindow = getOrCreateChatWindow(targetContactId, 
                                                   QString::number(targetContactId), 
                                                   isGroup);
    
    // 如果窗口不在标签页中，添加到标签页
    int tabIndex = m_chatTabs->indexOf(chatWindow);
    if (tabIndex < 0) {
        QString tabName = QString::number(targetContactId);
        if (isGroup) {
            tabName = "[群]" + tabName;
        }
        tabIndex = m_chatTabs->addTab(chatWindow, tabName);
    }
    
    // 显示消息
    MessageInfo msg;
    msg.fromUserId = fromUserId;
    msg.toUserId = toUserId;
    msg.content = content;
    msg.timestamp = timestamp;
    msg.isGroup = isGroup;
    msg.messageType = 0;
    
    chatWindow->addMessage(msg);
    
    // 高亮标签页（如果有未读消息）
    m_chatTabs->setTabText(tabIndex, m_chatTabs->tabText(tabIndex) + " *");
}

void MainWindow::onNetworkConnected()
{
    statusBar()->showMessage("已连接到服务器", 3000);
    if (m_currentUserId > 0) {
        m_networkManager->sendLogin(m_currentUserId, m_currentUsername);
        m_networkManager->sendGetContacts(m_currentUserId);
    }
    
    // 加载联系人列表
    if (m_currentUserId > 0) {
        m_contactList->loadContacts(m_currentUserId);
    }
}

void MainWindow::onNetworkDisconnected()
{
    statusBar()->showMessage("与服务器断开连接", 3000);
}

void MainWindow::onNetworkError(const QString& error)
{
    statusBar()->showMessage("网络错误: " + error, 5000);
}
