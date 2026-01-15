#ifndef CONTACTLISTWIDGET_H
#define CONTACTLISTWIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QMenu>
#include <QContextMenuEvent>
#include "databasemanager.h"

class ContactListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ContactListWidget(QWidget* parent = nullptr);
    
    void loadContacts(int userId);
    void addContact(const ContactInfo& contact);
    void updateContactLastMessage(int userId, int contactId, const QDateTime& time);

signals:
    void contactSelected(int contactId, const QString& contactName, bool isGroup);
    void addContactRequested();

private slots:
    void onContactDoubleClicked(QTreeWidgetItem* item, int column);
    void onAddContactClicked();
    void onRefreshClicked();
    void onContextMenuRequested(const QPoint& pos);
    void onDeleteContact();

private:
    QTreeWidget* m_treeWidget;
    QPushButton* m_addButton;
    QPushButton* m_refreshButton;
    QLineEdit* m_searchEdit;
    int m_currentUserId;
    
    void setupUI();
    void populateContacts();
    QTreeWidgetItem* findGroupItem(const QString& groupName);
    QTreeWidgetItem* findContactItem(int contactId);
};

#endif // CONTACTLISTWIDGET_H
