#ifndef CHOOSECARDDIALOG_H
#define CHOOSECARDDIALOG_H

class QSanCommandProgressBar;

#include <QToolButton>

class ChooseCardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChooseCardDialog(const QStringList &general_names, QWidget *parent, bool view_only = false, const QString &title = QString());

private:
    QSanCommandProgressBar *progress_bar;

};

#endif // CHOOSECARDDIALOG_H
