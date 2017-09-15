#include "user_delegate.h"
#include "users_model.h"

#include <QPainter>

//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//*****************************************************************************
QSize UserDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const {

    QRectF rect = option.rect;
    rect.adjust(8, 0, -8, 0);

    // calc size for label
    QRectF rectForLabel;
    {
        auto label = index.data(UsersModel::roleLabel).toString();

        QFont font;
        font.setPixelSize(fontSizeForLabel);

        QFontMetricsF fm(font);
        rectForLabel = fm.boundingRect(rect, Qt::AlignBottom, label);
    }

    // size for addr
    QRectF rectForAddress;
    {
        QString address = index.data(UsersModel::roleAddress).toString();

        QFont font;
        font.setPixelSize(fontSizeForAddress);

        QFontMetricsF fontMetricsF(font);
        rectForAddress = fontMetricsF.boundingRect(rect, Qt::AlignBottom | Qt::TextWordWrap, address);
    }


    QSize size;// = QStyledItemDelegate::sizeHint(option, index);

    size.setHeight(static_cast<int>(rectForLabel.height() + rectForAddress.height()));
    size.setWidth(static_cast<int>(rect.width()));

    return size;
}

//*****************************************************************************
//*****************************************************************************
void UserDelegate::paint(QPainter *painter,
                         const QStyleOptionViewItem &option,
                         const QModelIndex &index) const {

    // QStyledItemDelegate::paint(painter, option, index);

    if (!index.isValid()) {
        return;
    }

    QRect rect(option.rect);

    auto isRead = index.data(UsersModel::roleIsRead).toBool();
    auto label = index.data(UsersModel::roleLabel).toString();
    auto address = index.data(UsersModel::roleAddress).toString();

    // background
    if (option.state & QStyle::State_Selected) {
        QBrush bb(QColor(Qt::lightGray));
        painter->fillRect(rect, bb);
    }

    // draw grid
    {
        painter->setPen(Qt::SolidLine);
        painter->setPen(QColor(Qt::lightGray));

        painter->drawLine(QLine(rect.bottomLeft(), rect.bottomRight()));
    }

    // font size
    auto font = painter->font();

    // ajust rect
    rect.adjust(8, 0, -8, 0);

    painter->setPen(QColor(Qt::black));

    // draw label
    {
        font.setPixelSize(fontSizeForLabel);
        painter->setFont(font);
        painter->drawText(rect, isRead ? label : ("* " + label));
    }

    // draw address
    {
        painter->setPen(QColor(Qt::darkGray));

        rect.adjust(0, fontSizeForLabel, 0, 0);

        font.setPixelSize(fontSizeForAddress);
        painter->setFont(font);
        painter->drawText(rect, address);
    }
}
