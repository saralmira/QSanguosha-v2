#ifndef STARTUPDIALOG_H
#define STARTUPDIALOG_H

#include <QWidget>
#include <QMouseEvent>

class NoBorderBaseWidget : public QWidget
{
  Q_OBJECT
public:
  explicit NoBorderBaseWidget(QWidget *parent = 0);

  void setAreaMovable(const QRect rt);

protected:
  void mousePressEvent(QMouseEvent *);
  void mouseMoveEvent(QMouseEvent *);
  void mouseReleaseEvent(QMouseEvent *);

private:
  QRect m_areaMovable;
  bool m_bPressed;
  QPoint m_ptPress;
};

class StartUpDialog : public NoBorderBaseWidget
{
    Q_OBJECT
public:
    explicit StartUpDialog(QWidget *parent = 0);

signals:

public slots:
};

#endif // STARTUPDIALOG_H
