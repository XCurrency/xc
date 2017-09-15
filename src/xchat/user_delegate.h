#ifndef USER_DELEGATE_H
#define USER_DELEGATE_H

#include <QStyledItemDelegate>

//*****************************************************************************
//*****************************************************************************
class UserDelegate : public QStyledItemDelegate {
    enum {
        fontSizeForLabel = 14,
        fontSizeForAddress = 10
    };

public:
    UserDelegate() = default;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};


#endif // USER_DELEGATE_H