#ifndef SKGENERALS_H
#define SKGENERALS_H

#include "package.h"
#include "standard.h"
#include "wind.h"


class SKZiguoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SKZiguoCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class SKGuhuoDialog : public GuhuoDialog
{
    Q_OBJECT

public:
    static SKGuhuoDialog *getInstance(const QString &object);

protected:
    explicit SKGuhuoDialog(const QString &object);
};

class SKGuhuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SKGuhuoCard();

    bool targetFixed() const;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    const Card *validate(CardUseStruct &card_use) const;
};

class SKShemiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SKShemiCard();
    void onUse(Room *room, const CardUseStruct &card_use) const;
};

class SKFuzhuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SKFuzhuCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class SKSijianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SKSijianCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class SKShejianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SKShejianCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class SKPackage : public Package
{
    Q_OBJECT

public:
    SKPackage();
};


#endif // SKGENERALS_H
