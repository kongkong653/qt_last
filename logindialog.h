#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include "databasemanager.h"
#include "networkmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class LoginDialog; }
QT_END_NAMESPACE

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* parent = nullptr);
    ~LoginDialog();

    int getUserId() const { return m_userId; }
    QString getUsername() const { return m_username; }

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onLoginSuccess(int userId, const QString& username);
    void onLoginFailed(const QString& reason);
    void onRegisterSuccess(int userId);
    void onRegisterFailed(const QString& reason);
    void onNetworkConnected();
    void onNetworkDisconnected();
    void onNetworkError(const QString& error);

private:
    Ui::LoginDialog* ui;
    NetworkManager* m_networkManager;
    int m_userId;
    QString m_username;
    QString m_nickname;
    bool m_isProcessing;  // 防止重复操作
    
    void setupUI();
    bool connectToServer();
    void tryNetworkLogin(const QString& username, const QString& password);
    void tryLocalLogin(const QString& username, const QString& password);
    void tryNetworkRegister(const QString& username, const QString& password, const QString& nickname);
    void tryLocalRegister(const QString& username, const QString& password, const QString& nickname);
};

#endif // LOGINDIALOG_H
