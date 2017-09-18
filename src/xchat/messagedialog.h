//*****************************************************************************
//*****************************************************************************

#ifndef MESSAGESDIALOG_H
#define MESSAGESDIALOG_H

#include "messagesmodel.h"
#include "message_delegate.h"
#include "users_model.h"
#include "user_delegate.h"
#include "message_db.h"
#include "stored_pub_keys_db.h"
#include "../qt/editaddressdialog.h"

#include <QDialog>
#include <QTimer>
#include <QMenu>

#include <string>

//*****************************************************************************
//*****************************************************************************
namespace Ui {
    class MessagesDialog;
}

class WalletModel;

class WalletView;

//*****************************************************************************
//*****************************************************************************
class MessagesDialog : public QDialog {
Q_OBJECT

public:
    explicit MessagesDialog(QWidget *parent = 0);

    virtual ~MessagesDialog();

public:
    void setWalletModel(WalletModel *model);

    void setFromAddress(const QString &address);

    void setToAddress(const QString &address);

signals:

    void newIncomingMessage(const QString &title, const QString &text);

public slots:

    void showForAddress(const QString &address);

    void incomingMessage(Message &message);

private slots:

    void on_pasteButton_SM_clicked();

    void on_pasteButtonTo_SM_clicked();

    void on_pasteButtonPublicKey_SM_clicked();

    void on_addressBookButton_SM_clicked();

    void on_addressBookButtonTo_SM_clicked();

    void on_sendButton_SM_clicked();

    void on_clearButton_SM_clicked();

    void on_addresses_SM_clicked(const QModelIndex &index);

    void requestUndeliveredMessages();

    void onReadTimer();

    void addressContextMenu(QPoint point);

private:
    std::vector<std::string> getLocalAddresses() const;

    bool loadMessages(const QString &address, std::vector<Message> &result);

    void saveMessages(const QString &address, const std::vector<Message> &messages);

    void clearMessages(const QString &address);

    void pushToUndelivered(const Message &message);

    bool checkAddress(const std::string &address) const;

    bool getKeyForAddress(const std::string &address, CKey &key) const;

    bool signMessage(CKey &key, const std::string &message,
                     std::vector<unsigned char> &signature) const;

    bool resendUndelivered(const std::vector<std::string> &addresses);

private:
    Ui::MessagesDialog *ui;

    ChatDb &chatDb_;
    StoredPubKeysDb &keysDb_;

    MessagesModel messagesModel_;
    MessageDelegate messageDelegate_;

    UsersModel usersModel;
    UserDelegate userDelegate_;

    WalletModel *walletModel;

    QTimer readTimer_;

    QMenu userContextMenu_;
    QAction *addToAddressBookAction_;
    QAction *deleteAction_;
};

#endif // MESSAGEDIALOG_H
