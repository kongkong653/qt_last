#include "contactlistwidget.h"
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QtGlobal>

ContactListWidget::ContactListWidget(QWidget* parent)
    : QWidget(parent)
    , m_currentUserId(0)
{
    setupUI();
}

void ContactListWidget::setupUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);
    
    // 搜索框
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("🔍 搜索联系人...");
    m_searchEdit->setStyleSheet(
        "QLineEdit {"
        "    padding: 8px;"
        "    border: 2px solid #ddd;"
        "    border-radius: 15px;"
        "    font-size: 13px;"
        "    background: white;"
        "}"
        "QLineEdit:focus {"
        "    border-color: #0078d4;"
        "}"
    );
    layout->addWidget(m_searchEdit);
    
    // 联系人树
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabel("联系人");
    m_treeWidget->setRootIsDecorated(true);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeWidget->setStyleSheet(
        "QTreeWidget {"
        "    border: 1px solid #ddd;"
        "    border-radius: 5px;"
        "    background: white;"
        "    font-size: 13px;"
        "}"
        "QTreeWidget::item {"
        "    padding: 5px;"
        "    border-bottom: 1px solid #f0f0f0;"
        "}"
        "QTreeWidget::item:hover {"
        "    background: #e3f2fd;"
        "}"
        "QTreeWidget::item:selected {"
        "    background: #0078d4;"
        "    color: white;"
        "}"
        "QTreeWidget::branch:has-siblings:!adjoins-item {"
        "    border-image: none;"
        "}"
    );
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &ContactListWidget::onContactDoubleClicked);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &ContactListWidget::onContextMenuRequested);
    layout->addWidget(m_treeWidget);
    
    // 按钮栏
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(10);
    m_addButton = new QPushButton("➕ 添加联系人", this);
    m_refreshButton = new QPushButton("🔄 刷新", this);
    m_addButton->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "        stop:0 #0078d4, stop:1 #005a9e);"
        "    color: white;"
        "    border: none;"
        "    padding: 8px;"
        "    border-radius: 4px;"
        "    font-size: 13px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "        stop:0 #1084e4, stop:1 #006ab8);"
        "}"
        "QPushButton:pressed {"
        "    background: #005a9e;"
        "}"
    );
    m_refreshButton->setStyleSheet(
        "QPushButton {"
        "    background: #f5f5f5;"
        "    color: #333;"
        "    border: 1px solid #ddd;"
        "    padding: 8px;"
        "    border-radius: 4px;"
        "    font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "    background: #e8e8e8;"
        "}"
    );
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_refreshButton);
    layout->addLayout(buttonLayout);
    
    connect(m_addButton, &QPushButton::clicked, this, &ContactListWidget::onAddContactClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &ContactListWidget::onRefreshClicked);
    
    // 设置整体样式
    setStyleSheet(
        "QWidget {"
        "    background: #f5f5f5;"
        "}"
    );
}

void ContactListWidget::loadContacts(int userId)
{
    m_currentUserId = userId;
    populateContacts();
}

void ContactListWidget::populateContacts()
{
    m_treeWidget->clear();
    
    QList<ContactInfo> contacts = DatabaseManager::instance().getContacts(m_currentUserId);
    
    QHash<QString, QTreeWidgetItem*> groupItems;
    
    for (const ContactInfo& contact : qAsConst(contacts)) {
        QTreeWidgetItem* groupItem = findGroupItem(contact.groupName);
        if (!groupItem) {
            groupItem = new QTreeWidgetItem(m_treeWidget);
            groupItem->setText(0, contact.groupName);
            groupItem->setExpanded(true);
            groupItems[contact.groupName] = groupItem;
        }
        
        QTreeWidgetItem* contactItem = new QTreeWidgetItem(groupItem);
        QString displayName = contact.contactName;
        if (contact.isGroup) {
            displayName = "[群] " + displayName;
        }
        contactItem->setText(0, displayName);
        contactItem->setData(0, Qt::UserRole, contact.contactId);
        contactItem->setData(0, Qt::UserRole + 1, contact.isGroup);
        contactItem->setData(0, Qt::UserRole + 2, contact.contactName);
    }
}

QTreeWidgetItem* ContactListWidget::findGroupItem(const QString& groupName)
{
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_treeWidget->topLevelItem(i);
        if (item->text(0) == groupName) {
            return item;
        }
    }
    return nullptr;
}

QTreeWidgetItem* ContactListWidget::findContactItem(int contactId)
{
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* groupItem = m_treeWidget->topLevelItem(i);
        for (int j = 0; j < groupItem->childCount(); ++j) {
            QTreeWidgetItem* contactItem = groupItem->child(j);
            if (contactItem->data(0, Qt::UserRole).toInt() == contactId) {
                return contactItem;
            }
        }
    }
    return nullptr;
}

void ContactListWidget::addContact(const ContactInfo& contact)
{
    QTreeWidgetItem* groupItem = findGroupItem(contact.groupName);
    if (!groupItem) {
        groupItem = new QTreeWidgetItem(m_treeWidget);
        groupItem->setText(0, contact.groupName);
        groupItem->setExpanded(true);
    }
    
    QTreeWidgetItem* contactItem = new QTreeWidgetItem(groupItem);
    QString displayName = contact.contactName;
    if (contact.isGroup) {
        displayName = "[群] " + displayName;
    }
    contactItem->setText(0, displayName);
    contactItem->setData(0, Qt::UserRole, contact.contactId);
    contactItem->setData(0, Qt::UserRole + 1, contact.isGroup);
    contactItem->setData(0, Qt::UserRole + 2, contact.contactName);
}

void ContactListWidget::updateContactLastMessage(int userId, int contactId, const QDateTime& time)
{
    Q_UNUSED(userId)
    Q_UNUSED(time)
    // 可以在这里更新联系人项的显示，比如显示最后消息时间
    QTreeWidgetItem* item = findContactItem(contactId);
    if (item) {
        // 可以添加时间显示
    }
}

void ContactListWidget::onContactDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column)
    
    if (!item || item->parent() == nullptr) {
        return; // 点击的是分组，不是联系人
    }
    
    int contactId = item->data(0, Qt::UserRole).toInt();
    bool isGroup = item->data(0, Qt::UserRole + 1).toBool();
    QString contactName = item->data(0, Qt::UserRole + 2).toString();
    
    emit contactSelected(contactId, contactName, isGroup);
}

void ContactListWidget::onAddContactClicked()
{
    bool ok;
    QString contactIdStr = QInputDialog::getText(this, "添加联系人", "请输入联系人ID:", QLineEdit::Normal, "", &ok);
    
    if (ok && !contactIdStr.isEmpty()) {
        int contactId = contactIdStr.toInt();
        QString contactName = QInputDialog::getText(this, "添加联系人", "请输入联系人名称:", QLineEdit::Normal, "", &ok);
        
        if (ok && !contactName.isEmpty()) {
            QString groupName = QInputDialog::getText(this, "添加联系人", "请输入分组名称:", QLineEdit::Normal, "默认分组", &ok);
            if (ok) {
                if (DatabaseManager::instance().addContact(m_currentUserId, contactId, contactName, groupName)) {
                    ContactInfo info;
                    info.contactId = contactId;
                    info.contactName = contactName;
                    info.groupName = groupName.isEmpty() ? "默认分组" : groupName;
                    info.userId = m_currentUserId;
                    info.isGroup = false;
                    addContact(info);
                    QMessageBox::information(this, "成功", "联系人添加成功");
                } else {
                    QMessageBox::warning(this, "错误", "添加联系人失败");
                }
            }
        }
    }
}

void ContactListWidget::onRefreshClicked()
{
    populateContacts();
}

void ContactListWidget::onContextMenuRequested(const QPoint& pos)
{
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    if (!item || item->parent() == nullptr) {
        return; // 只对联系人项显示菜单
    }
    
    QMenu menu(this);
    QAction* deleteAction = menu.addAction("删除联系人");
    connect(deleteAction, &QAction::triggered, this, &ContactListWidget::onDeleteContact);
    
    menu.exec(m_treeWidget->mapToGlobal(pos));
}

void ContactListWidget::onDeleteContact()
{
    QTreeWidgetItem* item = m_treeWidget->currentItem();
    if (!item || item->parent() == nullptr) {
        return;
    }
    
    int contactId = item->data(0, Qt::UserRole).toInt();
    
    if (QMessageBox::question(this, "确认", "确定要删除这个联系人吗？") == QMessageBox::Yes) {
        if (DatabaseManager::instance().removeContact(m_currentUserId, contactId)) {
            delete item;
            QMessageBox::information(this, "成功", "联系人已删除");
        } else {
            QMessageBox::warning(this, "错误", "删除联系人失败");
        }
    }
}
