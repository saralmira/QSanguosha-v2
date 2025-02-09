#ifndef _CARD_ITEM_H
#define _CARD_ITEM_H

#include "qsan-selectable-item.h"
#include "settings.h"

class FilterSkill;
class General;
class Card;

class CardItem : public QSanSelectableItem
{
    Q_OBJECT

public:
    CardItem(const Card *card);
    CardItem(const QString &general_name);
    ~CardItem();

    virtual QRectF boundingRect() const;
    virtual void setEnabled(bool enabled);

    const Card *getCard() const;
    void setCard(const Card *card);
    inline int getId() const
    {
        return m_cardId;
    }

    // For move card animation
    void setHomePos(QPointF home_pos);
    QPointF homePos() const;
    QAbstractAnimation *getGoBackAnimation(bool doFadeEffect, bool smoothTransition = false,
        int duration = Config.S_MOVE_CARD_ANIMATION_DURATION);
    void goBack(bool playAnimation, bool doFade = true);
    inline QAbstractAnimation *getCurrentAnimation(bool)
    {
        return m_currentAnimation;
    }
    inline void setHomeOpacity(double opacity)
    {
        m_opacityAtHome = opacity;
    }
    inline double getHomeOpacity()
    {
        return m_opacityAtHome;
    }

    void showAvatar(const General *general);
    void hideAvatar();
    void setAutoBack(bool auto_back);
    void changeGeneral(const QString &general_name);
    void setFootnote(const QString &desc);

    inline bool isSelected() const
    {
        return m_isSelected;
    }
    inline void setSelected(bool selected)
    {
        m_isSelected = selected;
    }
    bool isEquipped() const;

    void setFrozen(bool is_frozen);

    inline void showFootnote()
    {
        _m_showFootnote = true;
    }
    inline void hideFootnote()
    {
        _m_showFootnote = false;
    }

    static CardItem *FindItem(const QList<CardItem *> &items, int card_id);

    struct UiHelper
    {
        int tablePileClearTimeStamp;
    } m_uiHelper;

    void clickItem()
    {
        emit clicked();
    }

    bool m_setDisabled;

private slots:
    void currentAnimationDestroyed();

protected:
    void _initialize();
    QAbstractAnimation *m_currentAnimation;
    QImage _m_footnoteImage;
    bool _m_showFootnote;
    // QGraphicsPixmapItem *_m_footnoteItem;
    QMutex m_animationMutex;
    double m_opacityAtHome;
    bool m_isSelected;
    bool _m_isUnknownGeneral;
    static const int _S_CLICK_JITTER_TOLERANCE;
    static const int _S_MOVE_JITTER_TOLERANCE;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    int m_cardId;
    QString _m_avatarName;
    QPointF home_pos;
    QPointF _m_lastMousePressScenePos;
    bool auto_back, frozen;
    bool m_isShiny;

signals:
    void toggle_discards();
    void clicked();
    void double_clicked();
    void thrown();
    void released();
    void enter_hover();
    void leave_hover();
    void movement_animation_finished();
};

#endif

