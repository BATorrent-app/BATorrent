#ifndef PROGRESSDELEGATE_H
#define PROGRESSDELEGATE_H

#include <QStyledItemDelegate>
#include <QTimer>
#include <QElapsedTimer>

class ProgressDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ProgressDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

private:
    QTimer *m_animTimer;
    QElapsedTimer m_elapsed;
};

#endif
