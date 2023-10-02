#ifndef HULAO_H
#define HULAO_H

#include "package.h"
#include "standard.h"

class HLZongyuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HLZongyuCard();
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class HulaoPackage : public Package
{
    Q_OBJECT

public:
    HulaoPackage();
    static QList<const General *> getGenerals();
    static bool contains(const QString &general_name);
};

#endif // HULAO_H
