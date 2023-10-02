#include "startupdialog.h"
#include <QDebug>
#include <QVBoxLayout>
#include <QLabel>

StartUpDialog::StartUpDialog(QWidget *parent) : NoBorderBaseWidget(parent)
{
    QString image;

    QLabel *ql = new QLabel;
    switch (qrand() % 4) {
    case 0: image = "wei"; break;
    case 1: image = "shu"; break;
    case 2: image = "wu"; break;
    case 3: image = "qun"; break;
    default:
        break;
    }
    ql->setPixmap(QPixmap(QString("image/system/backdrop/%1.jpg").arg(image)));

    QVBoxLayout *qbl = new QVBoxLayout;
    qbl->addWidget(ql);
    setLayout(qbl);

    resize(1000, 353);
}

NoBorderBaseWidget::NoBorderBaseWidget(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    m_areaMovable = geometry();
    m_bPressed = false;
}

void NoBorderBaseWidget::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
    {
        m_ptPress = e->pos();
        qDebug() << pos() << e->pos() << m_ptPress;
        m_bPressed = m_areaMovable.contains(m_ptPress);
    }
}

void NoBorderBaseWidget::mouseMoveEvent(QMouseEvent *e)
{
    if(m_bPressed)
    {
        qDebug() << pos() << e->pos() << m_ptPress;
        move(pos() + e->pos() - m_ptPress);
    }
}

void NoBorderBaseWidget::mouseReleaseEvent(QMouseEvent *)
{
    m_bPressed = false;
}

void NoBorderBaseWidget::setAreaMovable(const QRect rt)
{
    if(m_areaMovable != rt)
    {
        m_areaMovable = rt;
    }
}
