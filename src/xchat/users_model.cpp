#include "users_model.h"
#include "../qt/addresstablemodel.h"

#include <QDebug>

//*****************************************************************************
//*****************************************************************************
UsersModel::UsersModel(QObject *parent) :
        QAbstractListModel(parent), addressTableModel_(nullptr) {
}

//*****************************************************************************
//*****************************************************************************
// virtual
int UsersModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return static_cast<int>(items_.size());
}

//*****************************************************************************
//*****************************************************************************
// virtual
QVariant UsersModel::data(const QModelIndex &index, int role) const {
    auto row = static_cast<quint32>(index.row());
    if (row >= items_.size()) {
        return QVariant();
    }

    const Item &item = items_.at(row);

    QString address = QString::fromStdString(item.addr);

    QString label = labelForAddressWithAddress(address);

    switch (role) {
        case Qt::DisplayRole:
            return (!item.isRead_ ? "* " : "") + label;
        case Qt::ToolTipRole:
            return label;
        case roleAddress:
            return address;
        case roleLabel:
            return labelForAddress(address);
        case roleIsRead:
            return item.isRead_;
        default:
            return QVariant();
    }

}

//*****************************************************************************
//*****************************************************************************
void UsersModel::loadAddresses(const std::vector<std::string> &addresses) {
    emit beginResetModel();
    // items_ = addresses;
    for (const auto &address : addresses) {
        items_.emplace_back(address, true);
    }
    emit endResetModel();
}

//*****************************************************************************
//*****************************************************************************
void UsersModel::addAddress(const std::string &address, bool isNewMessage) {
    emit beginResetModel();
    items_.erase(std::remove(items_.begin(), items_.end(), address), items_.end());
    items_.insert(items_.begin(), Item(address, !isNewMessage));
    emit endResetModel();
}

//*****************************************************************************
//*****************************************************************************
void UsersModel::deleteAddress(const std::string &address) {

    emit beginResetModel();
    items_.erase(std::remove(items_.begin(), items_.end(), address), items_.end());
    emit endResetModel();
}

//*****************************************************************************
//*****************************************************************************
//const std::vector<Message> & UsersModel::plainData() const
//{
//    return m_messages;
//}

//*****************************************************************************
//*****************************************************************************
void UsersModel::onRead(const QModelIndex &index) {

    if (!index.isValid()) {
        return;
    }
    if (index.row() > items_.size()) {
        return;
    }
    items_[index.row()].isRead_ = true;
    emit dataChanged(index, index);
}

//*****************************************************************************
//*****************************************************************************
void UsersModel::setAddressTableModel(AddressTableModel *model) {
    addressTableModel_ = model;
}

//*****************************************************************************
//*****************************************************************************
QString UsersModel::labelForAddress(const QString &address) const {

    static const QString emptyLabel = trUtf8("(no label)");

    if (!addressTableModel_) {
        return emptyLabel;
    }
    QString label = addressTableModel_->labelForAddress(address);
    return label.isEmpty() ? emptyLabel : label;
}

//*****************************************************************************
//*****************************************************************************
QString UsersModel::labelForAddressWithAddress(const QString &address) const {
    if (!addressTableModel_) {
        return address;
    }

    QString label = addressTableModel_->labelForAddress(address);
    return label.isEmpty() ? address : label + '(' + address + ')';
}
