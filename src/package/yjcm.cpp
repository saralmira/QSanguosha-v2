#include "yjcm.h"
#include "skill.h"
#include "standard.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "engine.h"
#include "settings.h"
#include "ai.h"
#include "general.h"
#include "util.h"
#include "wrapped-card.h"
#include "room.h"
#include "roomthread.h"

class Yizhong : public TriggerSkill
{
public:
    Yizhong() : TriggerSkill("yizhong")
    {
        events << SlashEffected;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && TriggerSkill::triggerable(target) && target->getArmor() == NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.slash->isBlack()) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#SkillNullify";
            log.from = player;
            log.arg = objectName();
            log.arg2 = effect.slash->objectName();
            room->sendLog(log);

            return true;
        }
        return false;
    }
};

class Luoying : public TriggerSkill
{
public:
    Luoying() : TriggerSkill("luoying")
    {
        events << BeforeCardsMove;
        frequency = Frequent;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *caozhi, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == caozhi || move.from == NULL)
            return false;
        if (move.to_place == Player::DiscardPile
            && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD
            || move.reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE)) {
            QList<int> card_ids;
            int i = 0;
            foreach (int card_id, move.card_ids) {
                if (Sanguosha->getCard(card_id)->getSuit() == Card::Club
                    && ((move.reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE
                    && move.from_places[i] == Player::PlaceJudge
                    && move.to_place == Player::DiscardPile)
                    || (move.reason.m_reason != CardMoveReason::S_REASON_JUDGEDONE
                    && room->getCardOwner(card_id) == move.from
                    && (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip))))
                    card_ids << card_id;
                i++;
            }
            if (card_ids.isEmpty())
                return false;
            else if (caozhi->askForSkillInvoke(this, data)) {
                int ai_delay = Config.AIDelay;
                Config.AIDelay = 0;
                while (card_ids.length() > 1) {
                    room->fillAG(card_ids, caozhi);
                    int id = room->askForAG(caozhi, card_ids, true, objectName());
                    if (id == -1) {
                        room->clearAG(caozhi);
                        break;
                    }
                    card_ids.removeOne(id);
                    room->clearAG(caozhi);
                }
                Config.AIDelay = ai_delay;

                if (!card_ids.isEmpty()) {
                    room->broadcastSkillInvoke("luoying");
                    move.removeCardIds(card_ids);
                    data = QVariant::fromValue(move);
                    DummyCard *dummy = new DummyCard(card_ids);
                    room->moveCardTo(dummy, caozhi, Player::PlaceHand, move.reason, true);
                    delete dummy;
                }
            }
        }
        return false;
    }
};

class Jiushi : public ZeroCardViewAsSkill
{
public:
    Jiushi() : ZeroCardViewAsSkill("jiushi")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return Analeptic::IsAvailable(player) && player->faceUp();
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern.contains("analeptic") && player->faceUp();
    }

    const Card *viewAs() const
    {
        Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
        analeptic->setSkillName(objectName());
        return analeptic;
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return qrand() % 2 + 1;
    }
};

class JiushiFlip : public TriggerSkill
{
public:
    JiushiFlip() : TriggerSkill("#jiushi-flip")
    {
        events << PreCardUsed << PreDamageDone << DamageComplete;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == "jiushi")
                player->turnOver();
        } else if (triggerEvent == PreDamageDone) {
            player->tag["PredamagedFace"] = !player->faceUp();
        } else if (triggerEvent == DamageComplete) {
            bool facedown = player->tag.value("PredamagedFace").toBool();
            player->tag.remove("PredamagedFace");
            if (facedown && !player->faceUp() && player->askForSkillInvoke("jiushi", data)) {
                room->broadcastSkillInvoke("jiushi", 3);
                player->turnOver();
            }
        }

        return false;
    }
};

class Wuyan : public TriggerSkill
{
public:
    Wuyan() : TriggerSkill("wuyan")
    {
        events << DamageCaused << DamageInflicted;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->getTypeId() == Card::TypeTrick) {
            if (triggerEvent == DamageInflicted && TriggerSkill::triggerable(player)) {
                LogMessage log;
                log.type = "#WuyanGood";
                log.from = player;
                log.arg = damage.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
                room->broadcastSkillInvoke(objectName(), 2);

                return true;
            } else if (triggerEvent == DamageCaused && damage.from && TriggerSkill::triggerable(damage.from)) {
                LogMessage log;
                log.type = "#WuyanBad";
                log.from = player;
                log.arg = damage.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
                room->broadcastSkillInvoke(objectName(), 1);

                return true;
            }
        }

        return false;
    }
};

JujianCard::JujianCard()
{
    mute = true;
}

bool JujianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void JujianCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (effect.to->getGeneralName().contains("zhugeliang") || effect.to->getGeneralName() == "wolong")
        room->broadcastSkillInvoke("jujian", 2);
    else
        room->broadcastSkillInvoke("jujian", 1);

    QStringList choicelist;
    choicelist << "draw";
    if (effect.to->isWounded())
        choicelist << "recover";
    if (!effect.to->faceUp() || effect.to->isChained())
        choicelist << "reset";
    QString choice = room->askForChoice(effect.to, "jujian", choicelist.join("+"));

    if (choice == "draw")
        effect.to->drawCards(2, "jujian");
    else if (choice == "recover")
        room->recover(effect.to, RecoverStruct(effect.from));
    else if (choice == "reset") {
        if (effect.to->isChained())
            room->setPlayerProperty(effect.to, "chained", false);
        if (!effect.to->faceUp())
            effect.to->turnOver();
    }
}

class JujianViewAsSkill : public OneCardViewAsSkill
{
public:
    JujianViewAsSkill() : OneCardViewAsSkill("jujian")
    {
        filter_pattern = "^BasicCard!";
        response_pattern = "@@jujian";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        JujianCard *jujianCard = new JujianCard;
        jujianCard->addSubcard(originalCard);
        return jujianCard;
    }
};

class Jujian : public PhaseChangeSkill
{
public:
    Jujian() : PhaseChangeSkill("jujian")
    {
        view_as_skill = new JujianViewAsSkill;
    }

    bool onPhaseChange(ServerPlayer *xushu) const
    {
        Room *room = xushu->getRoom();
        if (xushu->getPhase() == Player::Finish && xushu->canDiscard(xushu, "he"))
            room->askForUseCard(xushu, "@@jujian", "@jujian-card", -1, Card::MethodDiscard);
        return false;
    }
};

class Enyuan : public TriggerSkill
{
public:
    Enyuan() : TriggerSkill("enyuan")
    {
        events << CardsMoveOneTime << Damaged;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.from && move.from->isAlive() && move.from != move.to && move.card_ids.size() >= 2 && move.reason.m_reason != CardMoveReason::S_REASON_PREVIEWGIVE
                    && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip)) {
                move.from->setFlags("EnyuanDrawTarget");
                bool invoke = room->askForSkillInvoke(player, objectName(), data);
                move.from->setFlags("-EnyuanDrawTarget");
                if (invoke) {
                    room->drawCards((ServerPlayer *)move.from, 1, objectName());
                    room->broadcastSkillInvoke(objectName(), 1);
                }
            }
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            if (!source || source == player) return false;
            int x = damage.damage;
            for (int i = 0; i < x; i++) {
                if (source->isAlive() && player->isAlive() && room->askForSkillInvoke(player, objectName(), data)) {
                    room->broadcastSkillInvoke(objectName(), 2);
                    const Card *card = NULL;
                    if (!source->isKongcheng())
                        card = room->askForExchange(source, objectName(), 1, 1, false, "EnyuanGive::" + player->objectName(), true);
                    if (card) {
                        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(),
                            player->objectName(), objectName(), QString());
                        reason.m_playerId = player->objectName();
                        room->moveCardTo(card, source, player, Player::PlaceHand, reason);
                        if (card->getSuit() != Card::Heart) {
                            room->drawCards(player, 1, objectName());
                        }
                        delete card;
                    } else {
                        room->loseHp(source);
                    }
                } else {
                    break;
                }
            }
        }
        return false;
    }
};

//class Xuanhuo : public TriggerSkill
//{
//public:
//    Xuanhuo() : TriggerSkill("xuanhuo")
//    {
//        events << EventPhaseEnd;
//        frequency = Skill::NotFrequent;
//    }
//
//    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
//    {
//        if (player->getPhase() == Player::Draw && room->askForSkillInvoke(player, objectName())) {
//            const Card *cards = room->askForExchange(player, objectName(), 2, 2, false, "", true);
//            if (cards) {
//                QList<int> cardids = cards->getSubcards();
//
//            }
//            if (move.to == player && move.from && move.from->isAlive() && move.from != move.to && move.card_ids.size() >= 2 && move.reason.m_reason != CardMoveReason::S_REASON_PREVIEWGIVE
//                    && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip)) {
//                move.from->setFlags("EnyuanDrawTarget");
//                bool invoke = room->askForSkillInvoke(player, objectName(), data);
//                move.from->setFlags("-EnyuanDrawTarget");
//                if (invoke) {
//                    room->drawCards((ServerPlayer *)move.from, 1, objectName());
//                    room->broadcastSkillInvoke(objectName(), 1);
//                }
//            }
//        }
//        return false;
//    }
//};

class Xuanhuo : public PhaseChangeSkill
{
public:
    Xuanhuo() : PhaseChangeSkill("xuanhuo")
    {
    }

    bool onPhaseChange(ServerPlayer *fazheng) const
    {
        Room *room = fazheng->getRoom();
        if (fazheng->getPhase() == Player::Draw) {
            ServerPlayer *to = room->askForPlayerChosen(fazheng, room->getOtherPlayers(fazheng), objectName(), "xuanhuo-invoke", true, true);
            if (to) {
                room->broadcastSkillInvoke(objectName());
                room->drawCards(to, 2, objectName());
                if (!fazheng->isAlive() || !to->isAlive())
                    return true;

                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *vic, room->getOtherPlayers(to)) {
                    if (to->canSlash(vic))
                        targets << vic;
                }
                ServerPlayer *victim = NULL;
                if (!targets.isEmpty()) {
                    victim = room->askForPlayerChosen(fazheng, targets, "xuanhuo_slash", "@dummy-slash2:" + to->objectName());

                    LogMessage log;
                    log.type = "#CollateralSlash";
                    log.from = fazheng;
                    log.to << victim;
                    room->sendLog(log);
                }

                if (victim == NULL || !room->askForUseSlashTo(to, victim, QString("xuanhuo-slash:%1:%2").arg(fazheng->objectName()).arg(victim->objectName()))) {
                    if (to->isNude())
                        return true;
                    room->setPlayerFlag(to, "xuanhuo_InTempMoving");
                    int first_id = room->askForCardChosen(fazheng, to, "he", "xuanhuo");
                    Player::Place original_place = room->getCardPlace(first_id);
                    DummyCard *dummy = new DummyCard;
                    dummy->addSubcard(first_id);
                    to->addToPile("#xuanhuo", dummy, false);
                    if (!to->isNude()) {
                        int second_id = room->askForCardChosen(fazheng, to, "he", "xuanhuo");
                        dummy->addSubcard(second_id);
                    }

                    //move the first card back temporarily
                    room->moveCardTo(Sanguosha->getCard(first_id), to, original_place, false);
                    room->setPlayerFlag(to, "-xuanhuo_InTempMoving");
                    room->moveCardTo(dummy, fazheng, Player::PlaceHand, false);
                    delete dummy;
                }

                return true;
            }
        }

        return false;
    }
};

class Huilei : public TriggerSkill
{
public:
    Huilei() :TriggerSkill("huilei")
    {
        events << Death;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasSkill(this);
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;
        ServerPlayer *killer = death.damage ? death.damage->from : NULL;
        if (killer && killer != player) {
            LogMessage log;
            log.type = "#HuileiThrow";
            log.from = player;
            log.to << killer;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(player, objectName());

            QString killer_name = killer->getGeneralName();
            if (killer_name.contains("zhugeliang") || killer_name == "wolong")
                room->broadcastSkillInvoke(objectName(), 1);
            else
                room->broadcastSkillInvoke(objectName(), 2);

            killer->throwAllHandCardsAndEquips();
        }

        return false;
    }
};

class Xuanfeng : public TriggerSkill
{
public:
    Xuanfeng() : TriggerSkill("xuanfeng")
    {
        events << CardsMoveOneTime << EventPhaseEnd << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    void perform(Room *room, ServerPlayer *lingtong) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *target, room->getOtherPlayers(lingtong)) {
            if (lingtong->canDiscard(target, "he"))
                targets << target;
        }
        if (targets.isEmpty())
            return;

        if (lingtong->askForSkillInvoke(this)) {
            room->broadcastSkillInvoke(objectName());

            ServerPlayer *first = room->askForPlayerChosen(lingtong, targets, "xuanfeng");
            ServerPlayer *second = NULL;
            int first_id = -1;
            int second_id = -1;
            if (first != NULL) {
                first_id = room->askForCardChosen(lingtong, first, "he", "xuanfeng", false, Card::MethodDiscard);
                room->throwCard(first_id, first, lingtong);
            }
            if (!lingtong->isAlive())
                return;
            targets.clear();
            foreach (ServerPlayer *target, room->getOtherPlayers(lingtong)) {
                if (lingtong->canDiscard(target, "he"))
                    targets << target;
            }
            if (!targets.isEmpty())
                second = room->askForPlayerChosen(lingtong, targets, "xuanfeng");
            if (second != NULL) {
                second_id = room->askForCardChosen(lingtong, second, "he", "xuanfeng", false, Card::MethodDiscard);
                room->throwCard(second_id, second, lingtong);
            }
        }
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *lingtong, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            lingtong->setMark("xuanfeng", 0);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != lingtong)
                return false;

            if (lingtong->getPhase() == Player::Discard
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                lingtong->addMark("xuanfeng", move.card_ids.length());

            if (move.from_places.contains(Player::PlaceEquip) && TriggerSkill::triggerable(lingtong))
                perform(room, lingtong);
        } else if (triggerEvent == EventPhaseEnd && TriggerSkill::triggerable(lingtong)
            && lingtong->getPhase() == Player::Discard && lingtong->getMark("xuanfeng") >= 2) {
            perform(room, lingtong);
        }

        return false;
    }
};

/*
class Pojun : public TriggerSkill
{
public:
    Pojun() : TriggerSkill("pojun")
    {
        events << Damage;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && !damage.chain && !damage.transfer
            && damage.to->isAlive() && !damage.to->hasFlag("Global_DebutFlag")
            && room->askForSkillInvoke(player, objectName(), data)) {
            int x = qMin(5, damage.to->getHp());
            room->broadcastSkillInvoke(objectName(), (x >= 3 || !damage.to->faceUp()) ? 2 : 1);
            damage.to->drawCards(x, objectName());
            damage.to->turnOver();
        }
        return false;
    }
};
*/

class Pojun : public TriggerSkill
{
public:
    Pojun() : TriggerSkill("pojun")
    {
        events << TargetSpecified;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;
            foreach (ServerPlayer *p, use.to) {
                if (player->askForSkillInvoke(this, QVariant::fromValue(p))) {
                    room->broadcastSkillInvoke(objectName());
                    room->showAllCards(p);
                    auto target_cards = p->getCards("he");
                    if (target_cards.isEmpty())
                        return false;
                    QString choice = room->askForChoice(player, objectName(), "red+black", QVariant::fromValue(p));
                    Card::Color color = choice == "red" ? Card::Red : Card::Black;
                    QList<const Card *> cards;
                    foreach (auto c, target_cards) {
                        if (c->getColor() == color) cards << c;
                    }
                    room->clearAG();
                    if (cards.isEmpty())
                        continue;
                    room->throwCards(cards, CardMoveReason(CardMoveReason::S_REASON_DISCARD, player->objectName(), p->objectName(), objectName(), QString()), p);
                    p->drawCards(qMin(2, cards.size()), objectName());
                }
            }
        }

        return false;
    }
};

XianzhenCard::XianzhenCard()
{
}

bool XianzhenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void XianzhenCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (effect.from->pindian(effect.to, "xianzhen", NULL)) {
        ServerPlayer *target = effect.to;
        effect.from->tag["XianzhenTarget"] = QVariant::fromValue(target);
        room->setPlayerFlag(effect.from, "XianzhenSuccess");

        QStringList assignee_list = effect.from->property("extra_slash_specific_assignee").toString().split("+");
        assignee_list << target->objectName();
        room->setPlayerProperty(effect.from, "extra_slash_specific_assignee", assignee_list.join("+"));

        room->setFixedDistance(effect.from, effect.to, 1);
        room->setArmorNullified(effect.to, true);
    } else {
        room->setPlayerCardLimitation(effect.from, "use", "Slash", true);
    }
}

class XianzhenViewAsSkill : public ZeroCardViewAsSkill
{
public:
    XianzhenViewAsSkill() : ZeroCardViewAsSkill("xianzhen")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("XianzhenCard") && !player->isKongcheng();
    }

    const Card *viewAs() const
    {
        return new XianzhenCard;
    }
};

class Xianzhen : public TriggerSkill
{
public:
    Xianzhen() : TriggerSkill("xianzhen")
    {
        events << EventPhaseChanging << Death;
        view_as_skill = new XianzhenViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->tag["XianzhenTarget"].value<ServerPlayer *>() != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *gaoshun, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        }
        ServerPlayer *target = gaoshun->tag["XianzhenTarget"].value<ServerPlayer *>();
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != gaoshun) {
                if (death.who == target) {
                    room->removeFixedDistance(gaoshun, target, 1);
                    gaoshun->tag.remove("XianzhenTarget");
                    room->setPlayerFlag(gaoshun, "-XianzhenSuccess");
                }
                return false;
            }
        }
        if (target) {
            QStringList assignee_list = gaoshun->property("extra_slash_specific_assignee").toString().split("+");
            assignee_list.removeOne(target->objectName());
            room->setPlayerProperty(gaoshun, "extra_slash_specific_assignee", assignee_list.join("+"));

            room->removeFixedDistance(gaoshun, target, 1);
            gaoshun->tag.remove("XianzhenTarget");
            room->setArmorNullified(target, false);
        }
        return false;
    }
};

class Jinjiu : public FilterSkill
{
public:
    Jinjiu() : FilterSkill("jinjiu")
    {
    }

    bool viewFilter(const Card *to_select) const
    {
        return to_select->objectName() == "analeptic";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

MingceCard::MingceCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void MingceCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    QList<ServerPlayer *> targets;
    if (Slash::IsAvailable(effect.to)) {
        foreach (ServerPlayer *p, room->getOtherPlayers(effect.to)) {
            if (effect.to->canSlash(p))
                targets << p;
        }
    }

    ServerPlayer *target = NULL;
    QStringList choicelist;
    choicelist << "draw";
    if (!targets.isEmpty() && effect.from->isAlive()) {
        target = room->askForPlayerChosen(effect.from, targets, "mingce", "@dummy-slash2:" + effect.to->objectName());
        target->setFlags("MingceTarget"); // For AI

        LogMessage log;
        log.type = "#CollateralSlash";
        log.from = effect.from;
        log.to << target;
        room->sendLog(log);

        choicelist << "use";
    }

    try {
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "mingce", QString());
        room->obtainCard(effect.to, this, reason);
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            if (target && target->hasFlag("MingceTarget")) target->setFlags("-MingceTarget");
        throw triggerEvent;
    }

    QString choice = room->askForChoice(effect.to, "mingce", choicelist.join("+"));
    if (target && target->hasFlag("MingceTarget")) target->setFlags("-MingceTarget");

    if (choice == "use") {
        if (effect.to->canSlash(target, NULL, false)) {
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_mingce");
            room->useCard(CardUseStruct(slash, effect.to, target));
        }
    } else if (choice == "draw") {
        effect.to->drawCards(1, "mingce");
    }
}

class Mingce : public OneCardViewAsSkill
{
public:
    Mingce() : OneCardViewAsSkill("mingce")
    {
        filter_pattern = "EquipCard,Slash";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MingceCard");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        MingceCard *mingceCard = new MingceCard;
        mingceCard->addSubcard(originalCard);

        return mingceCard;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Slash"))
            return -2;
        else
            return -1;
    }
};

class Zhichi : public TriggerSkill
{
public:
    Zhichi() : TriggerSkill("zhichi")
    {
        events << Damaged;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::NotActive)
            return false;

        ServerPlayer *current = room->getCurrent();
        if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
            room->broadcastSkillInvoke(objectName(), 1);
            room->notifySkillInvoked(player, objectName());
            if (player->getMark("@late") == 0)
                room->addPlayerMark(player, "@late");

            LogMessage log;
            log.type = "#ZhichiDamaged";
            log.from = player;
            room->sendLog(log);
        }

        return false;
    }
};

class ZhichiProtect : public TriggerSkill
{
public:
    ZhichiProtect() : TriggerSkill("#zhichi-protect")
    {
        events << CardEffected;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if ((effect.card->isKindOf("Slash") || effect.card->isNDTrick()) && effect.to->getMark("@late") > 0) {
            room->broadcastSkillInvoke("zhichi", 2);
            room->notifySkillInvoked(effect.to, "zhichi");
            LogMessage log;
            log.type = "#ZhichiAvoid";
            log.from = effect.to;
            log.arg = "zhichi";
            room->sendLog(log);

            return true;
        }
        return false;
    }
};

class ZhichiClear : public TriggerSkill
{
public:
    ZhichiClear() : TriggerSkill("#zhichi-clear")
    {
        events << EventPhaseChanging << Death;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || player != room->getCurrent())
                return false;
        }

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("@late") > 0)
                room->setPlayerMark(p, "@late", 0);
        }

        return false;
    }
};

GanluCard::GanluCard()
{
}

void GanluCard::swapEquip(ServerPlayer *first, ServerPlayer *second) const
{
    Room *room = first->getRoom();

    QList<int> equips1, equips2;
    foreach(const Card *equip, first->getEquips())
        equips1.append(equip->getId());
    foreach(const Card *equip, second->getEquips())
        equips2.append(equip->getId());

    QList<CardsMoveStruct> exchangeMove;
    CardsMoveStruct move1(equips1, second, Player::PlaceEquip,
        CardMoveReason(CardMoveReason::S_REASON_SWAP, first->objectName(), second->objectName(), "ganlu", QString()));
    CardsMoveStruct move2(equips2, first, Player::PlaceEquip,
        CardMoveReason(CardMoveReason::S_REASON_SWAP, second->objectName(), first->objectName(), "ganlu", QString()));
    exchangeMove.push_back(move2);
    exchangeMove.push_back(move1);
    room->moveCardsAtomic(exchangeMove, false);
}

bool GanluCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

bool GanluCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    switch (targets.length()) {
    case 0: return true;
    case 1: {
        int n1 = targets.first()->getEquips().length();
        int n2 = to_select->getEquips().length();
        return qAbs(n1 - n2) <= Self->getLostHp();
    }
    default:
        return false;
    }
}

void GanluCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    LogMessage log;
    log.type = "#GanluSwap";
    log.from = source;
    log.to = targets;
    room->sendLog(log);

    swapEquip(targets.first(), targets[1]);
}

class Ganlu : public ZeroCardViewAsSkill
{
public:
    Ganlu() : ZeroCardViewAsSkill("ganlu")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("GanluCard");
    }

    const Card *viewAs() const
    {
        return new GanluCard;
    }
};

class Buyi : public TriggerSkill
{
public:
    Buyi() : TriggerSkill("buyi")
    {
        events << Dying;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *wuguotai, QVariant &data) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        ServerPlayer *player = dying.who;
        if (player->isKongcheng()) return false;
        if (player->getHp() < 1 && wuguotai->askForSkillInvoke(this, data)) {
            const Card *card = NULL;
            if (player == wuguotai)
                card = room->askForCardShow(player, wuguotai, objectName());
            else {
                int card_id = room->askForCardChosen(wuguotai, player, "h", "buyi");
                card = Sanguosha->getCard(card_id);
            }

            room->showCard(player, card->getEffectiveId());

            if (card->getTypeId() != Card::TypeBasic) {
                if (!player->isJilei(card))
                    room->throwCard(card, player);

                room->broadcastSkillInvoke(objectName());
                room->recover(player, RecoverStruct(wuguotai));
            }
        }
        return false;
    }
};

XinzhanCard::XinzhanCard()
{
    target_fixed = true;
}

void XinzhanCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->drawCards(1 + qMax(1, source->getLostHp()), "xinzhan");
}

//void XinzhanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
//{
//    QList<int> cards = room->getNCards(3), left;
//
//    LogMessage log;
//    log.type = "$ViewDrawPile";
//    log.from = source;
//    log.card_str = IntList2StringList(cards).join("+");
//    room->sendLog(log, source);
//
//    left = cards;
//
//    QList<int> hearts, non_hearts;
//    foreach (int card_id, cards) {
//        const Card *card = Sanguosha->getCard(card_id);
//        if (card->getSuit() == Card::Heart)
//            hearts << card_id;
//        else
//            non_hearts << card_id;
//    }
//
//    if (!hearts.isEmpty()) {
//        DummyCard *dummy = new DummyCard;
//        do {
//            room->fillAG(left, source, non_hearts);
//            int card_id = room->askForAG(source, hearts, true, "xinzhan");
//            if (card_id == -1) {
//                room->clearAG(source);
//                break;
//            }
//
//            hearts.removeOne(card_id);
//            left.removeOne(card_id);
//
//            dummy->addSubcard(card_id);
//            room->clearAG(source);
//        } while (!hearts.isEmpty());
//
//        if (dummy->subcardsLength() > 0) {
//            room->doBroadcastNotify(QSanProtocol::S_COMMAND_UPDATE_PILE, QVariant(room->getDrawPile().length() + dummy->subcardsLength()));
//            source->obtainCard(dummy);
//            foreach(int id, dummy->getSubcards())
//                room->showCard(source, id);
//        }
//        delete dummy;
//    }
//
//    if (!left.isEmpty())
//        room->askForGuanxing(source, left, Room::GuanxingUpOnly);
//}

class Xinzhan : public OneCardViewAsSkill
{
public:
    Xinzhan() : OneCardViewAsSkill("xinzhan")
    {
    }

    bool isEnabledAtPlay(const Player *) const
    {
        //return player->usedTimes("XinzhanCard") < player->getHp();
        return true;
    }

    bool viewFilter(const Card *to_select) const
    {
        return to_select->getSuit() == Card::Heart;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        XinzhanCard *xz = new XinzhanCard;
        xz->addSubcard(originalCard);
        xz->setSkillName(objectName());
        return xz;
    }
};

//class Xinzhan : public ZeroCardViewAsSkill
//{
//public:
//    Xinzhan() : ZeroCardViewAsSkill("xinzhan")
//    {
//    }
//
//    bool isEnabledAtPlay(const Player *player) const
//    {
//        return !player->hasUsed("XinzhanCard") && player->getHandcardNum() > player->getMaxHp();
//    }
//
//    const Card *viewAs() const
//    {
//        return new XinzhanCard;
//    }
//};

class Quanji : public MasochismSkill
{
public:
    Quanji() : MasochismSkill("#quanji")
    {
        frequency = Frequent;
    }

    void onDamaged(ServerPlayer *zhonghui, const DamageStruct &damage) const
    {
        Room *room = zhonghui->getRoom();

        int x = damage.damage;
        for (int i = 0; i < x; i++) {
            if (zhonghui->askForSkillInvoke("quanji")) {
                room->broadcastSkillInvoke("quanji");
                room->drawCards(zhonghui, 1, objectName());
                if (!zhonghui->isKongcheng()) {
                    int card_id;
                    if (zhonghui->getHandcardNum() == 1) {
                        room->getThread()->delay();
                        card_id = zhonghui->handCards().first();
                    } else {
                        const Card *card = room->askForExchange(zhonghui, "quanji", 1, 1, false, "QuanjiPush");
                        card_id = card->getEffectiveId();
                        delete card;
                    }
                    zhonghui->addToPile("power", card_id);
                }
            }
        }

    }
};

class QuanjiKeep : public MaxCardsSkill
{
public:
    QuanjiKeep() : MaxCardsSkill("quanji")
    {
        frequency = Frequent;
    }

    int getExtra(const Player *target) const
    {
        if (target->hasSkill(this))
            return target->getPile("power").length();
        else
            return 0;
    }
};

class Zili : public PhaseChangeSkill
{
public:
    Zili() : PhaseChangeSkill("zili")
    {
        frequency = Wake;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && target->getMark("zili") == 0
            && target->getPile("power").length() >= 3;
    }

    bool onPhaseChange(ServerPlayer *zhonghui) const
    {
        Room *room = zhonghui->getRoom();
        room->notifySkillInvoked(zhonghui, objectName());

        LogMessage log;
        log.type = "#ZiliWake";
        log.from = zhonghui;
        log.arg = QString::number(zhonghui->getPile("power").length());
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());
        //room->doLightbox("$ZiliAnimate", 4000);

        room->doSuperLightbox("zhonghui", "zili");

        room->setPlayerMark(zhonghui, "zili", 1);
        if (room->changeMaxHpForAwakenSkill(zhonghui)) {
            if (zhonghui->isWounded() && room->askForChoice(zhonghui, objectName(), "recover+draw") == "recover")
                room->recover(zhonghui, RecoverStruct(zhonghui));
            else
                room->drawCards(zhonghui, 2, objectName());
            if (zhonghui->getMark("zili") == 1)
                room->acquireSkill(zhonghui, "paiyi");
        }

        return false;
    }
};

PaiyiCard::PaiyiCard()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool PaiyiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void PaiyiCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *zhonghui = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhonghui->getRoom();
    QList<int> powers = zhonghui->getPile("power");
    if (powers.isEmpty()) return;

    room->broadcastSkillInvoke("paiyi", target == zhonghui ? 1 : 2);

    int card_id = subcards.first();

    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), target->objectName(), "paiyi", QString());
    room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
    room->drawCards(target, 2, "paiyi");
    if (target->getHandcardNum() > zhonghui->getHandcardNum())
        room->damage(DamageStruct("paiyi", zhonghui, target));
}

class Paiyi : public OneCardViewAsSkill
{
public:
    Paiyi() : OneCardViewAsSkill("paiyi")
    {
        expand_pile = "power";
        filter_pattern = ".|.|.|power";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("power").isEmpty() && !player->hasUsed("PaiyiCard");
    }

    const Card *viewAs(const Card *c) const
    {
        PaiyiCard *py = new PaiyiCard;
        py->addSubcard(c);
        return py;
    }
};

class Jueqing : public TriggerSkill
{
public:
    Jueqing() : TriggerSkill("jueqing")
    {
        frequency = Compulsory;
        events << Predamage;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *zhangchunhua, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from == zhangchunhua) {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(zhangchunhua, objectName());
            room->loseHp(damage.to, damage.damage);

            return true;
        }
        return false;
    }
};

Shangshi::Shangshi() : TriggerSkill("shangshi")
{
    events << HpChanged << MaxHpChanged << CardsMoveOneTime;
    frequency = Frequent;
}

int Shangshi::getMaxLostHp(ServerPlayer *zhangchunhua) const
{
    int losthp = zhangchunhua->getLostHp();
    if (losthp > 2)
        losthp = 2;
    return qMin(losthp, zhangchunhua->getMaxHp());
}

bool Shangshi::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhangchunhua, QVariant &data) const
{
    int losthp = getMaxLostHp(zhangchunhua);
    if (triggerEvent == CardsMoveOneTime) {
        bool can_invoke = false;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == zhangchunhua && move.from_places.contains(Player::PlaceHand))
            can_invoke = true;
        if (move.to == zhangchunhua && move.to_place == Player::PlaceHand)
            can_invoke = true;
        if (!can_invoke)
            return false;
    } else if (triggerEvent == HpChanged || triggerEvent == MaxHpChanged) {
        if (zhangchunhua->getPhase() == Player::Discard) {
            zhangchunhua->addMark("shangshi");
            return false;
        }
    }

    if (zhangchunhua->getHandcardNum() < losthp && zhangchunhua->askForSkillInvoke(this)) {
        room->broadcastSkillInvoke("shangshi");
        zhangchunhua->drawCards(losthp - zhangchunhua->getHandcardNum(), objectName());
    }

    return false;
}

YJCMPackage::YJCMPackage()
    : Package("YJCM")
{
    General *caozhi = new General(this, "caozhi", "wei", 3); // YJ 001
    caozhi->addSkill(new Luoying);
    caozhi->addSkill(new Jiushi);
    caozhi->addSkill(new JiushiFlip);
    related_skills.insertMulti("jiushi", "#jiushi-flip");

    General *chengong = new General(this, "chengong", "qun", 3); // YJ 002
    chengong->addSkill(new Zhichi);
    chengong->addSkill(new ZhichiProtect);
    chengong->addSkill(new ZhichiClear);
    chengong->addSkill(new Mingce);
    related_skills.insertMulti("zhichi", "#zhichi-protect");
    related_skills.insertMulti("zhichi", "#zhichi-clear");

    General *fazheng = new General(this, "fazheng", "shu", 3); // YJ 003
    fazheng->addSkill(new Enyuan);
    fazheng->addSkill(new Xuanhuo);
    fazheng->addSkill(new FakeMoveSkill("xuanhuo"));
    related_skills.insertMulti("xuanhuo", "#xuanhuo-fake-move");

    General *gaoshun = new General(this, "gaoshun", "qun"); // YJ 004
    gaoshun->addSkill(new Xianzhen);
    gaoshun->addSkill(new Jinjiu);

    General *lingtong = new General(this, "lingtong", "wu"); // YJ 005
    lingtong->addSkill(new Xuanfeng);

    General *masu = new General(this, "masu", "shu", 3); // YJ 006
    masu->addSkill(new Xinzhan);
    masu->addSkill(new Huilei);

    General *wuguotai = new General(this, "wuguotai", "wu", 3, false); // YJ 007
    wuguotai->addSkill(new Ganlu);
    wuguotai->addSkill(new Buyi);

    General *xusheng = new General(this, "xusheng", "wu"); // YJ 008
    xusheng->addSkill(new Pojun);

    General *xushu = new General(this, "xushu", "shu", 3); // YJ 009
    xushu->addSkill(new Wuyan);
    xushu->addSkill(new Jujian);

    General *yujin = new General(this, "yujin", "wei"); // YJ 010
    yujin->addSkill(new Yizhong);

    General *zhangchunhua = new General(this, "zhangchunhua", "wei", 3, false); // YJ 011
    zhangchunhua->addSkill(new Jueqing);
    zhangchunhua->addSkill(new Shangshi);

    General *zhonghui = new General(this, "zhonghui", "wei"); // YJ 012
    zhonghui->addSkill(new QuanjiKeep);
    zhonghui->addSkill(new Quanji);
    zhonghui->addSkill(new Zili);
    zhonghui->addRelateSkill("paiyi");
    related_skills.insertMulti("quanji", "#quanji");

    addMetaObject<MingceCard>();
    addMetaObject<GanluCard>();
    addMetaObject<XianzhenCard>();
    addMetaObject<XinzhanCard>();
    addMetaObject<JujianCard>();
    addMetaObject<PaiyiCard>();

    skills << new Paiyi;
}

ADD_PACKAGE(YJCM)

