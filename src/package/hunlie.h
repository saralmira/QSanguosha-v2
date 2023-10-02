#ifndef HUNLIE_H
#define HUNLIE_H

#include "package.h"
#include "standard.h"


class SKYanmieCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SKYanmieCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class SKJieyanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SKJieyanCard();
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class SKFenyingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SKFenyingCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class SKQianqiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SKQianqiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class SKShajueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SKShajueCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class SKGuiquCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SKGuiquCard();
    void onEffect(const CardEffectStruct &effect) const;
};

class SKGuiyuanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SKGuiyuanCard();
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class HunLiePackage : public Package
{
    Q_OBJECT

public:
    HunLiePackage();
};


#endif // HUNLIE_H
