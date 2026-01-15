#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>
#include <QStatusBar>
#include "contactlistwidget.h"
#include "chatwindow.h"
#include "networkmanager.h"
#include "databasemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void setCurrentUserId(int userId) { m_currentUserId = userId; }
    void setCurrentUsername(const QString& username) { m_currentUsername = username; }

private slots:
    void onContactSelected(int contactId, const QString& contactName, bool isGroup);
    void onMessageReceived(int fromUserId, int toUserId, const QString& content, const QDateTime& timestamp, bool isGroup);
    void onNetworkConnected();
    void onNetworkDisconnected();
    void onNetworkError(const QString& error);

private:
    Ui::MainWindow* ui;
    ContactListWidget* m_contactList;
    QTabWidget* m_chatTabs;
    NetworkManager* m_networkManager;
    int m_currentUserId;
    QString m_currentUsername;
    
    QHash<int, ChatWindow*> m_chatWindows; // contactId -> ChatWindow
    
    void setupUI();
    void setupNetwork();
    ChatWindow* getOrCreateChatWindow(int contactId, const QString& contactName, bool isGroup);
};

#endif // MAINWINDOW_H
