#ifndef USERS_MODEL_H
#define USERS_MODEL_H


#include <QAbstractListModel>
#include <QDateTime>
#include <QString>

#include <utility>
#include <vector>

class AddressTableModel;

//*****************************************************************************
//*****************************************************************************
class UsersModel : public QAbstractListModel {
Q_OBJECT

public:
    enum {
        roleAddress = Qt::UserRole,
        roleLabel,
        roleIsRead
    };

    struct Item {
        std::string addr;
        bool isRead_;

        explicit Item(std::string address, const bool isRead = false)
                : addr(std::move(address)), isRead_(isRead) {}

        bool operator==(const std::string &address) const {
            return addr == address;
        }

        bool operator==(const Item &item) const {
            return addr == item.addr;
        }
    };

public:
    explicit UsersModel(QObject *parent = nullptr);

public:
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void loadAddresses(const std::vector<std::string> &addresses);

    void addAddress(const std::string &address, bool isNewMessage = true);

    void deleteAddress(const std::string &address);

    void setAddressTableModel(AddressTableModel *model);
    // const std::vector<Message> & plainData() const;

    QString labelForAddress(const QString &address) const;

    QString labelForAddressWithAddress(const QString &address) const;

signals:

public slots:

    void onRead(const QModelIndex &index);

private:
    std::vector<Item> items_;

    AddressTableModel *addressTableModel_;
};


#endif // USERS_MODEL_H