//*****************************************************************************
//*****************************************************************************

#include "messagedialog.h"
#include "ui_messagedialog.h"
#include "../qt/walletmodel.h"
#include "../qt/addresstablemodel.h"
#include "../qt/addressbookpage.h"
#include "../qt/editaddressdialog.h"
#include "../util/verify.h"
#include "../key.h"
#include "../base58.h"
#include "../wallet.h"
#include "../init.h"

#include <QDateTime>
#include <QMessageBox>
#include <QClipboard>
#include <QTimer>

#include <vector>

//*****************************************************************************
//*****************************************************************************
MessagesDialog::MessagesDialog(QWidget *parent)
        : QDialog(parent), ui(new Ui::MessagesDialog), chatDb_(ChatDb::instance()),
          keysDb_(StoredPubKeysDb::instance()),
          messagesModel_(0), walletModel(0) {
    ui->setupUi(this);

    ui->messages_SM->setModel(&messagesModel_);
    ui->messages_SM->setItemDelegate(&messageDelegate_);
    ui->messages_SM->setAutoScroll(true);

    ui->addresses_SM->setModel(&usersModel);
    ui->addresses_SM->setItemDelegate(&userDelegate_);

    ui->outMessage_SM->setFocus();

    std::vector<std::string> addrs;
    chatDb_.loadAddresses(addrs);
    usersModel.loadAddresses(addrs);

    addToAddressBookAction_ = new QAction(trUtf8("Add to address book"), this);
    userContextMenu_.addAction(addToAddressBookAction_);
    deleteAction_ = new QAction(trUtf8("Delete"), this);
    userContextMenu_.addAction(deleteAction_);

    VERIFY(connect(ui->addresses_SM, SIGNAL(customContextMenuRequested(QPoint)),
                   this, SLOT(addressContextMenu(QPoint))));
    VERIFY(connect(ui->outMessage_SM, SIGNAL(returnPressed()),
                   this, SLOT(on_sendButton_SM_clicked())));
    show();
}

//*****************************************************************************
//*****************************************************************************
MessagesDialog::~MessagesDialog() {
    delete ui;
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::setWalletModel(WalletModel *model) {
    walletModel = model;
    usersModel.setAddressTableModel(model->getAddressTableModel());

    // set from addr if empty
    if (ui->addressIn_SM->text().isEmpty()) {
        AddressTableModel *atm = walletModel->getAddressTableModel();
        if (atm) {
            auto count = atm->rowCount(QModelIndex());
            for (auto i = 0; i < count; ++i) {
                QModelIndex idx = atm->index(i, AddressTableModel::Address, QModelIndex());
                if (atm->data(idx, AddressTableModel::TypeRole).toString() == AddressTableModel::Receive) {
                    setFromAddress(atm->data(idx, Qt::DisplayRole).toString());
                    break;
                }
            }
        }
    }

    QTimer::singleShot(5000, this, SLOT(requestUndeliveredMessages()));
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::setFromAddress(const QString &address) {
    ui->addressIn_SM->setText(address);
    ui->outMessage_SM->setFocus();
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::setToAddress(const QString &address) {
    setWindowTitle(QString("Messages <%1>").arg(usersModel.labelForAddressWithAddress(address)));

    ui->pubKeyTo_SM->clear();

    CBitcoinAddress bitcoinAddress(address.toStdString());
    if (bitcoinAddress.IsValid()) {
        CPubKey pubKey;
        keysDb_.load(address.toStdString(), pubKey);

        if (pubKey.IsValid()) {
            ui->pubKeyTo_SM->setText(QString::fromStdString(EncodeBase58(pubKey.Raw())));
        }

        CKeyID id;
        if (bitcoinAddress.GetKeyID(id)) {
            CKey key;
            if (pwalletMain->GetKey(id, key)) {
                CPubKey cPubKey = key.GetPubKey();
                if (cPubKey.IsValid()) {
                    std::vector<unsigned char> raw = cPubKey.Raw();
                    QByteArray arr(static_cast<char *>(static_cast<void *>(&raw[0])), raw.size());
                    ui->pubKeyTo_SM->setText(QString(arr.toBase64()));
                }
            }
        }
    }

    // load messages for address
    std::vector<Message> messages;
    loadMessages(address, messages);
    messagesModel_.loadMessages(messages);

    // set remote address
    ui->addressTo_SM->setText(address);

    // set local address
    if (messages.size() > 0) {
        Message &m = messages.back();
        bool isIncoming = m.appliesToMe();

        ui->addressIn_SM->setText(QString::fromStdString(!isIncoming ? m.from : m.to));
    }

    ui->messages_SM->scrollToBottom();
    ui->outMessage_SM->setFocus();
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::showForAddress(const QString &address) {
    readTimer_.stop();

    setToAddress(address);
    // show();

    ui->messages_SM->scrollToBottom();

    readTimer_.singleShot(3000, this, SLOT(onReadTimer()));
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::incomingMessage(Message &message) {
    // before process this message try to send undelivered
    {
        std::vector<std::string> addrs;
        addrs.push_back(message.from);
        resendUndelivered(addrs);
    }

    // if message is empty - done
    if (message.isEmpty()) {
        return;
    }

    CKey key;
    if (!getKeyForAddress(message.to, key)) {
        QMessageBox::warning(this, "", QString("reseived message from <%1>,\nbut key for <%2> not found")
                .arg(QString::fromStdString(message.from),
                     QString::fromStdString(message.to)));
        return;
    }

    bool forMy = false;
    CPubKey senderPubKey;
    if (!message.decrypt(key, forMy, senderPubKey)) {
        if (forMy) {
            QMessageBox::warning(this, "", QString("cannot encrypt message from <%1>")
                    .arg(QString::fromStdString(message.from)));
        }
        return;
    }

    auto from = QString::fromStdString(message.from);

    std::vector<Message> messages;
    loadMessages(from, messages);
    messages.push_back(message);
    saveMessages(from, messages);

    // save sender pub key
    if (senderPubKey.IsValid()) {
        keysDb_.store(message.from, senderPubKey);
    }

    // add from address to model
    // or move to top
    usersModel.addAddress(from.toStdString());

    if (ui->addresses_SM->currentIndex().data(UsersModel::roleAddress).toString() == from ||
        !ui->addresses_SM->currentIndex().isValid()) {
        showForAddress(from);
    }

    emit newIncomingMessage(usersModel.labelForAddress(from), QString::fromStdString(message.text));
}

//*****************************************************************************
//*****************************************************************************
bool MessagesDialog::loadMessages(const QString &address, std::vector<Message> &result) {
    result.clear();

    std::vector<Message> messages;
    if (!chatDb_.load(address.toStdString(), messages)) {
        QMessageBox::warning(this, "",
                             trUtf8("Error when load messages for <%1>").arg(usersModel.labelForAddress(address)));
        return false;
    }

    BOOST_FOREACH(Message & msg, messages)
    {
        if (!msg.isExpired()) {
            result.push_back(msg);
        }
    }

    if (messages.size() != result.size()) {
        chatDb_.save(address.toStdString(), result);
    }

    return true;
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::saveMessages(const QString &address, const std::vector<Message> &messages) {
    chatDb_.save(address.toStdString(), messages);
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::clearMessages(const QString &address) {
    chatDb_.save(address.toStdString(), std::vector<Message>());
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::pushToUndelivered(const Message &message) {
    UndeliveredMap messages;
    chatDb_.loadUndelivered(messages);

    // check expired messages
    for (UndeliveredMap::iterator i = messages.begin(); i != messages.end();) {
        i->second.isExpired() ? messages.erase(i++) : ++i;
    }

    uint256 hash = message.getStaticHash();
    messages.insert(std::make_pair(hash, message));

    chatDb_.saveUndelivered(messages);
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::on_pasteButton_SM_clicked() {
    setFromAddress(QApplication::clipboard()->text());
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::on_pasteButtonTo_SM_clicked() {
    setToAddress(QApplication::clipboard()->text());
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::on_pasteButtonPublicKey_SM_clicked() {
    ui->pubKeyTo_SM->setText(QApplication::clipboard()->text());
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::on_addressBookButton_SM_clicked() {
    if (!walletModel) {
        // TODO
        // alert
        return;
    }

    AddressBookPage dlg(AddressBookPage::ForSending,
                        AddressBookPage::ReceivingTab, this);
    dlg.setModel(walletModel->getAddressTableModel());
    if (dlg.exec() != QDialog::Rejected) {
        setFromAddress(dlg.getReturnValue());
    }
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::on_addressBookButtonTo_SM_clicked() {
    if (!walletModel) {
        // TODO
        // alert
        return;
    }

    AddressBookPage dlg(AddressBookPage::ForSending,
                        AddressBookPage::SendingTab, this);
    dlg.setModel(walletModel->getAddressTableModel());
    if (dlg.exec() != QDialog::Rejected) {
        setToAddress(dlg.getReturnValue());
    }
}

//*****************************************************************************
//*****************************************************************************
bool MessagesDialog::checkAddress(const std::string &address) const {
    CBitcoinAddress bitcoinAddress(address);
    if (!bitcoinAddress.IsValid()) {
        // TODO
        // alert
        return false;
    }
    CKeyID keyID;
    if (!bitcoinAddress.GetKeyID(keyID)) {
        // TODO
        // alert
        QMessageBox::warning(this, tr("ERROR"), tr("Error get key  id"));
        return false;
    }

    return true;
}

//*****************************************************************************
//*****************************************************************************
bool MessagesDialog::getKeyForAddress(const std::string &address, CKey &key) const {
    if (!walletModel || !pwalletMain) {
        return false;
    }

    CBitcoinAddress bitcoinAddress(address);
    if (!bitcoinAddress.IsValid()) {
        return false;
    }

    CKeyID keyID;
    if (!bitcoinAddress.GetKeyID(keyID)) {
        return false;
    }

    // check unlock wallet and unlock
    if (!pwalletMain->IsCrypted() || !pwalletMain->IsLocked()) {
        // unlocked
        if (!pwalletMain->GetKey(keyID, key)) {
            return false;
        }
    } else {
        // locked
        WalletModel::UnlockContext ctx = walletModel->requestUnlock();
        if (!ctx.isValid()) {
            return false;
        }

        if (!pwalletMain->GetKey(keyID, key)) {
            return false;
        }
    }

    return true;
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::on_sendButton_SM_clicked() {
    Message message;

    // check empty message
    message.text = ui->outMessage_SM->text().toStdString();
    if (message.text.length() == 0) {
        QMessageBox::warning(this, "", "empty message");
        return;
    }

    if (message.text.size() > Message::DESCRIPTION::MAX_MESSAGE_SIZE) {
        message.text.resize(Message::DESCRIPTION::MAX_MESSAGE_SIZE);
    }

    // check source addr
    message.from = ui->addressIn_SM->text().toStdString();
    if (!checkAddress(message.from)) {
        ui->addressIn_SM->setValid(false);
        QMessageBox::warning(this, "", "invalid address");
        return;
    }

    // check destination addr
    message.to = ui->addressTo_SM->text().toStdString();
    if (!checkAddress(message.to)) {
        ui->addressTo_SM->setValid(false);
        QMessageBox::warning(this, "", "invalid address");
        return;
    }

    CKey myKey;
    if (!getKeyForAddress(message.from, myKey)) {
        QMessageBox::warning(this, "", "key not found");
        return;
    }

    if (!message.sign(myKey)) {
        QMessageBox::warning(this, "", "sign error");
        return;
    }

    std::vector<unsigned char> vkey;
    if (!ui->pubKeyTo_SM->text().isEmpty()) {
        DecodeBase58(ui->pubKeyTo_SM->text().toStdString(), vkey);
    }

    // check input key
    CKey destKey;
    CPubKey destPubKey;
    if (vkey.size() && destKey.SetPubKey(vkey)) {
        destPubKey = destKey.GetPubKey();

        // key is correct - store it
        keysDb_.store(message.to, destPubKey);
    } else {
        // not valid, check stored key
        if (!keysDb_.load(message.to, destPubKey)) {
            QMessageBox::warning(this, "", "invalid destination public key");
            return;
        }
    }

    message.date = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss").toStdString();

    // before send this message try to send undelivered
    {
        std::vector<std::string> addrs;
        addrs.push_back(message.to);
        resendUndelivered(addrs);
    }

    // send message
    {
        Message mCopy = message;
        if (!mCopy.encrypt(destPubKey)) {
            QMessageBox::warning(this, "", "message encryption failed");
            return;
        }
        mCopy.send();
        pushToUndelivered(mCopy);
    }

    messagesModel_.addMessage(message);
    saveMessages(ui->addressTo_SM->text(), messagesModel_.plainData());

    ui->outMessage_SM->clear();
    ui->messages_SM->scrollToBottom();

    usersModel.addAddress(message.to, false);
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::on_clearButton_SM_clicked() {
    QModelIndex idx = ui->addresses_SM->currentIndex();
    if (!idx.isValid()) {
        return;
    }

    if (QMessageBox::warning(this, "", trUtf8("Clear all messages?"), QMessageBox::Yes | QMessageBox::Cancel) !=
        QMessageBox::Yes) {
        return;
    }

    auto address = idx.data(UsersModel::roleAddress).toString();

    clearMessages(address);
    std::vector<Message> messages;
    messagesModel_.loadMessages(messages);
}

//*****************************************************************************
//*****************************************************************************
std::vector<std::string> MessagesDialog::getLocalAddresses() const {
    std::vector<std::string> result;

    if (!walletModel) {
        return result;
    }

    AddressTableModel *addressesModel = walletModel->getAddressTableModel();
    int rows = addressesModel->rowCount(QModelIndex());

    for (int i = 0; i < rows; ++i) {
        QModelIndex idx = addressesModel->index(i, AddressTableModel::Address, QModelIndex());
        QString type = idx.data(AddressTableModel::TypeRole).toString();
        if (type != AddressTableModel::Receive) {
            continue;
        }

        result.push_back(idx.data(Qt::DisplayRole).toString().toStdString());
    }

    return result;
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::requestUndeliveredMessages() {
    static quint64 requestCounter = 0;

    std::vector<std::string> addresses = getLocalAddresses();
    if (!addresses.size()) {
        return;
    }

    // send empty message for all addresses
    BOOST_FOREACH(std::string address, addresses)
    {
        Message message;
        message.from = address;
        message.date = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss").toStdString();
        message.send();
    }

    QTimer::singleShot(60000 * (++requestCounter > 8 ? 2 : 10), this, SLOT(requestUndeliveredMessages()));
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::on_addresses_SM_clicked(const QModelIndex &index) {
    if (!index.isValid()) {
        return;
    }

    QString address = index.data(UsersModel::roleAddress).toString();
    showForAddress(address);
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::onReadTimer() {
    if (ui->addresses_SM->currentIndex().isValid()) {
        usersModel.onRead(ui->addresses_SM->currentIndex());
    } else if (usersModel.rowCount() > 0) {
        usersModel.onRead(usersModel.index(0));
    }
}

//*****************************************************************************
//*****************************************************************************
void MessagesDialog::addressContextMenu(QPoint point) {
    QModelIndex index = ui->addresses_SM->indexAt(point);
    if (!index.isValid()) {
        return;
    }

    AddressTableModel *addressTableModel = walletModel->getAddressTableModel();
    if (!addressTableModel) {
        return;
    }

    QString address = index.data(UsersModel::roleAddress).toString();
    QString label = addressTableModel->labelForAddress(address);

    addToAddressBookAction_->setVisible(label.isEmpty());

    QAction *action = userContextMenu_.exec(QCursor::pos());


    if (action == addToAddressBookAction_) {

        EditAddressDialog editAddressDialog(EditAddressDialog::NewSendingAddress);
        editAddressDialog.setModel(addressTableModel);
        editAddressDialog.setAddress(address);
        if (editAddressDialog.exec()) {
            usersModel.addAddress(address.toStdString(), false);
        }
    } else if (action == deleteAction_) {
        int result = QMessageBox::information(this,
                                              trUtf8("Confirm delete"),
                                              trUtf8("Are you sure?"),
                                              QMessageBox::Yes | QMessageBox::Cancel);
        if (result == QMessageBox::Yes) {
            usersModel.deleteAddress(address.toStdString());
            chatDb_.erase(address.toStdString());
            messagesModel_.clear();
        }
    }
}

//*****************************************************************************
//*****************************************************************************
// static
bool MessagesDialog::resendUndelivered(const std::vector<std::string> &addresses) {
    ChatDb &chatDb = ChatDb::instance();

    UndeliveredMap map;
    if (!chatDb.loadUndelivered(map)) {
        return false;
    }

    bool needToSaveUndelivered = false;
    BOOST_FOREACH(const std::string address, addresses) {
        for (UndeliveredMap::iterator i = map.begin(); i != map.end();) {
            if (i->second.isExpired()) {
                // expired, delete
                map.erase(i++);
            } else {
                if (i->second.to == address) {
                    // update timestamp and resend message
                    Message mCopy = i->second;
                    // mCopy.date = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss").toStdString();
                    mCopy.timestamp = std::time(0);
                    mCopy.send();
                }
                ++i;
            }
        }
    }

    if (needToSaveUndelivered) {
        chatDb.saveUndelivered(map);
    }

    return true;
}
