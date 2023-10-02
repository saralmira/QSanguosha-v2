#include "choosecarddialog.h"
#include "choosegeneraldialog.h"
#include "card.h"
#include "engine.h"
#include "client.h"
#include "settings.h"
#include "protocol.h"
#include "skin-bank.h"
#include "clientplayer.h"
#include "clientstruct.h"
#include "timed-progressbar.h"

using namespace QSanProtocol;

ChooseCardDialog::ChooseCardDialog(const QStringList &card_names, QWidget *parent, bool view_only, const QString &title)
    : QDialog(parent)
{
    if (title.isEmpty())
        setWindowTitle(tr("Choose card"));
    else
        setWindowTitle(title);

    QSignalMapper *mapper = new QSignalMapper(this);
    QList<OptionButton *> buttons;
    QSize icon_size = G_COMMON_LAYOUT.m_chooseCardBoxDenseIconSize;

    foreach (const QString cardstr, card_names) {
        QString caption = Sanguosha->translate(cardstr);
        OptionButton *button = new OptionButton(QString(), caption);
        button->setIcon(QIcon(G_ROOM_SKIN.getCardMainPixmap(cardstr)));
        button->setIconSize(icon_size);
        button->setToolTip(Card::getDescription(cardstr));
        buttons << button;

        if (!view_only) {
            mapper->setMapping(button, cardstr);
            if (Config.OneClickToChoose) {
                connect(button, SIGNAL(clicked()), mapper, SLOT(map()));
                connect(button, SIGNAL(clicked()), this, SLOT(accept()));
            } else {
                connect(button, SIGNAL(double_clicked()), mapper, SLOT(map()));
                connect(button, SIGNAL(double_clicked()), this, SLOT(accept()));
            }
        }
    }

    QLayout *layout = NULL;
    const int columns = G_COMMON_LAYOUT.m_chooseCardBoxSwitchIconEachRow;

    if (card_names.length() <= columns) {
        layout = new QHBoxLayout;
        foreach(OptionButton *button, buttons)
            layout->addWidget(button);
    } else {
        QGridLayout *grid_layout = new QGridLayout;
        QHBoxLayout *hlayout = new QHBoxLayout;

        int columns_x = qMin(columns, (buttons.length() + 1) / 2);
        for (int i = 0; i < buttons.length(); i++) {
            int row = i / columns_x;
            int column = i % columns_x;
            grid_layout->addWidget(buttons.at(i), row, column);
        }
        hlayout->addLayout(grid_layout);
        layout = hlayout;
    }

    QString default_name = card_names.first();
    for (int i = 0; i < buttons.size(); i++) {
        if (buttons.at(i)->isEnabled()) {
            default_name = card_names.at(i);
            break;
        }
    }

    if (!view_only) {
        mapper->setMapping(this, default_name);
        connect(this, SIGNAL(rejected()), mapper, SLOT(map()));

        connect(mapper, SIGNAL(mapped(QString)), ClientInstance, SLOT(onPlayerChooseTemplateCard(QString)));
    }

    QVBoxLayout *dialog_layout = new QVBoxLayout;
    dialog_layout->addLayout(layout);

    // progress bar & free choose button
    QHBoxLayout *last_layout = new QHBoxLayout;
    if (view_only || ServerInfo.OperationTimeout == 0) {
        progress_bar = NULL;
    } else {
        progress_bar = new QSanCommandProgressBar();
        progress_bar->setFixedWidth(300);
        progress_bar->setTimerEnabled(true);
        progress_bar->setCountdown(QSanProtocol::S_COMMAND_CHOOSE_TEMPLATE_CARD);
        progress_bar->show();
        last_layout->addWidget(progress_bar);
    }

    last_layout->addStretch();

    if (last_layout->count() != 0) {
        dialog_layout->addLayout(last_layout);
    }

    setLayout(dialog_layout);
}


