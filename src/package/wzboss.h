#ifndef WZBOSS_H
#define WZBOSS_H

#include "package.h"
#include "standard.h"

class WZFenweiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WZFenweiCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class WZSuoshiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WZSuoshiCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class WzbossPackage : public Package
{
    Q_OBJECT

public:
    WzbossPackage();
    static QList<const General *> getGenerals();
    static bool contains(const QString &general_name);
};


#endif // WZBOSS_H
