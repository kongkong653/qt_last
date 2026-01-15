# 构建说明

## Qt Creator 构建步骤

1. **打开项目**
   - 启动Qt Creator
   - 选择"文件" -> "打开文件或项目"
   - 选择 `chat.pro` 文件

2. **配置构建套件**
   - 确保已安装Qt 5.12或更高版本
   - 选择正确的构建套件（Kit）
   - 检查编译器设置（MSVC、MinGW或GCC）

3. **构建项目**
   - 点击左下角的"构建"按钮（锤子图标）
   - 或使用快捷键 `Ctrl+B`
   - 等待编译完成

4. **运行程序**
   - 点击"运行"按钮（绿色三角形）
   - 或使用快捷键 `F5`

## 命令行构建（可选）

### Windows (MSVC)
```bash
qmake chat.pro
nmake
```

### Windows (MinGW)
```bash
qmake chat.pro
mingw32-make
```

### Linux/Mac
```bash
qmake chat.pro
make
```

## 常见问题

### 1. 找不到Qt模块
- 确保Qt安装正确
- 检查 `.pro` 文件中的模块名称
- 在Qt Creator中检查Kit配置

### 2. UI文件未生成
- 确保已安装Qt Designer
- 运行 `uic` 命令生成UI头文件：
  ```bash
  uic logindialog.ui -o ui_logindialog.h
  uic mainwindow.ui -o ui_mainwindow.h
  uic chatwindow.ui -o ui_chatwindow.h
  ```

### 3. 数据库错误
- 确保SQLite驱动已安装
- 检查应用数据目录权限
- Windows下可能需要管理员权限

### 4. 网络连接失败
- 这是正常的，程序支持离线模式
- 如需测试网络功能，需要先启动服务器端程序
- 可以在登录界面修改服务器地址和端口

## 调试建议

1. **查看控制台输出**：程序使用 `qDebug()` 输出调试信息
2. **检查数据库文件**：数据库保存在应用数据目录
3. **网络调试**：使用Wireshark等工具监控TCP连接
