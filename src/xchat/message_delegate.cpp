#include "message_delegate.h"
#include "messagesmodel.h"

#include <QPainter>

QSize MessageDelegate::sizeHint(const QStyleOptionViewItem &option,
                                const QModelIndex &index) const {

    QRectF rect = option.rect;
    rect.adjust(8, 0, -8, 0);

    // calc size for date
    QRectF rectForDate;
    {
        QString dateTime = index.data(MessagesModel::roleDateTimeString).toString();

        QFont f;
        f.setPixelSize(fontSizeForDate);

        QFontMetricsF fm(f);
        rectForDate = fm.boundingRect(rect, Qt::AlignBottom, dateTime);

    }

    // size for text
    QRectF rectForText;
    {
        QString text = index.data().toString();

        QFont f;
        f.setPixelSize(fontSizeForText);

        QFontMetricsF fm(f);
        rectForText = fm.boundingRect(rect, Qt::AlignBottom | Qt::TextWordWrap, text);
    }


    QSize size;// = QStyledItemDelegate::sizeHint(option, index);

    size.setHeight(static_cast<int>(rectForDate.height() + rectForText.height()));
    // size.setWidth(qMax(rectForDate.width(), rectForText.width()));//size.width());
    size.setWidth(static_cast<int>(rect.width()));

    return size;
}

//*****************************************************************************
//*****************************************************************************
void MessageDelegate::paint(QPainter *painter,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index) const {

    // QStyledItemDelegate::paint(painter, option, index);

    if (!index.isValid()) {
        return;
    }

    // Message msg = index.data().value<Message>();
    QString text = index.data().toString();
    bool isIncoming = index.data(MessagesModel::roleIncoming).toBool();
    QString dateTime = index.data(MessagesModel::roleDateTimeString).toString();

    painter->setPen(Qt::SolidLine);
    painter->setPen(QColor(Qt::lightGray));

    // grid
    painter->drawLine(QLine(option.rect.bottomLeft(), option.rect.bottomRight()));
    // painter->drawLine(QLine(option.rect.topRight(), option.rect.bottomRight()));

    // font size
    auto f = painter->font();

    // ajust rect
    QRect rect(option.rect);
    rect.adjust(8, 0, -8, 0);

    // text options
    QTextOption to;
    to.setWrapMode(QTextOption::WordWrap);
    to.setAlignment(Qt::AlignLeft | Qt::AlignBottom);

    // draw text
    {
        painter->setPen(QColor(Qt::black));
        f.setPixelSize(fontSizeForText);
        painter->setFont(f);
        painter->drawText(rect, text, to);
    }

    if (isIncoming) {
        painter->setPen(QColor(Qt::darkGreen));
    } else {
        painter->setPen(QColor(Qt::darkBlue));
    }

    // rect.setHeight(rect.height() / 2);
    rect.setHeight(fontSizeForDate);

    // draw date
    f.setPixelSize(fontSizeForDate);
    painter->setFont(f);

    to.setWrapMode(QTextOption::NoWrap);
    // to.setAlignment(Qt::AlignLeft | Qt::AlignTop);

    if (isIncoming) {
        painter->drawText(rect, ">> " + dateTime, to);
    } else {
        painter->drawText(rect, "<< " + dateTime, to);
    }
}
