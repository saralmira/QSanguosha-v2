#include "hunlie.h"
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
#include "json.h"

SKYanmieCard::SKYanmieCard()
{
    will_throw = true;
    target_fixed = false;
    handling_method = Card::MethodDiscard;
}

bool SKYanmieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (targets.size() > 0)
        return false;

    return to_select != Self && to_select->getHandcardNum() > 0;
}

void SKYanmieCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *player = effect.from;
    Room *room = player->getRoom();
    //room->throwCard(effect.card, player);

    ServerPlayer *target = effect.to;
    int card_length = target->getHandcardNum();
    //room->broadcastSkillInvoke("skyanmie", qrand() % 2 + 1);

    CardMoveReason reason;
    reason.m_reason = CardMoveReason::S_REASON_THROW;
    reason.m_playerId = target->objectName();
    reason.m_skillName = "skyanmie";
    room->throwCards(target->getHandcards(), reason, target);
    // room->askForDiscard(target, "skyanmie", card_length, card_length);
    QList<int> cards_ids = room->newDrawCards(target, card_length, "skyanmie");
    room->showHandCards(target, cards_ids);

    int count = 0;
    foreach (int card_id, cards_ids) {
        Card *card = Sanguosha->getCard(card_id);
        if (card->getTypeId() != Card::TypeBasic) {
            count++;
        }
    }

    bool res = false;
    if (count > 0) {
        res = player->askForSkillInvoke("skyanmie", QVariant::fromValue(effect));
    }
    room->clearAG();
    if (res) {
        int damage = 0;
        QList<int> to_discards;
        foreach (int card_id, cards_ids) {
            Card *card = Sanguosha->getCard(card_id);
            if (card->getTypeId() != Card::TypeBasic && target->canDiscard(target, card_id)) {
                to_discards.append(card_id);
                damage++;
            }
        }

        room->throwCards(to_discards, target, player);
        //room->damage(DamageStruct("skyanmie", player, target, damage));
        if (damage > 0)
            room->damage(DamageStruct("skyanmie", player, target, 1));
    }
}

class SKYanmie : public ViewAsSkill
{
public:
    SKYanmie() : ViewAsSkill("skyanmie")
    {
        response_or_use = true;
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.isEmpty() && to_select->getSuit() == Card::Spade && !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.size() != 1) return NULL;

        SKYanmieCard *skcard = new SKYanmieCard;
        skcard->addSubcard(cards.first());
        skcard->setSkillName(objectName());
        return skcard;
    }
};

class SKShunshi : public TriggerSkill
{
public:
    SKShunshi() : TriggerSkill("skshunshi")
    {
        events << TargetConfirming;
        frequency = Skill::NotFrequent;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from == player || room->getAlivePlayers().size() < 3)
            return false;
        if ((use.card->isKindOf("Slash") || use.card->isKindOf("Peach")) &&
            player->askForSkillInvoke(objectName(), data)) {

            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *sp, room->getOtherPlayers(player)) {
                if (sp == use.from)
                    continue;
                targets << sp;
            }
            if (targets.isEmpty())
                return false;

            // this is for ai
            player->tag["SKShunshiGoodValue"] = QVariant::fromValue(use.card->isKindOf("Peach"));
            QList<ServerPlayer *> selected = room->askForPlayersChosen(player, targets, objectName(), "skshunshi-invoke", true, true);
            if (selected.isEmpty())
                return false;

            room->broadcastSkillInvoke(objectName());
            room->drawCards(player, 1, objectName());
            foreach (ServerPlayer *target, selected) {
                room->drawCards(target, 1, objectName());
                use.to.append(target);
            }

            //room->askForUseCard(player, "@@skshunshi", "skshunshi-invoke", -1, Card::MethodNone);
            // room->broadcastSkillInvoke("skshunshi", qrand() % 2 + 1);

            data = QVariant::fromValue(use);
        }
        return false;
    }
};

SKJieyanCard::SKJieyanCard()
{
    will_throw = true;
    target_fixed = true;
    handling_method = Card::MethodDiscard;
}

void SKJieyanCard::use(Room *room, ServerPlayer *player, QList<ServerPlayer *> &) const
{
    CardUseStruct card_use = player->tag["SKJieyanTarget"].value<CardUseStruct>();
    ServerPlayer *target = card_use.to.first();
    room->damage(DamageStruct("skjieyan", player, target, 1, DamageStruct::Fire));
}

class SKJieyanViewAsSkill : public OneCardViewAsSkill
{
public:
    SKJieyanViewAsSkill() : OneCardViewAsSkill("skjieyan")
    {
        response_pattern = "@@skjieyan";
    }

    bool viewFilter(const Card *to_select) const
    {
        return !to_select->isEquipped();
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Card *skcard = new SKJieyanCard;
        skcard->addSubcard(originalCard->getId());
        skcard->setSkillName(objectName());
        return skcard;
    }
};

class SKJieyan : public TriggerSkill
{
public:
    SKJieyan() : TriggerSkill("skjieyan")
    {
        events << TargetSpecifying;
        frequency = Skill::NotFrequent;
        //view_as_skill = new SKJieyanViewAsSkill;
        mustskillowner = false;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        if (triggerEvent == TargetSpecifying) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.to.size() != 1 || !use.card->isRed())
                return false;

            QList<ServerPlayer *> skshenluxuns = room->findPlayersBySkillName("skjieyan");
            if (skshenluxuns.size() == 0)
                return false;

            bool result = false;
            foreach (ServerPlayer *skshenluxun, skshenluxuns) {
                if (skshenluxun->getHandcardNum() == 0)
                    continue;

                if (use.card->isKindOf("Slash") || (use.card->isKindOf("TrickCard") && !use.card->isKindOf("DelayedTrick"))) {
                    if (skshenluxun->askForSkillInvoke(objectName(), data)) {
                        //skshenluxun->tag["SKJieyanTarget"] = data;
                        if (room->askForDiscard(skshenluxun, objectName(), 1, 1, true, false, "skjieyan-invoke", ".|.|.|hand")) {
                            ServerPlayer *target = use.to.first();
                            use.nullified_list << target->objectName();
                            use.to.clear();
                            data = QVariant::fromValue(use);

                            LogMessage log;
                            log.type = "#skjieyan-avoid";
                            log.from = skshenluxun;
                            log.to << target;
                            log.card_str = use.card->toString();
                            log.arg = objectName();
                            room->sendLog(log);

                            room->damage(DamageStruct(objectName(), skshenluxun, target, 1, DamageStruct::Fire));
                            result = true;
                        }
                    }
                }
            }
            return result;
        }

        return false;
    }
};

SKFenyingCard::SKFenyingCard()
{
    will_throw = true;
    target_fixed = false;
    handling_method = Card::MethodDiscard;
}

bool SKFenyingCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty())
        return false;

    ClientPlayer *target = ClientInstance->getPlayer(Self->property("SKFenyingTarget").toString());
    if (!target)
        return false;
    return target->distanceTo(to_select) <= Self->property("SKFenyingDistance").toInt();
    //ClientPlayer *target = ClientInstance->getPlayer(ClientInstance->custom_str2);
    //if (!target || !target->isAlive())
    //    return false;
    //return target->distanceTo(to_select) <= ClientInstance->custom_int2;
}

void SKFenyingCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *player = effect.from;
    player->getRoom()->damage(DamageStruct("skfenying", player, effect.to, player->tag["SKFenyingDamage"].toInt(), DamageStruct::Fire));
}

class SKFenyingViewAsSkill : public OneCardViewAsSkill
{
public:
    SKFenyingViewAsSkill() : OneCardViewAsSkill("skfenying")
    {
        response_pattern = "@@skfenying";
    }

    bool viewFilter(const Card *to_select) const
    {
        return to_select->isRed() && !Self->isJilei(to_select);
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Card *skcard = new SKFenyingCard;
        skcard->addSubcard(originalCard->getId());
        skcard->setSkillName(objectName());
        return skcard;
    }
};

class SKFenying : public TriggerSkill
{
public:
    SKFenying() : TriggerSkill("skfenying")
    {
        events << Damage;
        frequency = Skill::NotFrequent;
        view_as_skill = new SKFenyingViewAsSkill;
        mustskillowner = false;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *, QVariant &data) const
    {
        QList<ServerPlayer *> skshenluxuns = room->findPlayersBySkillName("skfenying");
        if (skshenluxuns.size() == 0)
            return false;

        DamageStruct damage_data = data.value<DamageStruct>();
        if (damage_data.to == NULL || damage_data.nature != DamageStruct::Fire)
            return false;

        foreach (ServerPlayer *skshenluxun, skshenluxuns) {
            if (!skshenluxun->isAlive())
                continue;

            int handcardnum = skshenluxun->getHandcardNum();
            if (handcardnum > skshenluxun->getMaxHp())
                continue;

            if (handcardnum == 0) {
                QList<const Card *> equips = skshenluxun->getEquips();
                bool redequip = false;
                foreach (const Card * eq, equips) {
                    if (eq->isRed()) {
                        redequip = true;
                        break;
                    }
                }
                if (!redequip)
                    continue;
            }

            ServerPlayer *sk_fenying_target = damage_data.to;
            int sk_fenying_distance = -1;
            foreach (ServerPlayer * p, room->getOtherPlayers(sk_fenying_target, false)) {
                int dist = sk_fenying_target->distanceTo(p);
                if (sk_fenying_distance < 0)
                    sk_fenying_distance = dist;
                else if (sk_fenying_distance > dist)
                    sk_fenying_distance = dist;
            }

            room->setPlayerProperty(skshenluxun, "SKFenyingTarget", QVariant::fromValue(sk_fenying_target->objectName()));
            room->setPlayerProperty(skshenluxun, "SKFenyingDistance", QVariant::fromValue(sk_fenying_distance));
            skshenluxun->tag["SKFenyingDamage"] = QVariant::fromValue(damage_data.damage);

            if (room->askForUseCard(skshenluxun, "@@skfenying", "skfenying-invoke", -1, Card::MethodDiscard)) {
                room->notifySkillInvoked(skshenluxun, objectName());
            }
            skshenluxun->tag.remove("SKFenyingDamage");
        }

        return false;
    }
};

SKQianqiCard::SKQianqiCard()
{
}

bool SKQianqiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QString tlist = Self->property("skqianqi_targets").toString();
    return targets.isEmpty() && to_select != Self && Self->canSlash(to_select, false) && !tlist.contains(to_select->objectName());
}

void SKQianqiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    //room->broadcastSkillInvoke("skqianqi", (qrand() % 2) + 3);
    //room->notifySkillInvoked(effect.from, "skqianqi");
    effect.from->loseMark("@skqianqi", 1);
    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->setSkillName("skqianqi");
    slash->deleteLater();

    QStringList tlist = effect.from->property("skqianqi_targets").toString().split('+');
    tlist << effect.to->objectName();
    room->setPlayerProperty(effect.from, "skqianqi_targets", tlist.join('+'));
    room->useCard(CardUseStruct(slash, effect.from, effect.to));
}


class SKQianqi : public ZeroCardViewAsSkill
{
public:
    SKQianqi() : ZeroCardViewAsSkill("skqianqi")
    {
        response_or_use = true;
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return qrand() % 2 + 1;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@skqianqi") > 0;
    }

    const Card *viewAs() const
    {
        Card *skcard = new SKQianqiCard;
        skcard->setSkillName(objectName());
        return skcard;
    }
};

class SKQianqiEffect : public TriggerSkill
{
public:
    SKQianqiEffect() : TriggerSkill("#skqianqieffect")
    {
        events << CardsMoveOneTime << GameStart << EventPhaseStart;
        frequency = Compulsory;
        mustskillowner = false;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime)
        {
            QList<ServerPlayer *> skshenmachaos = room->findPlayersBySkillName("skqianqi");
            if (skshenmachaos.size() == 0)
                return false;

            int count = 0;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

            if (move.from == player && move.from_places.contains(Player::PlaceEquip)) {
                for (int i = 0; i < move.card_ids.size(); i++) {
                    if (!player->isAlive())
                        return false;
                    if (move.from_places[i] == Player::PlaceEquip && Sanguosha->getCard(move.card_ids[i])->isKindOf("Horse")) {
                        count++;
                    }
                }
            }

            /*
            if (move.to == player && move.to_place == Player::PlaceEquip) {
                for (int i = 0; i < move.card_ids.size(); i++) {
                    if (Sanguosha->getCard(move.card_ids[i])->isKindOf("Horse")) {
                        count++;
                    }
                }
            }
            */

            if (count > 0)
            {
                foreach (ServerPlayer * skshenmachao, skshenmachaos) {
                    if (!skshenmachao->isAlive())
                        continue;

                    skshenmachao->gainMark("@skqianqi", 2 * count);
                    room->broadcastSkillInvoke("skqianqi", (qrand() % 2) + 3);
                    room->notifySkillInvoked(skshenmachao, "skqianqi");
                }
            }
        }
        else if (triggerEvent == EventPhaseStart)
        {
            if (player->getPhase() == Player::Play) {
                QList<ServerPlayer *> skshenmachaos = room->findPlayersBySkillName("skqianqi");
                foreach (ServerPlayer * skshenmachao, skshenmachaos) {
                    if (!skshenmachao->isAlive())
                        continue;

                    room->setPlayerProperty(skshenmachao, "skqianqi_targets", QString());
                }
            }
        }
        else if (player->hasSkill("skqianqi"))
        {
            room->broadcastSkillInvoke("skqianqi", (qrand() % 2) + 3);
            room->installEquip(player, "@OffensiveHorse");
        }
        return false;
    }
};

class SKJuechen : public TriggerSkill
{
public:
    SKJuechen() : TriggerSkill("skjuechen")
    {
        events << TargetSpecified << ConfirmDamage << EventPhaseStart;
        frequency = Skill::NotFrequent;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;

            foreach (ServerPlayer *target, use.to)
            {
                if (!target->isAlive() || target->hasFlag("SKJuechenUsed"))
                    continue;

                bool res1 = false, res2 = false;
                if (target->getArmor() != NULL || target->getHandcardNum() >= player->getHandcardNum())
                {
                    res1 = true;
                }
                if (target->getDefensiveHorse() != NULL || target->getOffensiveHorse() != NULL || target->getHp() >= player->getHp())
                {
                    res2 = true;
                }
                if ((res1 || res2) && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(target)))
                {
                    room->setPlayerFlag(target, "SKJuechenUsed");
                    if (res1 && !room->askForDiscard(target, objectName(), 1, 1, true, true, "skjuechen-invoke1", "EquipCard|.|.|hand,equipped"))
                    {
                        room->setTargetNoJink(player, target, use);
                    }
                    // for ai.
                    room->setPlayerFlag(target, "SKJuechenSecond");
                    if (res2 && !room->askForDiscard(target, objectName(), 1, 1, true, true, "skjuechen-invoke2", "EquipCard|.|.|hand,equipped"))
                    {
                        target->tag["SKJuechenVulnerable"] = QVariant(use.card->toString());
                    }
                    room->setPlayerFlag(target, "-SKJuechenSecond");
                }
            }

        } else if (triggerEvent == ConfirmDamage) {

            DamageStruct damage = data.value<DamageStruct>();
            if (damage.chain || damage.transfer
                || !damage.card || !damage.card->isKindOf("Slash") || !damage.to->tag["SKJuechenVulnerable"].canConvert<QString>()
                || damage.to->tag["SKJuechenVulnerable"].toString() != damage.card->toString())
                return false;

            damage.to->tag.remove("SKJuechenVulnerable");

            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#skjuechen-log";
            log.from = player;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);
            data = QVariant::fromValue(damage);
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::RoundStart) {
                foreach (ServerPlayer *target, room->getAllPlayers(false)) {
                    room->setPlayerFlag(target, "-SKJuechenUsed");
                }
            }
        }
        return false;
    }
};

class SKLuosha : public TriggerSkill
{
public:
    SKLuosha() : TriggerSkill("skluosha")
    {
        events << EnterDying << GameStart;
        frequency = Compulsory;
        mustskillowner = false;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QList<ServerPlayer *> skshenlvbus = room->findPlayersBySkillName(objectName());
        if (skshenlvbus.size() == 0)
            return false;

        if (triggerEvent == EnterDying)
        {
            DyingStruct dying = data.value<DyingStruct>();
            QList<const Skill *> skills = room->searchSkillsWithText("【杀】");
            qShuffle(skills);
            int skill_i = 0;
            foreach (ServerPlayer * skshenlvbu, skshenlvbus) {
                if (!skshenlvbu->isAlive() || dying.who == skshenlvbu)
                    continue;
                auto list = skshenlvbu->tag[objectName()].value<QStringList>();
                if (list.contains(dying.who->objectName()))
                    continue;

                list.append(dying.who->objectName());
                skshenlvbu->tag[objectName()] = QVariant::fromValue(list);
                room->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(skshenlvbu, objectName());
                room->drawCards(skshenlvbu, 1, objectName());
                if (skill_i < skills.size()) {
                    room->handleAcquireDetachSkills(skshenlvbu, skills[skill_i]->objectName(), true);
                    ++skill_i;
                }
            }
        }
        else if (player->hasSkill(objectName()))
        {
            QList<const Skill *> skills = room->searchSkillsWithText("【杀】");
            qShuffle(skills);
            int skill_i = 0;
            foreach (ServerPlayer * skshenlvbu, skshenlvbus) {
                if (!skshenlvbu->isAlive())
                    continue;

                room->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(skshenlvbu, objectName());
                for (int i = 0; i < 3; ++i) {
                    if (skill_i < skills.size()) {
                        room->handleAcquireDetachSkills(skshenlvbu, skills[skill_i]->objectName(), true);
                        ++skill_i;
                    }
                }
            }
        }
        return false;
    }
};

inline void SKShajueFunction(ServerPlayer *player, ServerPlayer *target)
{
    Room *room = player->getRoom();
    room->loseHp(player, 1);
    int count = player->getHandcardNum();
    room->throwCards(player->getHandcards(), CardMoveReason(CardMoveReason::S_REASON_DISCARD, player->objectName(), "skshajue", QString()), player, NULL);

    QList<int> ids = room->showCardsOfDrawPile(player, count, "skshajue");
    QList<int> disabledids;
    QList<const Card *> cards;
    foreach (int id, ids) {
        const Card *card = Sanguosha->getCard(id);
        if (card->isKindOf("BasicCard") || card->isKindOf("EquipCard")) {
            cards << card;
        } else {
            disabledids << id;
        }
    }

    room->fillAG(ids, NULL, disabledids);

    room->setArmorNullified(target, true);
    foreach (const Card *card, cards) {
        if (!target->isAlive())
            break;

        Slash *slash;
        switch (qrand() % 3) {
        case 0:
            slash = new Slash(card->getSuit(), card->getNumber());
            break;
        case 1:
            slash = new ThunderSlash(card->getSuit(), card->getNumber());
            break;
        default:
            slash = new FireSlash(card->getSuit(), card->getNumber());
            break;
        }
        slash->setSkillName("skshajue");
        slash->addSubcard(card);
        room->useCard(CardUseStruct(slash, player, target), true);
        room->delay();
    }
    room->setArmorNullified(target, false);
    room->clearAG();

    CardsMoveStruct discardmove(disabledids, NULL, Player::DiscardPile, CardMoveReason(CardMoveReason::S_REASON_DISCARD, player->objectName(), "skshajue", QString()));
    room->moveCardsAtomic(discardmove, true);
}

SKShajueCard::SKShajueCard()
{
}

bool SKShajueCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && Self->canSlash(to_select, false);
}

void SKShajueCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *player = effect.from;
    ServerPlayer *target = effect.to;

    SKShajueFunction(player, target);
}

//class SKShajue : public TriggerSkill
//{
//public:
//    SKShajue() : TriggerSkill("skshajue")
//    {
//        events << EventPhaseStart;
//        frequency = Skill::NotFrequent;
//    }
//
//    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
//    {
//        if (player->getPhase() == Player::Play && player->getHandcardNum() > 0) {
//            QList<ServerPlayer *> targets;
//            foreach (ServerPlayer *p, room->getOtherPlayers(player, false)) {
//                if (player->canSlash(p, false))
//                    targets << p;
//            }
//            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "skshajue-invoke", true, true);
//            if (target && target->isAlive()) {
//                SKShajueFunction(player, target);
//            }
//        }
//        return false;
//    }
//};

class SKShajue : public ZeroCardViewAsSkill
{
public:
    SKShajue() : ZeroCardViewAsSkill("skshajue")
    {

    }

    bool isEnabledAtPlay(const Player *player) const
    {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->deleteLater();
        return !player->hasUsed("SKShajueCard") && !player->isLocked(slash, false);
    }

    const Card *viewAs() const
    {
        Card *skcard = new SKShajueCard;
        skcard->setSkillName(objectName());
        return skcard;
    }
};

SKGuiquCard::SKGuiquCard()
{
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void SKGuiquCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *player = effect.from;
    Room *room = player->getRoom();

    QStringList skill_names;
    QString skill_name_choose;

    QList<const Skill *> skills = player->getVisibleSkillList();
    foreach (const Skill *skill, skills) {
        if (skill->isDetachReasonable())
            skill_names << skill->objectName();
    }

    if (!skill_names.isEmpty()) {
        skill_name_choose = room->askForChoice(player, "skguiqu", skill_names.join("+"));
        if (player->hasSkill(skill_name_choose)) {
            room->detachSkillFromPlayer(player, skill_name_choose, false, false);
            Card *peach = Sanguosha->cloneCard("peach");
            peach->setSkillName("skguiqu");
            room->useCard(CardUseStruct(peach, player, player, true), true);
        }
    }
}

class SKGuiqu : public ZeroCardViewAsSkill
{
public:
    SKGuiqu() : ZeroCardViewAsSkill("skguiqu")
    {
        response_pattern = "peach";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        Card *peach = Sanguosha->cloneCard("peach");
        peach->deleteLater();
        return player->isDying() && !player->isLocked(peach, false);
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern.contains("peach") && player->isDying();
    }

    const Card *viewAs() const
    {
        Card *skcard = new SKGuiquCard;
        skcard->setSkillName(objectName());
        return skcard;
    }
};

class SKGuiquMaxCards : public MaxCardsSkill
{
public:
    SKGuiquMaxCards() : MaxCardsSkill("#skguiqu-mx")
    {
    }

    int getFixed(const Player *target) const
    {
        if (target->hasSkill("skguiqu"))
            return target->getSkillList(false, true).size();
        else
            return -1;
    }
};

class SKYuanhua : public TriggerSkill
{
public:
    SKYuanhua() :TriggerSkill("skyuanhua")
    {
        events << CardsMoveOneTime;
        frequency = Skill::Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to == player && move.to_place == Player::PlaceHand) {
            bool res = false;
            foreach (int card_id, move.card_ids) {
                Card *card = room->getCard(card_id);
                if (!card->isKindOf("Peach"))
                    continue;

                if (player->getHp() < player->getMaxHp()) {
                    room->recover(player, RecoverStruct());
                } else {
                    room->drawCards(player, 2, objectName());
                }
                player->addToPile("skyuanhua", card);
                res = true;
            }

            if (res)
                room->broadcastSkillInvoke(objectName());
        }

        return false;
    }
};

SKGuiyuanCard::SKGuiyuanCard()
{
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void SKGuiyuanCard::use(Room *room, ServerPlayer *player, QList<ServerPlayer *> &) const
{
    room->loseHp(player, 1);
    if (!player->isAlive())
        return;

    QList<ServerPlayer *> others = room->getOtherPlayers(player, false);
    bool res = false;
    foreach (ServerPlayer *p, others) {
        if (p->isAlive()) {
            const Card *card = room->askForExchange(p, "skguiyuan", 1, 1, false, QString(), false, "%peach");
            if (card != NULL){
                res = true;
                CardMoveReason movereason(CardMoveReason::S_REASON_GIVE, player->objectName(), p->objectName(), "skguiyuan", QString());
                room->moveCardTo(card, p, player, Player::PlaceHand, movereason, true);
            }
        }
    }
    if (!res) {
        int card_id = room->newGetCardFromPile("peach", true);
        if (card_id >= 0)
        {
            CardMoveReason reason(CardMoveReason::S_REASON_GOTCARD, player->objectName(), "skguiyuan", QString());
            room->moveCardTo(room->getCard(card_id), player, Player::PlaceHand, reason, true);
        } else {
            LogMessage log;
            log.type = "#skguiyuan-negative";
            room->sendLog(log);
        }
    }
}

class SKGuiyuan : public ZeroCardViewAsSkill
{
public:
    SKGuiyuan() : ZeroCardViewAsSkill("skguiyuan")
    {
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("SKGuiyuanCard");
    }

    const Card *viewAs() const
    {
        Card *skcard = new SKGuiyuanCard;
        skcard->setSkillName(objectName());
        return skcard;
    }
};

class SKChongsheng : public TriggerSkill
{
public:
    SKChongsheng() : TriggerSkill("skchongsheng")
    {
        events << EnterDying;
        frequency = Limited;
        limit_mark = "@skchongsheng";
        mustskillowner = false;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DyingStruct dying_data = data.value<DyingStruct>();
        QList<ServerPlayer *> sks = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *p, sks) {
            if (!p->isAlive()) continue;
            if (p->getMark("@skchongsheng") > 0) {
                if (p->askForSkillInvoke(objectName(), QVariant::fromValue(dying_data.who))) {
                    room->broadcastSkillInvoke(objectName());
                    room->doSuperLightbox("sk_shenhuatuo", objectName());
                    room->removePlayerMark(p, "@skchongsheng");

                    int rec = 1;
                    if (p->hasPile("skyuanhua")) {
                        rec = std::max(rec, p->getPile("skyuanhua").length());
                    }
                    int draw = 0;
                    int maxrec = player->getMaxHp() - player->getHp();
                    if (rec > maxrec) {
                        draw = rec - maxrec;
                        rec = maxrec;
                    }
                    room->recover(player, RecoverStruct(p, NULL, rec));
                    if (draw > 0) {
                        player->drawCards(draw, objectName());
                    }

                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
                    room->throwCards(p->getPile("skyuanhua"), reason, NULL);

                    if (room->askForSimpleChoice(player, objectName(), "invoke", QVariant::fromValue(QString("ASK_FOR_CHOICE")))) {
                        QList<ServerPlayer *> alvplayers = room->getAlivePlayers();
                        QSet<QString> general_names;
                        foreach(ServerPlayer *avp, alvplayers)
                            general_names << avp->getGeneralName() << avp->getGeneral2Name();

                        QStringList all_generals = Sanguosha->getLimitedGeneralNames();
                        QStringList generals;
                        foreach (QString name, all_generals) {
                            const General *general = Sanguosha->getGeneral(name);
                            if (general->getKingdom() == player->getKingdom() && !general_names.contains(name))
                                generals << name;
                        }

                        qShuffle(generals);
                        all_generals.clear();
                        for (int i = 0; i < std::min(3, generals.length()); ++i) {
                            all_generals << generals[i];
                        }
                        if (all_generals.length() > 0) {
                            QString general = room->askForGeneral(player, all_generals);
                            player->tag["newgeneral"] = general;
                            room->changeHero(player, general, false, true);
                        }
                    }

                    break;
                }
            }
        }
        return false;
    }
};

class SKYaozhiEffect : public TriggerSkill
{
public:
    SKYaozhiEffect() : TriggerSkill("#skyaozhi-effect")
    {
        events << EventPhaseEnd << DamageComplete;
        frequency = Skill::Compulsory;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
        return triggerEvent == EventPhaseEnd ? 1000 : 0;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        bool removeflag = false;
        if (triggerEvent == EventPhaseEnd) {
            if (player->getPhase() == Player::Start || player->getPhase() == Player::Play || player->getPhase() == Player::Finish) {
                removeflag = true;
            }
        } else {
            removeflag = true;
        }

        if (removeflag) {
            QStringList skill_1 = player->tag.value("skyaozhi_acquire", QStringList()).toStringList();
            foreach (auto s, skill_1) {
                if (player->hasSkill(s, true))
                    room->detachSkillFromPlayer(player, s, false, true);
            }
            player->tag["skyaozhi_acquire"].clear();
        }

        return false;
    }
};

class SKYaozhi : public TriggerSkill
{
public:
    SKYaozhi() : TriggerSkill("skyaozhi"), skillCount(3)
    {
        events << Damaged << EventPhaseChanging << EventPhaseStart;
    }

    const int skillCount;

    int getPriority(TriggerEvent ) const
    {
        return 1000;
    }

    void filterSkills(QList<const TriggerSkill *> &triggerSkills, const QString &skills_whitelist) const
    {
        QStringList skilllist = skills_whitelist.split("|");
        for (int i = 0; i < triggerSkills.length(); )
        {
            if (!skilllist.contains(triggerSkills[i]->objectName())) {
                triggerSkills.removeAt(i);
            } else {
                ++i;
            }
        }
    }

    inline QStringList getSkillTypeSkills(QString &skills, Room *room, Skill::SkillType type) const
    {
        auto skills_split = skills.split("|");
        auto exskills = room->searchSkillsWithSkillType(type, true, false, false);
        foreach (auto s, exskills) {
            if (skills_split.contains(s->objectName()))
                continue;
            skills_split << s->objectName();
        }
        return skills_split;
    }

    QStringList getSkillsInStart(Room *room) const
    {
        QString skills = "luoshen|tishen|guanxing|qinxue|zuixiang|shichou|shenzhi|tianfu|zhiji|ruoyu|hunzi|danji|juyi|yinghun|kegou|wuling|dongcha|qianxi|dangxian|xiansi|skwengua";
        return getSkillTypeSkills(skills, room, Skill::PhaseSkill_RoundStart);
    }

    QStringList getSkillsInEnd(Room *room) const
    {
        QString skills = "biyue|kuiwei|shude|yaojiazi|lianpo|huyuan|heyi|qiluan|fenming|neojushou|zhulou|spzhulou|yuanhu|wuji|bifa|mumu|shefu|junbing|jieyue|fentian|moshi|kunfen|jushou|\
chouliang|longluo|caizhaoji_hujia|gongmou|miji|zhiyan|juece|bingyi|youdi|xingxue|lizhan";
        return getSkillTypeSkills(skills, room, Skill::PhaseSkill_RoundEnd);
    }

    QStringList getSkillsInDamaged(Room *room) const
    {
        QString skills = "jianxiong|fankui|ganglie|yiji|wangxi|fenyong|langgu|jieming|guixin|mingshi|hengjiang|neoganglie|skfulu|jilei|benyu|fangzhu|tanlan|\
ytchengxiang|toudu|zhiyu|chengxiang|yuce|duodao|huituo|enyuan|zhichi|quanji|olex_yuqi|shiyong";
        return getSkillTypeSkills(skills, room, Skill::TriggerSkill_Damaged);
    }

    QStringList getSkillsInPlay(Room *room) const
    {
        QString skills = "zhishi|yijue|jianyan|zhiheng|fanjian|kurou|guose|jieyin|chuli|lijian|duyi|lihun|dahe|tanhu|fuluan|quhu|tianyi|gongxin|fenxun|shangyi|duanxie|\
skguiyuan|neoluoyi|neofanjian|tiaoxin|qingnang|sksijian|skziguo|xueji|qiangwu|xiemu|quji|sanyao|olmumu|dimeng|luanwu|jiuchi|houyuan|jueji|guihan|lexue|ytyishe|ytmidao|ytpudu|qice|anxu|\
gongqi|jiefan|junxing|mieji|fencheng|xianzhou|zenhui|taoxi|huaiyi|yanzhu|furong|anguo|ganlu|xianzhen|mingce|skfuzhu|weikui|sanfen|sktaoluan";
        return getSkillTypeSkills(skills, room, Skill::PhaseSkill_Play_OncePerPhase);
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QStringList skills2;
        bool isdamaged = false;

        if (triggerEvent == Damaged) {
            if (player->tag["skyaozhi_trigger"].toBool()) {
                player->tag["skyaozhi_trigger"] = QVariant::fromValue(false);
                return false;
            }
            //skills = room->searchTriggerSkillsWithEvent(Damaged, true, true, true);
            skills2 = this->getSkillsInDamaged(room);
            isdamaged = true;
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::RoundStart) {
                skills2 = this->getSkillsInStart(room);
            } else if (change.to == Player::Finish) {
                skills2 = this->getSkillsInEnd(room);
            }
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Play) {
                skills2 = this->getSkillsInPlay(room);
            }
        }

        QStringList skills;
        foreach (auto s, skills2) {
            if (!player->hasSkill(s, true))
                skills << s;
        }

        if (skills.length() < 1)
            return false;

        if (player->askForSkillInvoke(objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            // room->drawCards(player, 1, objectName());
            QStringList skill_names;
            QString skill_name;
            int count = 0;

            qShuffle(skills);
            foreach (QString skill, skills) {
                //const Skill *tskill = Sanguosha->getSkill(skill);
                skill_names << skill;// tskill->objectName();
                if (++count >= skillCount)
                    break;
            }

            if (!skill_names.isEmpty()) {
                skill_name = room->askForChoice(player, objectName(), skill_names.join("+"));
                if (skill_names.contains(skill_name)) {
                    room->handleAcquireDetachSkills(player, skill_name);
                    QStringList skill_history = player->tag["skyaozhi_history"].toStringList();
                    if (!skill_history.contains(skill_name)) {
                        skill_history << skill_name;
                        player->tag["skyaozhi_history"] = QVariant::fromValue(skill_history);
                    }
                    QStringList skill_acquire = player->tag["skyaozhi_acquire"].value<QStringList>();
                    skill_acquire << skill_name;
                    player->tag["skyaozhi_acquire"] = QVariant::fromValue(skill_acquire);
                    if (isdamaged) {
                        player->tag["skyaozhi_trigger"] = QVariant::fromValue(true);
                        room->getThread()->trigger(Damaged, room, player, data);
                        return true;
                    }
                }
            }
        }

        return false;
    }
};

class SKXingyun : public TriggerSkill
{
public:
    SKXingyun() : TriggerSkill("skxingyun")
    {
        events << EventPhaseChanging;
        frequency = Skill::Compulsory;
    }

    void filterSkills(QStringList &skills, ServerPlayer *player) const
    {
        for (int i = 0; i < skills.length(); ) {
            const Skill *skill = Sanguosha->getSkill(skills[i]);
            if (!skill || player->hasSkill(skill)) {
                skills.removeAt(i);
            } else {
                ++i;
            }
        }
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.from != Player::Finish) {
            return false;
        }
        //if (player->getPhase() != Player::Finish) {
        //    return false;
        //}

        room->notifySkillInvoked(player, objectName());
        room->broadcastSkillInvoke(objectName());
        room->loseMaxHp(player, 1);

        QStringList skills = player->tag["skyaozhi_history"].toStringList();
        this->filterSkills(skills, player);
        if (skills.length() < 1)
            return false;

        QString skill_name = room->askForChoice(player, objectName(), skills.join("+"));
        if (skills.contains(skill_name)) {
            skills.removeAll(skill_name);
            room->acquireSkill(player, skill_name);
            player->tag["skyaozhi_history"] = QVariant::fromValue(skills);
        }

        return false;
    }
};

class SKlvezhen : public TriggerSkill
{
public:
    SKlvezhen() : TriggerSkill("sklvezhen")
    {
        events << TargetSpecified;
        frequency = Skill::NotFrequent;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card || !use.card->isKindOf("Slash"))
            return false;

        foreach (ServerPlayer *target, use.to)
        {
            if (!target->isAlive())
                continue;

            if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(target)))
            {
                room->broadcastSkillInvoke(objectName());
                QList<int> ids = room->showCardsOfDrawPile(player, 3, objectName(), true);
                DummyCard *dummy = new DummyCard;
                int count = 0;
                foreach (int id, ids) {
                    if (!Sanguosha->getCard(id)->isKindOf("BasicCard")) {
                        ++count;
                        dummy->addSubcard(id);
                    }
                }

                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
                room->throwCard(dummy, reason, NULL);
                delete dummy;

                if (count > 0)
                    room->askForThrow(player, target, count, "he", objectName());
            }
        }
        return false;
    }
};

class SKYoulongDist : public TargetModSkill
{
public:
    SKYoulongDist() : TargetModSkill("#skyoulong-dist")
    {
        pattern = "Snatch";
    }

    int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->hasSkill("skyoulong"))
            return 1000;
        else
            return 0;
    }
};

class SKYoulongViewAsSkill : public ZeroCardViewAsSkill
{
public:
    SKYoulongViewAsSkill() : ZeroCardViewAsSkill("skyoulong")
    {
        response_pattern = "@@skyoulong";
    }

    const Card *viewAs() const
    {
        auto card = new Snatch(Card::NoSuit, 0);
        card->setSkillName("skyoulong");
        return card;
    }
};

class SKYoulong : public TriggerSkill
{
public:
    SKYoulong() : TriggerSkill("skyoulong")
    {
        events << EventPhaseEnd;
        frequency = Skill::NotFrequent;
        view_as_skill = new SKYoulongViewAsSkill;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        int num = 0;
        foreach (auto p, room->getOtherPlayers(player)) {
            num += player->getHistory(QString("throwCard:%1").arg(p->objectName()));
            if (num >= 2) break;
        }
        if (num >= 2 && room->askForSkillInvoke(player, objectName())) {
            room->broadcastSkillInvoke(objectName());
            room->askForUseCard(player, "@@skyoulong", "#skyoulong-invoke");
        }
        /*
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from && move.from != player && move.to_place == Player::DiscardPile
            && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            DummyCard *dummy = new DummyCard;
            foreach (int card_id, move.card_ids) {
                const Card *card = Sanguosha->getCard(card_id);
                if (card && !card->isKindOf("BasicCard")) {
                    dummy->addSubcard(card);
                }
            }
            dummy->deleteLater();
            if (dummy->getSubcards().length() == 0)
                return false;

            if (room->askForSkillInvoke(player, objectName())) {
                room->broadcastSkillInvoke(objectName());
                CardMoveReason reason(CardMoveReason::S_REASON_RECYCLE, player->objectName());
                room->moveCardTo(dummy, NULL, player, Player::PlaceHand, reason, true);
            }
        }
        */
        return false;
    }
};


#define SKSHENFU_ROUNDLIMIT
class SKShenfu : public TriggerSkill
{
public:
    SKShenfu() : TriggerSkill("skshenfu")
    {
#ifdef SKSHENFU_ROUNDLIMIT
        events << CardsMoveOneTime << EventPhaseStart;
        frequency = Skill::NotFrequent;
        mustskillowner = false;
#else
        events << CardsMoveOneTime;
        frequency = Skill::Frequent;
#endif
    }

#ifdef SKSHENFU_ROUNDLIMIT
    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
#else
    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
#endif
    {
#ifdef SKSHENFU_ROUNDLIMIT
        if (triggerEvent == EventPhaseStart) {
            //if (player && player->getPhase() == Player::RoundStart) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    p->tag["skshenfu"] = false;
                }
            //}
        } else {

#endif
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!move.from || move.from != player || !player->hasSkill(this))
                return false;
            player = room->findPlayerByObjectName(move.from->objectName());
            if (player
#ifdef SKSHENFU_ROUNDLIMIT
                && !player->tag["skshenfu"].toBool()
#endif
                && move.from_places.contains(Player::PlaceHand) && player->getHandcardNum() < 3) {
                if (room->askForSkillInvoke(player, objectName())) {
#ifdef SKSHENFU_ROUNDLIMIT
                    player->tag["skshenfu"] = true;
#endif
                    room->broadcastSkillInvoke(objectName());
                    room->drawCards(player, 3 - player->getHandcardNum(), objectName());
                    Card::Suit suit = Card::NoSuit;
                    foreach (const Card *card, player->getHandcards()) {
                        if (suit >= Card::NoSuitBlack) {
                            suit = card->getSuit();
                        } else if (card->getSuit() != suit) {
                            return false;
                        }
                    }

                    ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), objectName(), "skshenfu-i", true);
                    if (target && target->isAlive()) {
                        room->notifySkillInvoked(player, objectName());
                        room->broadcastSkillInvoke(objectName());
                        int x = player->getHandcardNum();
                        player->throwAllHandCards();
                        room->drawCards(player, x, objectName());
                        room->damage(DamageStruct(objectName(), player, target, 3));
                    }
                }
            }
#ifdef SKSHENFU_ROUNDLIMIT
        }
#endif
        return false;
    }
};

HunLiePackage::HunLiePackage() : Package("hunlie")
{
    // generals
    General *sk_shenjiaxu = new General(this, "sk_shenjiaxu", "god", 3);
    sk_shenjiaxu->addSkill(new SKYanmie);
    sk_shenjiaxu->addSkill(new SKShunshi);

    General *sk_shenluxun = new General(this, "sk_shenluxun", "god", 3);
    sk_shenluxun->addSkill(new SKJieyan);
    sk_shenluxun->addSkill(new SKFenying);

    General *sk_shenmachao = new General(this, "sk_shenmachao", "god", 4);
    sk_shenmachao->addSkill(new SKQianqi);
    sk_shenmachao->addSkill(new SKQianqiEffect);
    related_skills.insertMulti("skqianqi", "#skqianqieffect");
    sk_shenmachao->addSkill(new SKJuechen);

    General *sk_shenlvbu = new General(this, "sk_shenlvbu", "god", 2);
    sk_shenlvbu->addSkill(new SKLuosha);
    sk_shenlvbu->addSkill(new SKShajue);
    sk_shenlvbu->addSkill(new SKGuiqu);
    sk_shenlvbu->addSkill(new SKGuiquMaxCards);
    related_skills.insertMulti("skguiqu", "#skguiqu-mx");

    General *sk_shenhuatuo = new General(this, "sk_shenhuatuo", "god", 3);
    sk_shenhuatuo->addSkill(new SKYuanhua);
    sk_shenhuatuo->addSkill(new SKGuiyuan);
    sk_shenhuatuo->addSkill(new SKChongsheng);

    General *sk_spshenzhuge = new General(this, "sk_spshenzhuge", "god", 7, true, false, false, 3);
    sk_spshenzhuge->addSkill(new SKYaozhi);
    sk_spshenzhuge->addSkill(new SKYaozhiEffect);
    sk_spshenzhuge->addSkill(new SKXingyun);
    related_skills.insertMulti("skyaozhi", "#skyaozhi-effect");

    General *sk_shenganning = new General(this, "sk_shenganning", "god", 4);
    sk_shenganning->addSkill(new SKlvezhen);
    sk_shenganning->addSkill(new SKYoulong);
    sk_shenganning->addSkill(new SKYoulongDist);
    related_skills.insertMulti("skyoulong", "#skyoulong-dist");

    General *sk_shenzhenji = new General(this, "sk_shenzhenji", "god", 3, false);
    sk_shenzhenji->addSkill(new SKShenfu);

    addMetaObject<SKYanmieCard>();
    //addMetaObject<SKShunshiCard>();
    addMetaObject<SKJieyanCard>();
    addMetaObject<SKFenyingCard>();
    addMetaObject<SKQianqiCard>();
    addMetaObject<SKShajueCard>();
    addMetaObject<SKGuiquCard>();
    addMetaObject<SKGuiyuanCard>();
}



ADD_PACKAGE(HunLie)
