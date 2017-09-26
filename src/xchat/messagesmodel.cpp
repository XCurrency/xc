#include "messagesmodel.h"


#include "messagesmodel.h"

#include <QDebug>

//*****************************************************************************
//*****************************************************************************
MessagesModel::MessagesModel(QObject *parent) :
        QAbstractListModel(parent) {}

//*****************************************************************************
//*****************************************************************************
// virtual
int MessagesModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return static_cast<int>(messages_.size());
}

//*****************************************************************************
//*****************************************************************************
// virtual
QVariant MessagesModel::data(const QModelIndex &index, int role) const {
    auto row = static_cast<quint32>(index.row());
    if (row >= messages_.size()) {
        return QVariant();
    }

    const Message &message = messages_[row];
    if (role == Qt::DisplayRole) {
        return QString::fromStdString(message.text);
    } else if (role == roleMessage) {
        return QVariant::fromValue(message);
    } else if (role == roleIncoming) {
        return message.appliesToMe();
    } else if (role == roleDateTime) {
        return QDateTime::fromTime_t(message.getTime());
    } else if (role == roleDateTimeString) {
        return QDateTime::fromTime_t(message.getTime()).toString("yyyy-MM-dd hh:mm:ss");
    }

    return QVariant();
}

//*****************************************************************************
//*****************************************************************************
void MessagesModel::loadMessages(vector<Message> &messages) {
    emit beginResetModel();
    messages_ = std::move(messages);
    std::sort(messages_.begin(), messages_.end());
    emit endResetModel();
}

//*****************************************************************************
//*****************************************************************************
void MessagesModel::addMessage(const Message &message) {
    auto size = messages_.size();
    beginInsertRows(QModelIndex(), size, size + 1);
    messages_.push_back(message);
    endInsertRows();
}

//*****************************************************************************
//*****************************************************************************
void MessagesModel::clear() {
    emit beginResetModel();
    messages_.clear();
    emit endResetModel();
}

//*****************************************************************************
//*****************************************************************************
const std::vector<Message> &MessagesModel::plainData() const {
    return messages_;
}
