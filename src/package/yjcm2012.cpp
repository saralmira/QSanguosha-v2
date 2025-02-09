#include "yjcm2012.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"
#include "util.h"
#include "exppattern.h"
#include "room.h"
#include "roomthread.h"
#include "wrapped-card.h"

class Zhenlie : public TriggerSkill
{
public:
    Zhenlie() : TriggerSkill("zhenlie")
    {
        events << TargetConfirmed;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetConfirmed && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.to.contains(player) && use.from != player) {
                if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
                    if (room->askForSkillInvoke(player, objectName(), data)) {
                        room->broadcastSkillInvoke(objectName());
                        player->setFlags("-ZhenlieTarget");
                        player->setFlags("ZhenlieTarget");
                        room->loseHp(player);
                        if (player->isAlive() && player->hasFlag("ZhenlieTarget")) {
                            player->setFlags("-ZhenlieTarget");
                            use.nullified_list << player->objectName();
                            data = QVariant::fromValue(use);
                            if (player->canDiscard(use.from, "he")) {
                                int id = room->askForCardChosen(player, use.from, "he", objectName(), false, Card::MethodDiscard);
                                room->throwCard(id, use.from, player);
                            }
                        }
                    }
                }
            }
        }
        return false;
    }
};

class Miji : public TriggerSkill
{
public:
    Miji() : TriggerSkill("miji")
    {
        events << EventPhaseStart << ChoiceMade;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        if (TriggerSkill::triggerable(target) && triggerEvent == EventPhaseStart
            && target->getPhase() == Player::Finish && target->isWounded() && target->askForSkillInvoke(this)) {
            room->broadcastSkillInvoke(objectName());
            QStringList draw_num;
            for (int i = 1; i <= target->getLostHp(); draw_num << QString::number(i++)) {

            }
            int num = room->askForChoice(target, "miji_draw", draw_num.join("+")).toInt();
            target->drawCards(num, objectName());
            target->setMark(objectName(), 0);
            if (!target->isKongcheng()) {
                forever {
                    int n = target->getMark(objectName());
                    if (n < num && !target->isKongcheng()) {
                        QList<int> handcards = target->handCards();
                        if (!room->askForYiji(target, handcards, objectName(), false, false, false, num - n))
                            break;
                    } else {
                        break;
                    }
                }
                // give the rest cards randomly
                if (target->getMark(objectName()) < num && !target->isKongcheng()) {
                    int rest_num = num - target->getMark(objectName());
                    forever {
                        QList<int> handcard_list = target->handCards();
                        qShuffle(handcard_list);
                        int give = qrand() % rest_num + 1;
                        rest_num -= give;
                        QList<int> to_give = handcard_list.length() < give ? handcard_list : handcard_list.mid(0, give);
                        ServerPlayer *receiver = room->getOtherPlayers(target).at(qrand() % (target->aliveCount() - 1));
                        DummyCard *dummy = new DummyCard(to_give);
                        room->obtainCard(receiver, dummy, false);
                        delete dummy;
                        if (rest_num == 0 || target->isKongcheng())
                            break;
                    }
                }
            }
        } else if (triggerEvent == ChoiceMade) {
            QString str = data.toString();
            if (str.startsWith("Yiji:" + objectName()))
                target->addMark(objectName(), str.split(":").last().split("+").length());
        }
        return false;
    }
};

QiceCard::QiceCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool QiceCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *card = Self->tag.value("qice").value<const Card *>();
    Card *mutable_card = Sanguosha->cloneCard(card);
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, mutable_card, targets);
}

bool QiceCard::targetFixed() const
{
    const Card *card = Self->tag.value("qice").value<const Card *>();
    Card *mutable_card = Sanguosha->cloneCard(card);
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetFixed();
}

bool QiceCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Card *card = Self->tag.value("qice").value<const Card *>();
    Card *mutable_card = Sanguosha->cloneCard(card);
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetsFeasible(targets, Self);
}

const Card *QiceCard::validate(CardUseStruct &card_use) const
{
    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("qice");
    use_card->addSubcards(this->subcards);
    bool available = true;
    foreach(ServerPlayer *to, card_use.to)
        if (card_use.from->isProhibited(to, use_card)) {
            available = false;
            break;
        }
    available = available && use_card->isAvailable(card_use.from);
    use_card->deleteLater();
    if (!available) return NULL;
    return use_card;
}

class Qice : public ViewAsSkill
{
public:
    Qice() : ViewAsSkill("qice")
    {
        response_pattern = "nullification";
        response_or_use = true;
    }

    QDialog *getDialog() const
    {
        return GuhuoDialog::getInstance("qice", false);
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isEquipped();
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() < Self->getHandcardNum())
            return NULL;

        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
            const Card *c = Self->tag.value("qice").value<const Card *>();
            if (c) {
                QiceCard *card = new QiceCard;
                card->setUserString(c->objectName());
                card->addSubcards(cards);
                return card;
            } else
                return NULL;
        }
        case CardUseStruct::CARD_USE_REASON_RESPONSE:
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "nullification") {
                Card *nullification = Sanguosha->cloneCard("nullification");
                nullification->addSubcards(cards);
                nullification->setSkillName(objectName());
                return nullification;
            }
            return NULL;
        }
        default:
            return NULL;
        }
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("QiceCard");
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("QiceCard");
    }
};

class Zhiyu : public MasochismSkill
{
public:
    Zhiyu() : MasochismSkill("zhiyu")
    {
    }

    void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        if (target->askForSkillInvoke(this, QVariant::fromValue(damage))) {
            target->drawCards(1, objectName());

            Room *room = target->getRoom();
            room->broadcastSkillInvoke(objectName(), 1);

            if (target->isKongcheng())
                return;
            room->showAllCards(target);

            QList<const Card *> cards = target->getHandcards();
            Card::Color color = cards.first()->getColor();
            bool same_color = true;
            foreach (const Card *card, cards) {
                if (card->getColor() != color) {
                    same_color = false;
                    break;
                }
            }

            if (same_color && damage.from) {
                ServerPlayer *chosen = room->askForPlayerChosen(target, room->getOtherPlayers(target), objectName(), "zhiyu-invoke", true);
                if (!chosen)
                    return;
                room->clearAG();
                chosen->drawCards(1, objectName());
                if (chosen->isKongcheng())
                    return;
                room->showAllCards(chosen);
                room->delay();
                room->clearAG();
                room->broadcastSkillInvoke(objectName(), 2);
                room->damage(DamageStruct(objectName(), target, chosen));
            }
        }
    }
};

//class Jiangchi : public DrawCardsSkill
//{
//public:
//    Jiangchi() : DrawCardsSkill("jiangchi")
//    {
//    }
//
//    int getDrawNum(ServerPlayer *caozhang, int n) const
//    {
//        Room *room = caozhang->getRoom();
//        QString choice = room->askForChoice(caozhang, objectName(), "jiang+chi+cancel");
//        if (choice == "cancel")
//            return n;
//
//        room->notifySkillInvoked(caozhang, objectName());
//        LogMessage log;
//        log.from = caozhang;
//        log.arg = objectName();
//        if (choice == "jiang") {
//            log.type = "#Jiangchi1";
//            room->sendLog(log);
//            room->broadcastSkillInvoke(objectName(), 1);
//            room->setPlayerCardLimitation(caozhang, "use,response", "Slash", true);
//            return n + 1;
//        } else {
//            log.type = "#Jiangchi2";
//            room->sendLog(log);
//            room->broadcastSkillInvoke(objectName(), 2);
//            room->setPlayerFlag(caozhang, "JiangchiInvoke");
//            return n - 1;
//        }
//    }
//};

class Jiangchi : public TriggerSkill
{
public:
    Jiangchi() : TriggerSkill("jiangchi")
    {
        events << EventPhaseStart;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Play/* && room->askForSkillInvoke(player, objectName())*/) {

            room->setPlayerMark(player, "JiangchiInvoke", 0);
            QString choice = room->askForChoice(player, objectName(), "jiang+chi+cancel");
            if (choice == "cancel")
                return false;

            room->notifySkillInvoked(player, objectName());
            LogMessage log;
            log.from = player;
            log.arg = objectName();
            int x = player->getLostHp();
            x = x > 0 ? x : 1;
            log.arg2 = QString::number(x);
            if (choice == "jiang") {
                log.type = "#Jiangchi1";
                room->sendLog(log);
                room->broadcastSkillInvoke(objectName(), 1);
                room->drawCards(player, 2, objectName());
                room->askForDiscard(player, objectName(), x, x, false, true);
                room->setPlayerCardLimitation(player, "use,response", "Slash", true);
                room->setPlayerMark(player, "JiangchiInvoke", 2);
                room->setPlayerMark(player, "JiangchiMaxCard", x);
            } else {
                log.type = "#Jiangchi2";
                room->sendLog(log);
                room->broadcastSkillInvoke(objectName(), 2);
                room->drawCards(player, x, objectName());
                room->askForDiscard(player, objectName(), 2, 2, false, true);
                room->setPlayerMark(player, "JiangchiInvoke", 1);
            }
        }
        return false;
    }
};

class JiangchiTargetMod : public TargetModSkill
{
public:
    JiangchiTargetMod() : TargetModSkill("#jiangchi-target")
    {
        frequency = NotFrequent;
    }

    int getResidueNum(const Player *from, const Card *) const
    {
        if (from->getMark("JiangchiInvoke") == 1)
            return 1;
        else
            return 0;
    }

    int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->getMark("JiangchiInvoke") == 1)
            return 1000;
        else
            return 0;
    }
};

class JiangchiEffect : public MaxCardsSkill
{
public:
    JiangchiEffect() : MaxCardsSkill("#jiangchi-effect2")
    {
    }

    int getExtra(const Player *target) const
    {
        return target->getMark("JiangchiInvoke") == 2 ? target->getMark("JiangchiMaxCard") : 0;
    }
};


class Qianxi : public TriggerSkill
{
public:
    Qianxi() : TriggerSkill("qianxi")
    {
        events << EventPhaseStart << FinishJudge;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Start) {
            if (room->askForSkillInvoke(target, objectName())) {
                JudgeStruct judge;
                judge.reason = objectName();
                judge.play_animation = false;
                judge.who = target;

                room->judge(judge);
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason != objectName() || !target->isAlive()) return false;

            QString color = judge->card->isRed() ? "red" : "black";
            target->tag[objectName()] = QVariant::fromValue(color);
            judge->pattern = color;

            QList<ServerPlayer *> to_choose;
            foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
                if (target->distanceTo(p) == 1)
                    to_choose << p;
            }
            if (to_choose.isEmpty())
                return false;

            ServerPlayer *victim = room->askForPlayerChosen(target, to_choose, objectName(), QString("qianxi-invoke:::%1").arg(color), true);
            if (!victim || !victim->isAlive()) return false;

            QList<const Card*> equips;
            QList<int> disabledIDs;
            foreach (auto c, victim->getEquips()) {
                if (c->getColor() == judge->card->getColor())
                    equips << c;
                else
                    disabledIDs << c->getId();
            }

            if (!equips.isEmpty())
                room->askForThrow(target, victim, 1, "e", objectName(), false, disabledIDs);

            QString pattern = QString(".|%1|.|hand$0").arg(color);

            room->broadcastSkillInvoke(objectName());
            room->setPlayerFlag(victim, "QianxiTarget");
            room->addPlayerMark(victim, QString("@qianxi_%1").arg(color));
            room->setPlayerCardLimitation(victim, "use,response", pattern, false);

            LogMessage log;
            log.type = "#Qianxi";
            log.from = victim;
            log.arg = QString("no_suit_%1").arg(color);
            room->sendLog(log);
        }
        return false;
    }
};

class QianxiClear : public TriggerSkill
{
public:
    QianxiClear() : TriggerSkill("#qianxi-clear")
    {
        events << EventPhaseChanging << Death;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return !target->tag["qianxi"].toString().isNull();
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;
        }

        QString color = player->tag["qianxi"].toString();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->hasFlag("QianxiTarget")) {
                room->removePlayerCardLimitation(p, "use,response", QString(".|%1|.|hand$0").arg(color));
                room->setPlayerMark(p, QString("@qianxi_%1").arg(color), 0);
            }
        }
        return false;
    }
};

class DangxianFilter : public FilterSkill
{
public:
    DangxianFilter() : FilterSkill("#dangxian-f")
    {
    }

    bool canFilter(const Room *, const ServerPlayer *player) const
    {
        return player->getMark("dangxian");
    }

    bool viewFilter(const Card *to_select) const
    {
        Room *room = Sanguosha->currentRoom();
        Player::Place place = room->getCardPlace(to_select->getEffectiveId());
        return place == Player::PlaceHand && (to_select->isRed() || to_select->isBlack());
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Card *vs = NULL;
        if (originalCard->isRed()) {
            vs = new FireSlash(originalCard->getSuit(), originalCard->getNumber());
        } else if (originalCard->isBlack()) {
            vs = new ThunderSlash(originalCard->getSuit(), originalCard->getNumber());
        }
        if (!vs) { // this is nearly impossible.
            return originalCard;
        }
        vs->setSkillName("dangxian");
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(vs);
        return card;
    }
};

class DangxianTriggerEnd : public TriggerSkill
{
public:
    DangxianTriggerEnd() : TriggerSkill("#dangxian-te")
    {
        frequency = Compulsory;
        events << EventPhaseEnd << Damage;
        global = true;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *liaohua, QVariant &data) const
    {
        if (liaohua->getPhase() != Player::Play || !liaohua->getMark("dangxian"))
            return false;

        if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from == liaohua && damage.damage > 0) {
                room->addPlayerHistory(liaohua, "dangxianDmg");
            }
        } else if (triggerEvent == EventPhaseEnd) {
            room->setPlayerMark(liaohua, "dangxian", 0);
            room->setInfinityAttackRange(liaohua, false);
            //room->filterCards(liaohua, liaohua->getCards("he"), true);
            if (!liaohua->getHistory("dangxianDmg"))
                room->loseHp(liaohua, 1);
        }

        return false;
    }
};

class Dangxian : public TriggerSkill
{
public:
    Dangxian() : TriggerSkill("dangxian")
    {
        frequency = Compulsory;
        events << EventPhaseStart;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *liaohua, QVariant &) const
    {
        if (liaohua->getPhase() == Player::RoundStart) {
            int n = 1;
            if (liaohua->getGeneralName() == "guansuo") {
                n = 2;
            }

            if (room->askForSimpleChoice(liaohua, objectName(), "invoke")) {
                room->setPlayerMark(liaohua, "dangxian", 1);
                liaohua->drawCards(1, objectName());
                room->setInfinityAttackRange(liaohua, true);
                //room->filterCards(liaohua, liaohua->getCards("he"), true);
            }

            room->broadcastSkillInvoke(objectName(), n);
            room->sendCompulsoryTriggerLog(liaohua, objectName());
            //room->setPlayerMark(liaohua, "dangxian_trigger", 0);

            liaohua->setPhase(Player::Play);
            room->broadcastProperty(liaohua, "phase");
            RoomThread *thread = room->getThread();
            if (!thread->trigger(EventPhaseStart, room, liaohua))
                thread->trigger(EventPhaseProceeding, room, liaohua);
            thread->trigger(EventPhaseEnd, room, liaohua);

            liaohua->setPhase(Player::RoundStart);
            room->broadcastProperty(liaohua, "phase");
        }
        return false;
    }
};

class Fuli : public TriggerSkill
{
public:
    Fuli() : TriggerSkill("fuli")
    {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@laoji";
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark("@laoji") > 0;
    }

    int getKingdoms(Room *room) const
    {
        QSet<QString> kingdom_set;
        foreach(ServerPlayer *p, room->getAlivePlayers())
            kingdom_set << p->getKingdom();
        return kingdom_set.size();
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *liaohua, QVariant &data) const
    {
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != liaohua)
            return false;
        if (liaohua->askForSkillInvoke(this, data)) {
            room->broadcastSkillInvoke(objectName());
            //room->doLightbox("$FuliAnimate", 3000);

            room->doSuperLightbox("liaohua", "fuli");

            room->removePlayerMark(liaohua, "@laoji");
            int x = getKingdoms(room);
            room->recover(liaohua, RecoverStruct(liaohua, NULL, x - liaohua->getHp()));
            liaohua->drawCards(x, objectName());

            int hp = liaohua->getHp();
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->getHp() >= hp)
                    return false;
            }
            liaohua->turnOver();
        }
        return false;
    }
};

class Zishou : public DrawCardsSkill
{
public:
    Zishou() : DrawCardsSkill("zishou")
    {
    }

    int getDrawNum(ServerPlayer *liubiao, int n) const
    {
        Room *room = liubiao->getRoom();
        if (liubiao->isWounded() && room->askForSkillInvoke(liubiao, objectName())) {
            int losthp = liubiao->getLostHp();
            room->broadcastSkillInvoke(objectName(), qMin(3, losthp));
            liubiao->clearHistory();
            liubiao->skip(Player::Play);
            return n + losthp;
        } else
            return n;
    }
};

class Zongshi : public MaxCardsSkill
{
public:
    Zongshi() : MaxCardsSkill("zongshi")
    {
    }

    int getExtra(const Player *target) const
    {
        int extra = 0;
        QSet<QString> kingdom_set;
        if (target->parent()) {
            foreach(const Player *player, target->parent()->findChildren<const Player *>())
            {
                if (player->isAlive())
                    kingdom_set << player->getKingdom();
            }
        }
        extra = kingdom_set.size();
        if (target->hasSkill(this))
            return extra;
        else
            return 0;
    }
};

class Shiyong : public TriggerSkill
{
public:
    Shiyong() : TriggerSkill("shiyong")
    {
        events << Damaged;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        // damage.card->hasFlag("drank")
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isRed()) {
            if (!damage.from)
                return false;
            int index = 2;
            if (damage.from->getGeneralName().contains("guanyu"))
                index = 3;
            // else if (damage.card->hasFlag("drank"))
            //     index = 2;
            room->broadcastSkillInvoke(objectName(), index);
            room->sendCompulsoryTriggerLog(player, objectName());

            damage.from->drawCards(1, objectName());
            // room->loseMaxHp(player);
        }
        else {
            room->broadcastSkillInvoke(objectName(), 1);
            room->sendCompulsoryTriggerLog(player, objectName());

            player->drawCards(1, objectName());
        }
        return false;
    }
};

YJYaowuCard::YJYaowuCard()
{
    will_throw = true;
    target_fixed = false;
    handling_method = Card::MethodDiscard;
}

bool YJYaowuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void YJYaowuCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *player = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = player->getRoom();

    Duel *duel = new Duel(Card::NoSuit, 0);
    duel->setSkillName("yjyaowu");
    room->useCard(CardUseStruct(duel, player, target));
    delete duel;
}

class YJYaowuViewAsSkill : public OneCardViewAsSkill
{
public:
    YJYaowuViewAsSkill() : OneCardViewAsSkill("yjyaowu")
    {
        response_pattern = "@@yjyaowu";
    }

    bool viewFilter(const Card *to_select) const
    {
        // return to_select->isKindOf("EquipCard");
        return to_select->isEquipped();
    }

    const Card *viewAs(const Card *originalcard) const
    {
        YJYaowuCard *card = new YJYaowuCard;
        card->addSubcard(originalcard->getId());
        card->setSkillName("yjyaowu");
        return card;
    }
};

class YJYaowu : public TriggerSkill
{
public:
    YJYaowu() : TriggerSkill("yjyaowu")
    {
        events << EventPhaseStart;
        frequency = NotFrequent;
        view_as_skill = new YJYaowuViewAsSkill;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Play && player->getEquips().count() > 0)
        {
            room->askForUseCard(player, "@@yjyaowu", "yjyaowu-invoke", -1, Card::MethodDiscard);
            //if (card) {
            //    room->notifySkillInvoked(player, objectName());
            //    room->broadcastSkillInvoke(objectName());
            //}
        }
        return false;
    }
};

class FuhunViewAsSkill : public ViewAsSkill
{
public:
    FuhunViewAsSkill() : ViewAsSkill("fuhun")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getHandcardNum() >= 2 && Slash::IsAvailable(player);
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return player->getHandcardNum() >= 2 && pattern == "slash";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && !to_select->isEquipped();
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        Slash *slash = new Slash(Card::SuitToBeDecided, 0);
        slash->setSkillName(objectName());
        slash->addSubcards(cards);

        return slash;
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return 1;
    }
};

class Fuhun : public TriggerSkill
{
public:
    Fuhun() : TriggerSkill("fuhun")
    {
        events << Damage << EventPhaseChanging;
        view_as_skill = new FuhunViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Damage && TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->getSkillName() == objectName()
                && player->getPhase() == Player::Play) {
                room->handleAcquireDetachSkills(player, "wusheng|paoxiao");
                room->broadcastSkillInvoke(objectName(), 2);
                player->setFlags(objectName());
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && player->hasFlag(objectName()))
                room->handleAcquireDetachSkills(player, "-wusheng|-paoxiao", true);
        }

        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return 1;
    }
};

GongqiCard::GongqiCard()
{
    mute = true;
    target_fixed = true;
}

void GongqiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->setInfinityAttackRange(source, true);
    const Card *cd = Sanguosha->getCard(subcards.first());
    if (cd->getTypeId() != Card::TypeBasic/*cd->isKindOf("EquipCard")*/) {
        room->broadcastSkillInvoke("gongqi", 2);
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getOtherPlayers(source))
            if (source->canDiscard(p, "he")) targets << p;
        if (!targets.isEmpty()) {
            ServerPlayer *to_discard = room->askForPlayerChosen(source, targets, "gongqi", "@gongqi-discard", true);
            if (to_discard)
                room->throwCard(room->askForCardChosen(source, to_discard, "he", "gongqi", false, Card::MethodDiscard), to_discard, source);
        }
    } else {
        room->broadcastSkillInvoke("gongqi", 1);
    }
}

class Gongqi : public OneCardViewAsSkill
{
public:
    Gongqi() : OneCardViewAsSkill("gongqi")
    {
        filter_pattern = ".!";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("GongqiCard");
    }

    const Card *viewAs(const Card *originalcard) const
    {
        GongqiCard *card = new GongqiCard;
        card->addSubcard(originalcard->getId());
        card->setSkillName(objectName());
        return card;
    }
};

JiefanCard::JiefanCard()
{
    mute = true;
}

bool JiefanCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void JiefanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->removePlayerMark(source, "@rescue");
    ServerPlayer *target = targets.first();
    source->tag["JiefanTarget"] = QVariant::fromValue(target);
    room->broadcastSkillInvoke("jiefan");
    //room->doLightbox("$JiefanAnimate", 2500);
    room->doSuperLightbox("handang", "jiefan");

    foreach (ServerPlayer *player, room->getAllPlayers()) {
        if (player->isAlive() && player->inMyAttackRange(target))
            room->cardEffect(this, source, player);
    }
    source->tag.remove("JiefanTarget");
}

void JiefanCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    ServerPlayer *target = effect.from->tag["JiefanTarget"].value<ServerPlayer *>();
    QVariant data = effect.from->tag["JiefanTarget"];
    if (target && !room->askForCard(effect.to, ".Weapon", "@jiefan-discard::" + target->objectName(), data))
        target->drawCards(1, "jiefan");
}

class Jiefan : public ZeroCardViewAsSkill
{
public:
    Jiefan() : ZeroCardViewAsSkill("jiefan")
    {
        frequency = Limited;
        limit_mark = "@rescue";
    }

    const Card *viewAs() const
    {
        return new JiefanCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@rescue") >= 1;
    }
};

AnxuCard::AnxuCard()
{
    mute = true;
}

bool AnxuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (to_select == Self)
        return false;
    if (targets.isEmpty())
        return true;
    else if (targets.length() == 1)
        return to_select->getHandcardNum() != targets.first()->getHandcardNum();
    else
        return false;
}

bool AnxuCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void AnxuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QList<ServerPlayer *> selecteds = targets;
    ServerPlayer *from = selecteds.first()->getHandcardNum() < selecteds.last()->getHandcardNum() ? selecteds.takeFirst() : selecteds.takeLast();
    ServerPlayer *to = selecteds.takeFirst();
    if (from->getGeneralName().contains("sunquan"))
        room->broadcastSkillInvoke("anxu", 2);
    else
        room->broadcastSkillInvoke("anxu", 1);
    int id = room->askForCardChosen(from, to, "h", "anxu");
    const Card *cd = Sanguosha->getCard(id);
    from->obtainCard(cd);
    room->showCard(from, id);
    if (cd->getSuit() != Card::Spade)
        source->drawCards(1, "anxu");
}

class Anxu : public ZeroCardViewAsSkill
{
public:
    Anxu() : ZeroCardViewAsSkill("anxu")
    {
    }

    const Card *viewAs() const
    {
        return new AnxuCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("AnxuCard");
    }
};

class Zhuiyi : public TriggerSkill
{
public:
    Zhuiyi() : TriggerSkill("zhuiyi")
    {
        events << Death;
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
        QList<ServerPlayer *> targets = (death.damage && death.damage->from) ? room->getOtherPlayers(death.damage->from) :
            room->getAlivePlayers();

        if (targets.isEmpty())
            return false;

        QString prompt = "zhuiyi-invoke";
        if (death.damage && death.damage->from && death.damage->from != player)
            prompt = QString("%1x:%2").arg(prompt).arg(death.damage->from->objectName());
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), prompt, true, true);
        if (!target) return false;

        if (target->getGeneralName().contains("sunquan"))
            room->broadcastSkillInvoke(objectName(), 2);
        else
            room->broadcastSkillInvoke(objectName(), 1);

        target->drawCards(3, objectName());
        room->recover(target, RecoverStruct(player), true);
        return false;
    }
};

class LihuoViewAsSkill : public OneCardViewAsSkill
{
public:
    LihuoViewAsSkill() : OneCardViewAsSkill("lihuo")
    {
        filter_pattern = "%slash";
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player);
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
            && pattern == "slash";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Card *acard = new FireSlash(originalCard->getSuit(), originalCard->getNumber());
        acard->addSubcard(originalCard->getId());
        acard->setSkillName(objectName());
        return acard;
    }
};

class Lihuo : public TriggerSkill
{
public:
    Lihuo() : TriggerSkill("lihuo")
    {
        events << PreDamageDone << CardFinished;
        view_as_skill = new LihuoViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->getSkillName() == objectName()) {
                QVariantList slash_list = damage.from->tag["InvokeLihuo"].toList();
                slash_list << QVariant::fromValue(damage.card);
                damage.from->tag["InvokeLihuo"] = QVariant::fromValue(slash_list);
            }
        } else if (TriggerSkill::triggerable(player) && !player->hasFlag("Global_ProcessBroken")) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;

            bool can_invoke = false;
            QVariantList slash_list = use.from->tag["InvokeLihuo"].toList();
            foreach (QVariant card, slash_list) {
                if (card.value<const Card *>() == use.card) {
                    can_invoke = true;
                    slash_list.removeOne(card);
                    use.from->tag["InvokeLihuo"] = QVariant::fromValue(slash_list);
                    break;
                }
            }
            if (!can_invoke) return false;

            room->broadcastSkillInvoke("lihuo", 2);
            room->sendCompulsoryTriggerLog(player, objectName());
            room->loseHp(player, 1);
        }
        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return 1;
    }
};

class LihuoTargetMod : public TargetModSkill
{
public:
    LihuoTargetMod() : TargetModSkill("#lihuo-target")
    {
        frequency = NotFrequent;
    }

    int getExtraTargetNum(const Player *from, const Card *card) const
    {
        if (from->hasSkill("lihuo") && card->isKindOf("FireSlash"))
            return 1;
        else
            return 0;
    }
};

ChunlaoCard::ChunlaoCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void ChunlaoCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->addToPile("wine", this);
}

ChunlaoWineCard::ChunlaoWineCard()
{
    m_skillName = "chunlao";
    mute = true;
    target_fixed = true;
    will_throw = false;
}

void ChunlaoWineCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &) const
{
    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (!who) return;

    if (subcards.length() != 0) {
        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "chunlao", QString());
        DummyCard *dummy = new DummyCard(subcards);
        room->throwCard(dummy, reason, NULL);
        delete dummy;
        Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
        analeptic->setSkillName("_chunlao");
        room->useCard(CardUseStruct(analeptic, who, who, false));
    }
}

class ChunlaoViewAsSkill : public ViewAsSkill
{
public:
    ChunlaoViewAsSkill() : ViewAsSkill("chunlao")
    {
        expand_pile = "wine";
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "@@chunlao"
            || (pattern.contains("peach") && !player->getPile("wine").isEmpty());
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@@chunlao")
            return to_select->isKindOf("Slash");
        else {
            ExpPattern pattern(".|.|.|wine");
            if (!pattern.match(Self, to_select)) return false;
            return selected.length() == 0;
        }
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@@chunlao") {
            if (cards.length() == 0) return NULL;

            Card *acard = new ChunlaoCard;
            acard->addSubcards(cards);
            acard->setSkillName(objectName());
            return acard;
        } else {
            if (cards.length() != 1) return NULL;
            Card *wine = new ChunlaoWineCard;
            wine->addSubcards(cards);
            wine->setSkillName(objectName());
            return wine;
        }
    }
};

class Chunlao : public TriggerSkill
{
public:
    Chunlao() : TriggerSkill("chunlao")
    {
        events << EventPhaseStart;
        view_as_skill = new ChunlaoViewAsSkill;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *chengpu, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && chengpu->getPhase() == Player::Finish
            && !chengpu->isKongcheng() && chengpu->getPile("wine").isEmpty()) {
            room->askForUseCard(chengpu, "@@chunlao", "@chunlao", -1, Card::MethodNone);
        }
        return false;
    }

    int getEffectIndex(const ServerPlayer *player, const Card *card) const
    {
        if (card->isKindOf("Analeptic")) {
            if (player->getGeneralName().contains("zhouyu"))
                return 3;
            else
                return 2;
        } else
            return 1;
    }
};

YJCM2012Package::YJCM2012Package()
    : Package("YJCM2012")
{
    General *bulianshi = new General(this, "bulianshi", "wu", 3, false); // YJ 101
    bulianshi->addSkill(new Anxu);
    bulianshi->addSkill(new Zhuiyi);

    General *caozhang = new General(this, "caozhang", "wei"); // YJ 102
    caozhang->addSkill(new Jiangchi);
    caozhang->addSkill(new JiangchiTargetMod);
    caozhang->addSkill(new JiangchiEffect);
    related_skills.insertMulti("jiangchi", "#jiangchi-target");
    related_skills.insertMulti("jiangchi", "#jiangchi-effect2");

    General *chengpu = new General(this, "chengpu", "wu"); // YJ 103
    chengpu->addSkill(new Lihuo);
    chengpu->addSkill(new LihuoTargetMod);
    chengpu->addSkill(new Chunlao);
    related_skills.insertMulti("lihuo", "#lihuo-target");

    General *guanxingzhangbao = new General(this, "guanxingzhangbao", "shu"); // YJ 104
    guanxingzhangbao->addSkill(new Fuhun);

    General *handang = new General(this, "handang", "wu"); // YJ 105
    handang->addSkill(new Gongqi);
    handang->addSkill(new Jiefan);

    General *huaxiong = new General(this, "huaxiong", "qun", 6); // YJ 106
    huaxiong->addSkill(new Shiyong);
    huaxiong->addSkill(new YJYaowu);

    General *liaohua = new General(this, "liaohua", "shu"); // YJ 107
    liaohua->addSkill(new Dangxian);
    //liaohua->addSkill(new DangxianFilter);
    //related_skills.insertMulti("dangxian", "#dangxian-f");
    liaohua->addSkill(new Fuli);

    General *liubiao = new General(this, "liubiao", "qun", 4); // YJ 108
    liubiao->addSkill(new Zishou);
    liubiao->addSkill(new Zongshi);

    General *madai = new General(this, "madai", "shu"); // YJ 109
    madai->addSkill("mashu");
    madai->addSkill(new Qianxi);
    madai->addSkill(new QianxiClear);
    related_skills.insertMulti("qianxi", "#qianxi-clear");

    General *wangyi = new General(this, "wangyi", "wei", 3, false); // YJ 110
    wangyi->addSkill(new Zhenlie);
    wangyi->addSkill(new Miji);

    General *xunyou = new General(this, "xunyou", "wei", 3); // YJ 111
    xunyou->addSkill(new Qice);
    xunyou->addSkill(new Zhiyu);

    skills << new DangxianTriggerEnd;

    addMetaObject<QiceCard>();
    addMetaObject<ChunlaoCard>();
    addMetaObject<ChunlaoWineCard>();
    addMetaObject<GongqiCard>();
    addMetaObject<JiefanCard>();
    addMetaObject<AnxuCard>();
    addMetaObject<YJYaowuCard>();
}

ADD_PACKAGE(YJCM2012)

