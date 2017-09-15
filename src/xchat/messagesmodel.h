#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include "message.h"
#include "message_meta_type.h"

#include <QAbstractListModel>
#include <QDateTime>
#include <QString>

#include <vector>


//*****************************************************************************
//*****************************************************************************
class MessagesModel : public QAbstractListModel {
Q_OBJECT

public:
    enum {
        roleMessage = Qt::UserRole,
        roleIncoming,
        roleDateTime,
        roleDateTimeString
    };

public:
    explicit MessagesModel(QObject *parent = nullptr);

public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void loadMessages(vector<Message> &messages);

    void addMessage(const Message &message);

    void clear();

    const std::vector<Message> &plainData() const;

signals:

public slots:

private:
    std::vector<Message> messages_;

};


#endif // MESSAGESMODEL_H