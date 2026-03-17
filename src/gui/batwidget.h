#ifndef BATWIDGET_H
#define BATWIDGET_H

#include <QWidget>
#include <QPixmap>

class BatWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BatWidget(QWidget *parent = nullptr);
    void setMessage(const QString &msg);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap m_logo;
    QString m_message;
};

#endif
