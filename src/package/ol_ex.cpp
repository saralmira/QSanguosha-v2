#include "ol_ex.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "god.h"
#include "standard.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "util.h"
#include "wrapped-card.h"
#include "room.h"
#include "roomthread.h"

class OLEX_jiqiao : public TriggerSkill
{
public:
    OLEX_jiqiao() : TriggerSkill("olex_jiqiao")
    {
        events << EventPhaseStart << EventPhaseEnd << CardFinished;
        frequency = Skill::NotFrequent;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            if (room->askForSkillInvoke(player, objectName())) {
                room->broadcastSkillInvoke(objectName());
                QList<int> cards = room->getNCards(player->getMaxHp());
                player->addToPile("olex_jiqiao", cards, true);
            }
        } else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Play) {
            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
            room->throwCards(player->getPile("olex_jiqiao"), reason, NULL);
        } else if (triggerEvent == CardFinished) {
            CardUseStruct card_use = data.value<CardUseStruct>();
            if (card_use.card->isSkillOrDummy()) {
                return false;
            }
            QList<int> cards = player->getPile("olex_jiqiao");
            if (cards.length() > 0) {
                room->broadcastSkillInvoke(objectName());
                room->fillAG(cards, player);
                int card_id = room->askForAG(player, cards, false, objectName());
                room->clearAG(player);
                room->obtainCard(player, card_id, true);
                cards.removeOne(card_id);
                int value = 0;
                foreach (int cid, cards) {
                    const Card *card = Sanguosha->getCard(cid);
                    value = card->isRed() ? value + 1 : value - 1;
                }
                if (value == 0) {
                    room->recover(player, RecoverStruct(player, NULL, 1), true);
                } else {
                    room->loseHp(player, 1);
                }
            }
        }
        return false;
    }
};

class OLEX_Xiongyi : public TriggerSkill
{
public:
    OLEX_Xiongyi() : TriggerSkill("olex_xiongyi"), XushiGeneral("xushi")
    {
        events << AskForPeaches;
        frequency = Skill::Limited;
        limit_mark = "@olex_xiongyi";
    }

    const QString XushiGeneral;

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != player || player->getMark("@olex_xiongyi") < 1)
            return false;

        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->getGeneral()->objectName() == XushiGeneral || p->getGeneral2()->objectName() == XushiGeneral) {
                if (room->askForSimpleChoice(player, objectName(), "choice2")) {
                    room->notifySkillInvoked(player, objectName());
                    room->broadcastSkillInvoke(objectName());
                    room->doSuperLightbox("olex_sunyi", objectName());
                    room->removePlayerMark(player, "@olex_xiongyi");
                    room->recover(player, RecoverStruct(player, NULL, 1 - player->getHp()));
                    room->handleAcquireDetachSkills(player, "hunzi");
                }
                return false;
            }
        }

        if (room->askForSimpleChoice(player, objectName(), "choice1")) {
            room->notifySkillInvoked(player, objectName());
            room->broadcastSkillInvoke(objectName());
            room->doSuperLightbox("olex_sunyi", objectName());
            room->removePlayerMark(player, "@olex_xiongyi");
            room->recover(player, RecoverStruct(player, NULL, 3 - player->getHp()));
            const General *xushi = Sanguosha->getGeneral(XushiGeneral);
            if (xushi) {
                player->tag["newgeneral"] = xushi;
                bool isSecondaryHero = (player->getGeneralName() != "olex_sunyi");
                room->changeHero(player, XushiGeneral, false, true, isSecondaryHero, true, Room::KeepOriginMaxHp);
            }
        }

        return false;
    }
};

// yuqi start value.
inline int getYuqiValue(Player *player, const char *name)
{
    int v = player->property(name).toInt();
    if (!strcmp(name, "yuqi_dist"))
        return std::max(0, v);
    if (!strcmp(name, "yuqi_total"))
        return std::max(1, v);
    if (!strcmp(name, "yuqi_target"))
        return std::max(1, v);
    if (!strcmp(name, "yuqi_player"))
        return std::max(1, v);
    return v;
}

OLEXYuqiCard::OLEXYuqiCard()
{
    mute = true;
    will_throw = false;
    target_fixed = true;
}

void OLEXYuqiCard::use(Room *, ServerPlayer *, QList<ServerPlayer *> &) const
{
    // nothing
}

class OLEX_YuqiVAS : public ViewAsSkill
{
public:
    OLEX_YuqiVAS() : ViewAsSkill("olex_yuqi")
    {
        response_pattern = "@@olex_yuqi";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        QStringList l = Self->property("yuqi_toget").toString().split("+");
        QList<int> li = StringList2IntList(l);
        int count = Self->hasFlag("yuqi_InMovingStage1") ? getYuqiValue(Self, "yuqi_target") : getYuqiValue(Self, "yuqi_player");
        return selected.length() < count && li.contains(to_select->getEffectiveId());
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        OLEXYuqiCard *dummy = new OLEXYuqiCard;
        dummy->addSubcards(cards);
        dummy->setSkillName(objectName());
        return dummy;
    }
};

class OLEX_Yuqi : public TriggerSkill
{
public:
    OLEX_Yuqi() : TriggerSkill("olex_yuqi")
    {
        view_as_skill = new OLEX_YuqiVAS;
        events << Damaged << EventPhaseStart;
        frequency = Skill::NotFrequent;
        mustskillowner = false;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Damaged) {
            DamageStruct damage_data = data.value<DamageStruct>();
            if (damage_data.damage < 1)
                return false;

            QList<ServerPlayer *> players = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *p, players) {
                int distance = getYuqiValue(p, "yuqi_dist");
                int trigcount = p->tag["yuqi_trigcount"].toInt();
                if (p->distanceTo(player) <= distance && trigcount < 2 && room->askForSkillInvoke(p, objectName(), QVariant::fromValue(player))) {
                    room->broadcastSkillInvoke(objectName());

                    int total = getYuqiValue(p, "yuqi_total");
                    int target_count = getYuqiValue(p, "yuqi_target");
                    int player_count = getYuqiValue(p, "yuqi_player");
                    QList<int> cardids_player;
                    QList<int> cardids = room->getNCards(total, true);

                    CardMoveReason r(CardMoveReason::S_REASON_UNKNOWN, p->objectName());
                    CardsMoveStruct fake_move(cardids, NULL, p, Player::DrawPile, Player::PlaceHand, r);
                    QList<CardsMoveStruct> moves;
                    moves << fake_move;
                    QList<ServerPlayer *> tmpplayers;
                    tmpplayers << p;
                    room->notifyMoveCards(true, moves, false, tmpplayers);
                    room->notifyMoveCards(false, moves, false, tmpplayers);
                    //room->moveCardsAtomic(fake_move, false);
                    room->setPlayerFlag(p, "yuqi_InMovingStage1");
                    room->setPlayerProperty(p, "yuqi_toget", IntList2StringList(cardids).join("+"));
                    const Card *card_target = room->askForUseCard(p, "@@olex_yuqi", QString("@olex_yuqi_target:%1::%2").arg(player->objectName()).arg(target_count), -1, Card::MethodNone);
                    room->setPlayerFlag(p, "-yuqi_InMovingStage1");
                    bool trig_next = false;
                    if (card_target && card_target->getSubcards().length() > 0) {
                        trig_next = true;
                    }

                    p->tag["yuqi_trigcount"] = QVariant::fromValue(trigcount + 1);
                    const Card *card_player = NULL;
                    if (trig_next) {
                        foreach (int cid, cardids) {
                            if (!card_target->hasSubcard(cid)) {
                                cardids_player << cid;
                            }
                        }
                        //room->moveCardsAtomic(CardsMoveStruct(card_target->getSubcards(), player, Player::DrawPile, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_PREVIEWGIVE, p->objectName(), objectName(), QString())), false);
                        // room->obtainCard(player, card_target, CardMoveReason(CardMoveReason::S_REASON_PREVIEWGIVE, p->objectName(), objectName(), QString()), false);
                        room->setPlayerProperty(p, "yuqi_toget", IntList2StringList(cardids_player).join("+"));
                        card_player = room->askForUseCard(p, "@@olex_yuqi", QString("@olex_yuqi_player:::%1").arg(player_count), -1, Card::MethodNone);
                    }

                    CardsMoveStruct fake_move2(cardids, p, NULL, Player::PlaceHand, Player::DrawPile, r);
                    QList<CardsMoveStruct> moves2;
                    moves2 << fake_move2;
                    room->notifyMoveCards(true, moves2, false, tmpplayers);
                    room->notifyMoveCards(false, moves2, false, tmpplayers);
                    //room->moveCardsAtomic(fake_move2, false);

                    if (trig_next) {
                        if (card_target)
                            room->moveCardsAtomic(CardsMoveStruct(card_target->getSubcards(), NULL, player, Player::DrawPile, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_PREVIEWGIVE, p->objectName(), objectName(), QString())), false);
                        if (card_player)
                            room->moveCardsAtomic(CardsMoveStruct(card_player->getSubcards(), NULL, p, Player::DrawPile, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_GOTCARD, p->objectName(), objectName(), QString())), false);
                        //room->obtainCard(player, card_target, CardMoveReason(CardMoveReason::S_REASON_PREVIEWGIVE, p->objectName(), objectName(), QString()), false);
                        //room->obtainCard(p, card_player, CardMoveReason(CardMoveReason::S_REASON_GOTCARD, p->objectName(), objectName(), QString()), false);
                    }
                }
            }
        }
        else if (player->getPhase() == Player::RoundStart) {
            QList<ServerPlayer *> players = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *p, players) {
                p->tag["yuqi_trigcount"] = QVariant::fromValue(0);
            }
        }

        return false;
    }
};

class OLEX_Shanshen : public TriggerSkill
{
public:
    OLEX_Shanshen() : TriggerSkill("olex_shanshen")
    {
        events << Death << Damage;
        frequency = Skill::Frequent;
        mustskillowner = false;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            target = death.who;
            QList<ServerPlayer *> players = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *player, players) {
                QStringList deathlist = player->tag["shanshen_death"].toStringList();
                if (target == player || deathlist.contains(target->objectName()) || !room->askForSkillInvoke(player, objectName()))
                    continue;
                room->broadcastSkillInvoke(objectName());
                deathlist << target->objectName();
                player->tag["shanshen_death"] = deathlist;

                int distance = getYuqiValue(player, "yuqi_dist");
                int total = getYuqiValue(player, "yuqi_total");
                int target_count = getYuqiValue(player, "yuqi_target");
                int player_count = getYuqiValue(player, "yuqi_player");
                QStringList choicelist;
                choicelist << QString("dist:%1::%2/5").arg(distance < 5 ? "" : "disabled").arg(distance);
                choicelist << QString("total:%1::%2/5").arg(total < 5 ? "" : "disabled").arg(total);
                choicelist << QString("target:%1::%2/5").arg(target_count < 5 ? "" : "disabled").arg(target_count);
                choicelist << QString("player:%1::%2/5").arg(player_count < 5 ? "" : "disabled").arg(player_count);
                if (choicelist.length() == 0 || (distance >= 5 && total >= 5 && target_count >= 5 && player_count >= 5)) {
                    continue;
                }
                QString choice = room->askForChoice(player, objectName(), choicelist.join("+"), data);
                if (choice == "dist") {
                    room->setPlayerProperty(player, "yuqi_dist", std::min(distance + 2, 5));
                } else if (choice == "total") {
                    room->setPlayerProperty(player, "yuqi_total", std::min(total + 2, 5));
                } else if (choice == "target") {
                    room->setPlayerProperty(player, "yuqi_target", std::min(target_count + 2, 5));
                } else if (choice == "player") {
                    room->setPlayerProperty(player, "yuqi_player", std::min(player_count + 2, 5));
                }

                QStringList damagelist = player->tag["shanshen_damage"].toStringList();
                if (!damagelist.contains(target->objectName())) {
                    room->recover(player, RecoverStruct(player), true);
                }
            }
        }
        else if (triggerEvent == Damage && target->hasSkill(objectName())) {
            DamageStruct damage = data.value<DamageStruct>();
            QStringList damagelist = target->tag["shanshen_damage"].toStringList();
            if (damage.to && !damagelist.contains(damage.to->objectName())) {
                damagelist << damage.to->objectName();
                target->tag["shanshen_damage"] = damagelist;
            }
        }
        return false;
    }
};

class OLEX_Xianjing : public TriggerSkill
{
public:
    OLEX_Xianjing() : TriggerSkill("olex_xianjing")
    {
        events << EventPhaseProceeding;
        frequency = Skill::Frequent;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::RoundStart) {
            int trig_count = player->getHp() == player->getMaxHp() ? 2 : 1;
            if (!room->askForSkillInvoke(player, objectName()))
                return false;
            room->broadcastSkillInvoke(objectName());

            XIANJING_START:
            if (trig_count < 1) {
                return false;
            }
            --trig_count;
            int distance = getYuqiValue(player, "yuqi_dist");
            int total = getYuqiValue(player, "yuqi_total");
            int target_count = getYuqiValue(player, "yuqi_target");
            int player_count = getYuqiValue(player, "yuqi_player");
            QStringList choicelist;
            choicelist << QString("dist:%1::%2/5").arg(distance < 5 ? "" : "disabled").arg(distance);
            choicelist << QString("total:%1::%2/5").arg(total < 5 ? "" : "disabled").arg(total);
            choicelist << QString("target:%1::%2/5").arg(target_count < 5 ? "" : "disabled").arg(target_count);
            choicelist << QString("player:%1::%2/5").arg(player_count < 5 ? "" : "disabled").arg(player_count);
            if (choicelist.length() == 0 || (distance >= 5 && total >= 5 && target_count >= 5 && player_count >= 5)) {
                return false;
            }
            QString choice = room->askForChoice(player, objectName(), choicelist.join("+"));
            if (choice == "dist") {
                room->setPlayerProperty(player, "yuqi_dist", std::min(distance + 1, 5));
            } else if (choice == "total") {
                room->setPlayerProperty(player, "yuqi_total", std::min(total + 1, 5));
            } else if (choice == "target") {
                room->setPlayerProperty(player, "yuqi_target", std::min(target_count + 1, 5));
            } else if (choice == "player") {
                room->setPlayerProperty(player, "yuqi_player", std::min(player_count + 1, 5));
            }
            goto XIANJING_START;
        }

        return false;
    }
};

/* error: undefined reference to `vtable for OLEXPackage
 *
 * 原因
 * 导致错误信息的原因是：子类没有实现父类的纯虚函数；
在Qt中，首先要想到的是在一个类中添加了新的继承QObject，并添加了 Q_OBJECT 宏，然后执行构造或重新构造，都会造成这个错误。
根本原因是，只执行构造或重新构造，都不会编译新添加的宏Q_OBJECT。因此在这之前要执行qmake，让moc编译器去预编译Q_OBJECT，然后再执行构造，就不再报错了。
 *
 * 解决办法
 * 首先重新执行qmake，然后再执行构造。
*/

OLEXPackage::OLEXPackage() : Package("olex")
{
    // generals
    General *olex_sunyi = new General(this, "olex_sunyi", "wu", 5);
    olex_sunyi->addSkill(new OLEX_jiqiao);
    olex_sunyi->addSkill(new OLEX_Xiongyi);

    General *olex_caojinyu = new General(this, "olex_caojinyu", "wei", 3, false);
    olex_caojinyu->addSkill(new OLEX_Yuqi);
    olex_caojinyu->addSkill(new OLEX_Shanshen);
    olex_caojinyu->addSkill(new OLEX_Xianjing);

    addMetaObject<OLEXYuqiCard>();
}

ADD_PACKAGE(OLEX)
