#include "logindialog.h"
#include "ui_logindialog.h"
#include <QDebug>
#include <QTimer>

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
    , m_networkManager(nullptr)
    , m_userId(0)
    , m_isProcessing(false)
{
    ui->setupUi(this);
    
    // 设置中文文本（UI文件中使用英文以避免uic工具问题）
    setWindowTitle("登录/注册");
    ui->titleLabel->setText("即时通讯系统");
    ui->serverLabel->setText("服务器:");
    ui->portLabel->setText("端口:");
    ui->statusLabel->setText("未连接");
    ui->tabWidget->setTabText(0, "登录");
    ui->tabWidget->setTabText(1, "注册");
    ui->label->setText("用户名");
    ui->label_2->setText("密码");
    ui->label_3->setText("用户名");
    ui->label_4->setText("密码");
    ui->label_5->setText("昵称");
    ui->loginUsernameEdit->setPlaceholderText("请输入用户名");
    ui->loginPasswordEdit->setPlaceholderText("请输入密码");
    ui->registerUsernameEdit->setPlaceholderText("请输入用户名");
    ui->registerPasswordEdit->setPlaceholderText("请输入密码(至少6位)");
    ui->registerNicknameEdit->setPlaceholderText("请输入昵称");
    ui->loginButton->setText("登 录");
    ui->registerButton->setText("注 册");
    
    setFixedSize(450, 520);
    
    m_networkManager = new NetworkManager(this);
    connect(m_networkManager, &NetworkManager::loginSuccess, this, &LoginDialog::onLoginSuccess);
    connect(m_networkManager, &NetworkManager::loginFailed, this, &LoginDialog::onLoginFailed);
    connect(m_networkManager, &NetworkManager::registerSuccess, this, &LoginDialog::onRegisterSuccess);
    connect(m_networkManager, &NetworkManager::registerFailed, this, &LoginDialog::onRegisterFailed);
    connect(m_networkManager, &NetworkManager::connected, this, &LoginDialog::onNetworkConnected);
    connect(m_networkManager, &NetworkManager::disconnected, this, &LoginDialog::onNetworkDisconnected);
    connect(m_networkManager, &NetworkManager::errorOccurred, this, &LoginDialog::onNetworkError);
    
    connect(ui->loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(ui->registerButton, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    
    // 延迟尝试连接服务器，确保UI完全初始化
    QTimer::singleShot(100, this, [this]() {
        connectToServer();
    });
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::setupUI()
{
    // UI已在.ui文件中定义
}

bool LoginDialog::connectToServer()
{
    QString host = ui->serverEdit->text().isEmpty() ? "127.0.0.1" : ui->serverEdit->text();
    quint16 port = ui->portEdit->text().isEmpty() ? 8888 : ui->portEdit->text().toUShort();
    
    return m_networkManager->connectToServer(host, port);
}

void LoginDialog::onLoginClicked()
{
    if (m_isProcessing) {
        return;
    }
    
    QString username = ui->loginUsernameEdit->text().trimmed();
    QString password = ui->loginPasswordEdit->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入用户名和密码");
        return;
    }
    
    m_isProcessing = true;
    ui->loginButton->setEnabled(false);
    
    // 优先尝试网络登录，如果网络未连接则使用本地登录
    if (m_networkManager->isConnected()) {
        tryNetworkLogin(username, password);
    } else {
        // 尝试连接服务器
        if (connectToServer()) {
            // 连接成功，等待连接完成后再登录
            // 使用定时器等待连接完成
            QTimer::singleShot(500, this, [this, username, password]() {
                if (m_networkManager->isConnected()) {
                    tryNetworkLogin(username, password);
                } else {
                    tryLocalLogin(username, password);
                }
            });
        } else {
            // 连接失败，使用本地登录
            tryLocalLogin(username, password);
        }
    }
}

void LoginDialog::tryNetworkLogin(const QString& username, const QString& password)
{
    // 网络登录需要先验证本地数据库（因为需要userId）
    // 如果本地数据库中有该用户，使用本地userId进行网络登录
    int userId;
    QString nickname;
    if (DatabaseManager::instance().loginUser(username, password, userId, nickname)) {
        m_userId = userId;
        m_username = username;
        m_nickname = nickname;
        // 发送网络登录请求
        m_networkManager->sendLogin(userId, username);
        // 等待网络响应，onLoginSuccess会处理accept
    } else {
        // 本地数据库中没有该用户，提示错误
        m_isProcessing = false;
        ui->loginButton->setEnabled(true);
        QMessageBox::warning(this, "错误", "用户名或密码错误");
    }
}

void LoginDialog::tryLocalLogin(const QString& username, const QString& password)
{
    int userId;
    QString nickname;
    if (DatabaseManager::instance().loginUser(username, password, userId, nickname)) {
        m_userId = userId;
        m_username = username;
        m_nickname = nickname;
        m_isProcessing = false;
        ui->loginButton->setEnabled(true);
        accept();
    } else {
        m_isProcessing = false;
        ui->loginButton->setEnabled(true);
        QMessageBox::warning(this, "错误", "用户名或密码错误");
    }
}

void LoginDialog::onRegisterClicked()
{
    if (m_isProcessing) {
        return;
    }
    
    QString username = ui->registerUsernameEdit->text().trimmed();
    QString password = ui->registerPasswordEdit->text();
    QString nickname = ui->registerNicknameEdit->text().trimmed();
    
    if (username.isEmpty() || password.isEmpty() || nickname.isEmpty()) {
        QMessageBox::warning(this, "提示", "请填写所有字段");
        return;
    }
    
    if (password.length() < 6) {
        QMessageBox::warning(this, "提示", "密码长度至少6位");
        return;
    }
    
    m_isProcessing = true;
    ui->registerButton->setEnabled(false);
    
    // 优先尝试网络注册，如果网络未连接则使用本地注册
    if (m_networkManager->isConnected()) {
        tryNetworkRegister(username, password, nickname);
    } else {
        // 尝试连接服务器
        if (connectToServer()) {
            // 连接成功，等待连接完成后再注册
            QTimer::singleShot(500, this, [this, username, password, nickname]() {
                if (m_networkManager->isConnected()) {
                    tryNetworkRegister(username, password, nickname);
                } else {
                    tryLocalRegister(username, password, nickname);
                }
            });
        } else {
            // 连接失败，使用本地注册
            tryLocalRegister(username, password, nickname);
        }
    }
}

void LoginDialog::tryNetworkRegister(const QString& username, const QString& password, const QString& nickname)
{
    // 先检查本地是否已存在该用户
    int tempUserId;
    QString tempNickname;
    if (DatabaseManager::instance().loginUser(username, password, tempUserId, tempNickname)) {
        // 用户已存在
        m_isProcessing = false;
        ui->registerButton->setEnabled(true);
        QMessageBox::warning(this, "错误", "用户名已存在");
        return;
    }
    
    // 发送网络注册请求
    m_networkManager->sendRegister(username, password, nickname);
    // 等待网络响应，onRegisterSuccess会处理后续操作
}

void LoginDialog::tryLocalRegister(const QString& username, const QString& password, const QString& nickname)
{
    if (DatabaseManager::instance().registerUser(username, password, nickname)) {
        // 注册成功后自动登录
        int userId;
        QString dbNickname;
        if (DatabaseManager::instance().loginUser(username, password, userId, dbNickname)) {
            m_userId = userId;
            m_username = username;
            m_nickname = dbNickname;
            m_isProcessing = false;
            ui->registerButton->setEnabled(true);
            QMessageBox::information(this, "成功", "注册成功！");
            accept();
        } else {
            m_isProcessing = false;
            ui->registerButton->setEnabled(true);
            QMessageBox::warning(this, "错误", "注册成功但登录失败，请重新登录");
        }
    } else {
        m_isProcessing = false;
        ui->registerButton->setEnabled(true);
        QMessageBox::warning(this, "错误", "注册失败，用户名可能已存在");
    }
}

void LoginDialog::onLoginSuccess(int userId, const QString& username)
{
    Q_UNUSED(userId)
    Q_UNUSED(username)
    // 网络登录成功，用户信息已经在tryNetworkLogin中设置
    m_isProcessing = false;
    ui->loginButton->setEnabled(true);
    accept();
}

void LoginDialog::onLoginFailed(const QString& reason)
{
    // 网络登录失败，尝试本地登录
    QString username = ui->loginUsernameEdit->text().trimmed();
    QString password = ui->loginPasswordEdit->text();
    
    qDebug() << "网络登录失败:" << reason << "，尝试本地登录";
    tryLocalLogin(username, password);
}

void LoginDialog::onRegisterSuccess(int userId)
{
    // 网络注册成功，需要在本地数据库中也注册并登录
    QString username = ui->registerUsernameEdit->text().trimmed();
    QString password = ui->registerPasswordEdit->text();
    QString nickname = ui->registerNicknameEdit->text().trimmed();
    
    // 确保本地数据库也有该用户（如果网络注册成功但本地没有）
    int localUserId;
    QString localNickname;
    if (!DatabaseManager::instance().loginUser(username, password, localUserId, localNickname)) {
        // 本地数据库中没有，添加进去
        DatabaseManager::instance().registerUser(username, password, nickname);
        DatabaseManager::instance().loginUser(username, password, localUserId, localNickname);
    }
    
    m_userId = localUserId > 0 ? localUserId : userId;
    m_username = username;
    m_nickname = localNickname.isEmpty() ? nickname : localNickname;
    
    m_isProcessing = false;
    ui->registerButton->setEnabled(true);
    QMessageBox::information(this, "成功", "注册成功！");
    accept();
}

void LoginDialog::onRegisterFailed(const QString& reason)
{
    // 网络注册失败，尝试本地注册
    QString username = ui->registerUsernameEdit->text().trimmed();
    QString password = ui->registerPasswordEdit->text();
    QString nickname = ui->registerNicknameEdit->text().trimmed();
    
    qDebug() << "网络注册失败:" << reason << "，尝试本地注册";
    tryLocalRegister(username, password, nickname);
}

void LoginDialog::onNetworkConnected()
{
    ui->statusLabel->setText("已连接");
    ui->statusLabel->setStyleSheet("QLabel { color: #2e7d32; font-weight: bold; padding: 5px; background: #e8f5e9; border-radius: 3px; }");
    qDebug() << "网络连接成功";
}

void LoginDialog::onNetworkDisconnected()
{
    ui->statusLabel->setText("未连接");
    ui->statusLabel->setStyleSheet("QLabel { color: #d32f2f; font-weight: bold; padding: 5px; background: #ffebee; border-radius: 3px; }");
    qDebug() << "网络连接断开";
}

void LoginDialog::onNetworkError(const QString& error)
{
    ui->statusLabel->setText("连接错误: " + error);
    ui->statusLabel->setStyleSheet("QLabel { color: #d32f2f; font-weight: bold; padding: 5px; background: #ffebee; border-radius: 3px; }");
    qDebug() << "网络错误:" << error;
}
