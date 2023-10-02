#ifndef OL_EX_H
#define OL_EX_H

#include "package.h"
#include "standard.h"

class OLEXYuqiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE OLEXYuqiCard();
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class OLEXPackage : public Package
{
    Q_OBJECT

public:
    OLEXPackage();
};

#endif // OL_EX_H
