#include "mainwindow.h"
#include "logindialog.h"
#include "databasemanager.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用样式
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // 初始化数据库
    DatabaseManager::instance().init();
    
    // 显示登录对话框
    LoginDialog loginDialog;
    if (loginDialog.exec() == QDialog::Accepted) {
        MainWindow window;
        window.setCurrentUserId(loginDialog.getUserId());
        window.setCurrentUsername(loginDialog.getUsername());
        window.show();
        return app.exec();
    }
    
    return 0;
}
