#include "skgenerals.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "god.h"
#include "standard.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "clientstruct.h"
#include "util.h"
#include "wrapped-card.h"
#include "room.h"
#include "roomthread.h"


class SKZiguoMaxCardsSkill : public MaxCardsSkill
{
public:
    SKZiguoMaxCardsSkill() : MaxCardsSkill("#skziguo2")
    {
    }

    int getExtra(const Player *target) const
    {
        return target->getMark("skziguo_mark") > 0 ? -2 : 0;
    }
};

class SKZiguoTriggerSkill : public TriggerSkill
{
public:
    SKZiguoTriggerSkill() : TriggerSkill("#skziguo")
    {
        events << EventPhaseChanging;
        frequency = Skill::Compulsory;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;

        QList<ServerPlayer *> players = room->getAllPlayers();
        foreach (ServerPlayer *player, players) {
            if (player->getMark("skziguo_mark") == 0) continue;
            player->removeMark("skziguo_mark");
        }
        return false;
    }
};

SKZiguoCard::SKZiguoCard()
{
    will_throw = false;
    target_fixed = false;
    handling_method = Card::MethodNone;
}

bool SKZiguoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (targets.size() > 0)
        return false;

    return to_select->isWounded();
}

void SKZiguoCard::use(Room *, ServerPlayer *player, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    target->drawCards(2, "skziguo");
    player->setMark("skziguo_mark", 1);
}

class SKZiguo : public ZeroCardViewAsSkill
{
public:
    SKZiguo() : ZeroCardViewAsSkill("skziguo")
    {
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("SKZiguoCard");
    }

    const Card *viewAs() const
    {
        Card *skcard = new SKZiguoCard;
        skcard->setSkillName(objectName());
        return skcard;
    }
};

class SKShangdao : public TriggerSkill
{
public:
    SKShangdao() : TriggerSkill("skshangdao")
    {
        events << EventPhaseStart;
        frequency = Skill::Compulsory;
        mustskillowner = false;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Start) {
            QList<ServerPlayer *> mizhus = room->findPlayersBySkillName(objectName());
            if (mizhus.size() == 0)
                return false;

            foreach (ServerPlayer *mizhu, mizhus) {
                if (player == mizhu)
                    continue;

                if (player->getHandcardNum() > mizhu->getHandcardNum())
                {
                    room->broadcastSkillInvoke(objectName());

                    QList<int> ids = room->getNCards(1, false);
                    CardsMoveStruct move(ids, mizhu, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, mizhu->objectName(), objectName(), QString()));
                    room->moveCardsAtomic(move, true);

                    mizhu->obtainCard(Sanguosha->getCard(ids.first()));
                }
            }
        }

        return false;
    }
};

SKGuhuoDialog *SKGuhuoDialog::getInstance(const QString &object)
{
    static SKGuhuoDialog *instance;
    if (instance == NULL || instance->objectName() != object)
        instance = new SKGuhuoDialog(object);

    return instance;
}

SKGuhuoDialog::SKGuhuoDialog(const QString &object)
    : GuhuoDialog(object, true, true, false, false, false)
{
}


SKGuhuoCard::SKGuhuoCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool SKGuhuoCard::targetFixed() const
{
    const Card *card = Self->tag.value("skguhuo").value<const Card *>();
    Card *mutable_card = Sanguosha->cloneCard(card);
    if (mutable_card) {
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetFixed();
}

bool SKGuhuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *card = Self->tag.value("skguhuo").value<const Card *>();
    Card *mutable_card = Sanguosha->cloneCard(card);
    if (mutable_card) {
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    ClientPlayer *p = ClientInstance->getPlayer(Self->property("SKGuhuoTarget").toString());
    if (p && p->isAlive()) {
        return mutable_card && mutable_card->targetFilter(targets, to_select, p) && !Self->isProhibited(to_select, mutable_card, targets);
    }
    return mutable_card && mutable_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, mutable_card, targets);
}

bool SKGuhuoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Card *card = Self->tag.value("skguhuo").value<const Card *>();
    Card *mutable_card = Sanguosha->cloneCard(card);
    if (mutable_card) {
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    ClientPlayer *p = ClientInstance->getPlayer(Self->property("SKGuhuoTarget").toString());
    if (p && p->isAlive()) {
        return mutable_card && mutable_card->targetsFeasible(targets, p);
    }
    return mutable_card && mutable_card->targetsFeasible(targets, Self);
}

const Card *SKGuhuoCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *realuser = card_use.from->tag.value("SKGuhuoTarget", QVariant::fromValue((ServerPlayer *)NULL)).value<ServerPlayer *>();
    if (realuser)
        card_use.from = realuser;
    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("skguhuo");
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

class SKGuhuoViewAsSkill : public ZeroCardViewAsSkill
{
public:
    SKGuhuoViewAsSkill() : ZeroCardViewAsSkill("skguhuo")
    {
        response_pattern = "@@skguhuo";
    }

    QDialog *getDialog() const
    {
        return SKGuhuoDialog::getInstance(objectName());
    }

    const Card *viewAs() const
    {
        const Card *choose = Self->tag.value("skguhuo").value<const Card *>();
        if (!choose)
            return NULL;

        SKGuhuoCard *skcard = new SKGuhuoCard;
        skcard->setUserString(choose->objectName());
        skcard->setSkillName(objectName());
        return skcard;
    }
};

class SKGuhuo : public TriggerSkill
{
public:
    SKGuhuo() : TriggerSkill("skguhuo")
    {
        events << EventPhaseStart;
        frequency = Skill::NotFrequent;
        mustskillowner = false;
        view_as_skill = new SKGuhuoViewAsSkill;
    }

    QDialog *getDialog() const
    {
        return SKGuhuoDialog::getInstance(objectName());
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) {
            QList<ServerPlayer *> yujis = room->findPlayersBySkillName(objectName());
            if (yujis.size() == 0)
                return false;

            foreach (ServerPlayer *yuji, yujis)
            {
                if (player == yuji || !yuji->isAlive())
                    continue;

                if (player->getHandcardNum() > 0 && yuji->getHandcardNum() > 0)
                {
                    QVariant data = QVariant::fromValue(player);
                    if (yuji->askForSkillInvoke(objectName(), data)) {
                        bool success = yuji->pindian(player, objectName(), NULL);
                        if (success) {
                            yuji->tag["SKGuhuoTarget"] = data;
                            room->setPlayerProperty(yuji, "SKGuhuoTarget", QVariant::fromValue(player->objectName()));
                            //room->setClientCustomData(yuji->objectName(), player->objectName(), 0, 0);
                            room->askForUseCard(yuji, "@@skguhuo", "skguhuo-invoke", -1, Card::MethodNone);
                            yuji->tag.remove("SKGuhuoTarget");
                        }
                        else {
                            room->damage(DamageStruct(objectName(), player, yuji, 1));
                        }
                    }
                }
            }
        }

        return false;
    }
};

class SKFulu : public TriggerSkill
{
public:
    SKFulu() : TriggerSkill("skfulu")
    {
        events << Damaged << HpRecover;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Damaged && TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *from = damage.from;

            QQueue<ServerPlayer *> dqueue;
            dqueue = player->tag.value("SKFuluDamage", QVariant::fromValue(QQueue<ServerPlayer *>())).value<QQueue<ServerPlayer *> >();
            QQueue<ServerPlayer *> rqueue = player->tag.value("SKFuluRecover", QVariant::fromValue(QQueue<ServerPlayer *>())).value<QQueue<ServerPlayer *> >();

            if (from) {
                if (dqueue.size() >= 3) {
                    dqueue.dequeue();
                }
                dqueue.enqueue(from);
                player->tag["SKFuluDamage"] = QVariant::fromValue(dqueue);
            }

            // for AI
            QStringList stringlist;
            for (int i = 0; i < 3; ++i) {
                if (i >= dqueue.size())
                    stringlist << "";
                else
                    stringlist << dqueue.at(i)->objectName();
            }
            QString playerinfo1 = stringlist.join("[P]");
            for (int i = 0; i < 3; ++i) {
                if (i >= rqueue.size())
                    stringlist << "";
                else
                    stringlist << rqueue.at(i)->objectName();
            }
            QStringList siii = stringlist.mid(3);
            QString playerinfo2 = siii.join("[P]");

            for (int i = 0; i < damage.damage; i++) {
                //if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(stringlist))) {
                if (room->askForSimpleChoice(player, objectName(), QString("hint:::[P]%1:[P]%2").arg(playerinfo1).arg(playerinfo2), QVariant::fromValue(stringlist))) {
                    room->broadcastSkillInvoke(objectName());
                    foreach (ServerPlayer *p, dqueue) {
                        QList<const Card *> cards = room->getDiscardableCardsOfPlayer(player, p);
                        int cards_size = cards.size();
                        if (cards_size) {
                            room->throwCard(cards[qrand() % cards_size], p, player);
                        }
                    }
                    foreach (ServerPlayer *p, rqueue) {
                        p->drawCards(1, objectName());
                    }
                }
            }
        } else if (triggerEvent == HpRecover) {
            RecoverStruct recover_struct = data.value<RecoverStruct>();
            ServerPlayer *who = recover_struct.who;
            if (!who)
                return false;
            QQueue<ServerPlayer *> rqueue = player->tag.value("SKFuluRecover", QVariant::fromValue(QQueue<ServerPlayer *>())).value<QQueue<ServerPlayer *> >();
            if (rqueue.size() >= 3) {
                rqueue.dequeue();
            }
            rqueue.enqueue(who);
            player->tag["SKFuluRecover"] = QVariant::fromValue(rqueue);
        }
        return false;
    }
};

class SKFuluRecord : public TriggerSkill
{
public:
    SKFuluRecord() : TriggerSkill("#skfulu-record")
    {
        events << Damaged << HpRecover;
        mustskillowner = false;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *from = damage.from;

            QQueue<ServerPlayer *> dqueue;
            dqueue = player->tag.value("SKFuluDamage", QVariant::fromValue(QQueue<ServerPlayer *>())).value<QQueue<ServerPlayer *> >();
            QQueue<ServerPlayer *> rqueue = player->tag.value("SKFuluRecover", QVariant::fromValue(QQueue<ServerPlayer *>())).value<QQueue<ServerPlayer *> >();

            if (from) {
                if (dqueue.size() >= 3) {
                    dqueue.dequeue();
                }
                dqueue.enqueue(from);
                player->tag["SKFuluDamage"] = QVariant::fromValue(dqueue);
            }

            // for AI
            QStringList stringlist;
            for (int i = 0; i < 3; ++i) {
                if (i >= dqueue.size())
                    stringlist << "";
                else
                    stringlist << dqueue.at(i)->objectName();
            }
            for (int i = 0; i < 3; ++i) {
                if (i >= rqueue.size())
                    stringlist << "";
                else
                    stringlist << rqueue.at(i)->objectName();
            }

            for (int i = 0; i < damage.damage; i++) {
                if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(stringlist))) {
                    room->broadcastSkillInvoke(objectName());
                    foreach (ServerPlayer *p, dqueue) {
                        QList<const Card *> cards = room->getDiscardableCardsOfPlayer(player, p);
                        int cards_size = cards.size();
                        if (cards_size) {
                            room->throwCard(cards[qrand() % cards_size], p, player);
                        }
                    }
                    foreach (ServerPlayer *p, rqueue) {
                        p->drawCards(1, objectName());
                    }
                }
            }
        } else if (triggerEvent == HpRecover) {
            RecoverStruct recover_struct = data.value<RecoverStruct>();
            ServerPlayer *who = recover_struct.who;
            if (!who)
                return false;
            QQueue<ServerPlayer *> rqueue = player->tag.value("SKFuluRecover", QVariant::fromValue(QQueue<ServerPlayer *>())).value<QQueue<ServerPlayer *> >();
            if (rqueue.size() >= 3) {
                rqueue.dequeue();
            }
            rqueue.enqueue(who);
            player->tag["SKFuluRecover"] = QVariant::fromValue(rqueue);
        }
        return false;
    }
};

SKShemiCard::SKShemiCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void SKShemiCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *player = card_use.from;

    CardMoveReason movereason;
    movereason.m_playerId = player->objectName();
    movereason.m_skillName = card_use.card->getSkillName();
    movereason.m_reason = CardMoveReason::S_REASON_DISCARD;

    room->throwCards(subcards, movereason, player);
}

class SKShemiViewAsSkill : public ViewAsSkill
{
public:
    SKShemiViewAsSkill() : ViewAsSkill("skshemi")
    {
        response_pattern = "@@skshemi";
    }

    bool viewFilter(const QList<const Card *> &, const Card *) const
    {
        return true;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        Card *skcard = new SKShemiCard;
        skcard->addSubcards(cards);
        skcard->setSkillName("skshemi");
        return skcard;
    }
};

class SKShemiEffect : public DrawCardsSkill
{
public:
    SKShemiEffect() : DrawCardsSkill("#skshemi-effect")
    {
        frequency = Compulsory;
    }

    int getDrawNum(ServerPlayer *player, int n) const
    {
        if (player->hasSkill("skshemi")) {
            int mm = player->getMark("@skshemi");
            if (mm > 0) {
                Room *room = player->getRoom();
                room->broadcastSkillInvoke("skshemi");
                room->sendCompulsoryTriggerLog(player, "skshemi");
                return n + 1;
            }
        }

        return n;
    }
};

class SKShemiTargetSkill : public TargetModSkill
{
public:
    SKShemiTargetSkill() : TargetModSkill("#shemi-target")
    {
        pattern = ".";
    }

    int getResidueNum(const Player *from, const Card *) const
    {
        return from->hasSkill("skshemi") ? from->getMark("@skshemi") : 0;
    }
};

class SKShemi : public TriggerSkill
{
public:
    SKShemi() : TriggerSkill("skshemi")
    {
        events << EventPhaseEnd << CardsMoveOneTime;
        view_as_skill = new SKShemiViewAsSkill;
        frequency = Skill::Compulsory;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Discard) {

            const Card *discard = NULL;
            int discard_num = player->getHandcardNum() + player->getEquips().size();
            //bool result = false;
            if (discard_num > 0) {
                //result = room->askForDiscard(player, objectName(), discard_num, 0, true, true, "skshemi-discard");
                discard = room->askForCard(player, "@@skshemi", "skshemi-discard");
                //if (discard) {
                //    room->broadcastSkillInvoke(objectName());
                //    CardMoveReason movereason;
                //    movereason.m_playerId = player->objectName();
                //    movereason.m_skillName = objectName();
                //    movereason.m_reason = CardMoveReason::S_REASON_DISCARD;
                //    room->throwCards(discard->getSubcards(), movereason, player);
                //}
            }

            if (discard)
            //if (result)
                room->broadcastSkillInvoke(objectName());

            int last = player->tag.value("SKShemiDiscardNum2", 0).toInt(); // from -1 to 0
            int x = player->tag.value("SKShemiDiscardNum", 0).toInt();
            player->tag["SKShemiDiscardNum2"] = QVariant::fromValue(x);
            player->tag["SKShemiDiscardNum"] = QVariant::fromValue(0);
            //room->setPlayerProperty(player, "discard_count", QVariant::fromValue(x));
            room->setPlayerMark(player, "@discard", x);

            if (last >= 0) {
                if (last < x) {
                    player->gainMark("@skshemi", 1);
                } else {
                    player->loseMark("@skshemi", 1);
                }
            }

        } else if (triggerEvent == CardsMoveOneTime) {

            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (player && player->isAlive() && player->getPhase() == Player::Discard
                // in some cases like zhangzhao's 'guzheng', move.from is null, which makes this crash.
                && (move.from && move.from->objectName() == player->objectName())
                && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                /*&& (move.reason.m_reason == CardMoveReason::S_REASON_RULEDISCARD)*/) {

                player->tag["SKShemiDiscardNum"] = QVariant::fromValue(player->tag.value("SKShemiDiscardNum", 0).toInt() + move.card_ids.size());
            }
        }
        return false;
    }
};

class SKJiaohui : public TriggerSkill
{
public:
    SKJiaohui() : TriggerSkill("skjiaohui")
    {
        events << DamageForseen;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        if (damage.damage > 0 && room->askForSkillInvoke(player, objectName(), data)) {
            QStringList choicelist;
            choicelist << "draw";
            if (player->getHandcardNum() + player->getEquips().size() > 0)
                choicelist << "discard";
            QString choice = room->askForChoice(player, objectName(), choicelist.join("+"), data);
            if (choice == "draw") {
                room->broadcastSkillInvoke(objectName(), 1);
                player->drawCards(1, objectName());
            } else if (choice == "discard") {
                room->broadcastSkillInvoke(objectName(), 2);
                room->askForDiscard(player, objectName(), 1, 1, false, true);
            }
            if (player->getHandcardNum() == player->getHp()) {

                LogMessage log;
                log.type = "#SKJiaohui";
                log.from = player;
                log.arg2 = QString::number(damage.damage--);
                log.arg = QString::number(damage.damage);
                room->sendLog(log);

                data = QVariant::fromValue(damage);
            }
            return damage.damage < 1;
        }

        return false;
    }
};

class SKWengua : public TriggerSkill
{
public:
    SKWengua() : TriggerSkill("skwengua")
    {
        events << EventPhaseStart;
        frequency = Skill::NotFrequent;
        mustskillowner = false;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::RoundStart)
            return false;

        QList<ServerPlayer *> skxushis = room->findPlayersBySkillName(objectName());
        if (skxushis.size() == 0)
            return false;

        foreach (ServerPlayer *skxushi, skxushis) {
            if (!skxushi->isAlive())
                continue;

            const Card *card;
            if (skxushi != player)
            {
                card = room->askForExchange(player, objectName(), 1, 1, true, "skwengua-give::" + skxushi->objectName(), true, QString());
                if (!card)
                    continue;

                room->notifySkillInvoked(skxushi, objectName());
                room->broadcastSkillInvoke(objectName(), 1);
                CardMoveReason movereason(CardMoveReason::S_REASON_GIVE, player->objectName(), skxushi->objectName(), objectName(), QString());
                room->moveCardTo(card, player, skxushi, Player::PlaceHand, movereason);
                delete card;
            }

            // for AI
            room->setPlayerFlag(skxushi, "SKWenguaPut");
            card = room->askForExchange(skxushi, objectName(), 1, 1, true, "skwengua-invoke", true);
            room->setPlayerFlag(skxushi, "-SKWenguaPut");

            if (!card)
                continue;

            room->notifySkillInvoked(skxushi, objectName());
            room->broadcastSkillInvoke(objectName(), 2);
            CardMoveReason reason(CardMoveReason::S_REASON_PUT, skxushi->objectName(), objectName(), QString());
            room->moveCardTo(card, skxushi, NULL, Player::DrawPileBottom, reason, false);
            delete card;

            room->drawCards(skxushi, 1, objectName());
            room->drawCards(player, 1, objectName());
        }
        return false;
    }
};

SKFuzhuCard::SKFuzhuCard()
{
    mute = true;
    will_throw = false;
    target_fixed = false;
}

bool SKFuzhuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (targets.size() > 0)
        return false;
    return Self->canSlash(to_select, true);
}

void SKFuzhuCard::use(Room *room, ServerPlayer *player, QList<ServerPlayer *> &targets) const
{
    room->broadcastSkillInvoke("skfuzhu");
    room->notifySkillInvoked(player, "skfuzhu");
    room->doSuperLightbox("sk_xushi", "skfuzhu");
    room->removePlayerMark(player, "@skfuzhu");

    ServerPlayer *target = targets.first();
    do {
        QList<int> ids = room->getNCardsBottom(1, false);
        CardsMoveStruct move(ids, player, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "skfuzhu", QString()));
        room->moveCardsAtomic(move, true);

        Card *card = room->getCard(ids.first());
        if (card->isBlack()) {
            Slash *slash = new Slash(card->getSuit(), card->getNumber());
            slash->setSkillName("skfuzhu");
            slash->addSubcard(card);
            room->useCard(CardUseStruct(slash, player, target));
        }
        room->throwCard(card, CardMoveReason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "skfuzhu", QString()), NULL);
        if (!card->isBlack())
            break;
        if (!target->isAlive())
            break;

        room->getThread()->delay();

    } while (true);
}

class SKFuzhu : public ZeroCardViewAsSkill
{
public:
    SKFuzhu() : ZeroCardViewAsSkill("skfuzhu")
    {
        response_or_use = true;
        frequency = Limited;
        limit_mark = "@skfuzhu";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@skfuzhu") > 0;
    }

    const Card *viewAs() const
    {
        Card *skcard = new SKFuzhuCard;
        skcard->setSkillName("skfuzhu");
        return skcard;
    }
};

SKSijianCard::SKSijianCard()
{
    mute = true;
    will_throw = true;
    target_fixed = false;
}

bool SKSijianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty())
        return false;

    return to_select != Self;
}

void SKSijianCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *target = effect.to;
    ServerPlayer *player = effect.from;
    Room *room = target->getRoom();

    room->notifySkillInvoked(player, "sksijian");
    room->broadcastSkillInvoke("sksijian");
    target->drawCards(player->getMaxHp(), "sksijian");
    room->showAllCards(target);

    QMap<Card::Suit, bool> smap;
    smap.insert(Card::Spade, false);
    smap.insert(Card::Club, false);
    smap.insert(Card::Heart, false);
    smap.insert(Card::Diamond, false);
    foreach (const Card *card, target->getHandcards())
    {
        smap[card->getSuit()] = true;
    }
    if (!(smap[Card::Spade] && smap[Card::Club] && smap[Card::Heart] && smap[Card::Diamond]))
    {
        room->loseHp(player);
    }
}

class SKSijian : public ZeroCardViewAsSkill
{
public:
    SKSijian() : ZeroCardViewAsSkill("sksijian")
    {
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("SKSijianCard");
    }

    const Card *viewAs() const
    {
        Card *skcard = new SKSijianCard;
        skcard->setSkillName(objectName());
        return skcard;
    }
};

class SKGangzhi : public TriggerSkill
{
public:
    SKGangzhi() : TriggerSkill("skgangzhi")
    {
        events << PreHpLost << DamageInflicted;
        frequency = Skill::NotFrequent;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        int damage = 0;
        if (data.canConvert<DamageStruct>())
            damage = data.value<DamageStruct>().damage;
        else
            damage = data.toInt();
        if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(damage)))
        {
            room->broadcastSkillInvoke(objectName());
            player->drawCards(3, objectName());
            room->loseMaxHp(player, 1);
            return true;
        }
        return false;
    }
};

class SKShanxi : public ProhibitSkill
{
public:
    SKShanxi() : ProhibitSkill("skshanxi")
    {
    }

    bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->hasSkill(this) && card->isKindOf("Lightning");
    }
};

class SKShanxiEffect : public TriggerSkill
{
public:
    SKShanxiEffect() : TriggerSkill("#skshanxi-effect")
    {
        frequency = Skill::Compulsory;
        events << FinishJudge;
        mustskillowner = false;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (judge->reason != "lightning")
            return false;

        const Card *card = judge->card;

        foreach (ServerPlayer *skzn, room->findPlayersBySkillName("skshanxi"))
        {
            if (!skzn->isAlive())
                continue;

            if (room->getCardPlace(card->getEffectiveId()) == Player::PlaceJudge) {

                room->broadcastSkillInvoke("skshanxi");
                skzn->obtainCard(judge->card);
            }
            break;
        }

        return false;
    }
};

class SKLeiji : public TriggerSkill
{
public:
    SKLeiji() : TriggerSkill("skleiji")
    {
        events << CardResponded;
        frequency = Skill::NotFrequent;
        mustskillowner = false;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardResponseStruct card_resp = data.value<CardResponseStruct>();
        const Card *card_star = card_resp.m_card;
        if (!card_star->isKindOf("Jink") || !card_resp.m_isUse) return false;

        QList<ServerPlayer *> players = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *skzn, players)
        {
            if (!skzn->isAlive() || player == skzn)
                continue;

            int card_id = room->newGetCardFromPile("lightning", true);
            if (card_id < 0)
            {
                LogMessage log;
                log.type = "#skleiji-negative";
                log.from = skzn;
                room->sendLog(log);
                return false;
            }

            if (!room->askForSkillInvoke(skzn, objectName()))
                continue;

            QList<ServerPlayer *> targets = room->getOtherPlayers(skzn, false);
            Lightning *dummy = new Lightning(Card::NoSuit, 0);
            for (int i = 0; i < targets.size(); ) {
                if (!dummy->isAvailable(targets[i]))
                    targets.removeAt(i);
                else
                    ++i;
            }
            delete dummy;

            if (targets.isEmpty())
                continue;

            const Card *lightning = Sanguosha->getCard(card_id);
            ServerPlayer *target = room->askForPlayerChosen(skzn, targets, objectName(), "skleiji-invoke", true, true);
            if (target && target->isAlive() && lightning->isAvailable(target))
            {
                room->broadcastSkillInvoke(objectName());

                LogMessage log;
                log.from = skzn;
                log.to << target;
                log.type = "#skleiji-sucess";
                log.card_str = lightning->toString();
                room->sendLog(log);

                CardMoveReason reason(CardMoveReason::S_REASON_UNKNOWN, skzn->objectName(), target->objectName(), objectName(), QString());
                room->moveCardTo(lightning, target, Player::PlaceDelayedTrick, reason, true);
            }
        }

        return false;
    }
};

class SKTaoluan : public TriggerSkill
{
public:
    SKTaoluan() : TriggerSkill("sktaoluan")
    {
        events << CardUsed << TurnStart << EventPhaseStart;
        frequency = Skill::NotFrequent;
        mustskillowner = false;
    }

    inline bool isCardValid(const Card *card) const
    {
        return card->isBasicOrNonDelayedTrick() && !card->isKindOf("Jink") && !card->isKindOf("Nullification");
    }

    inline QStringList getLeft(const Card *card_org, const QStringList &used_flag) const
    {
        QMap<QString, bool> map;
        QStringList list;
        static QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        foreach (const Card *card, cards) {
            if (card->getTypeId() == card_org->getTypeId()
                && isCardValid(card)
                && !map.contains(card->objectName())
                && !ServerInfo.Extensions.contains("!" + card->getPackage())
                && !used_flag.contains(card->objectName())
                && card->objectName() != card_org->objectName()) {
                list << card->objectName();
                map[card->objectName()] = true;
            }
        }
        return list;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardUsed) {
            if (room->getCurrentDyingPlayer()) return false;

            CardUseStruct card_use = data.value<CardUseStruct>();
            const Card *card = card_use.card;
            if (!isCardValid(card) || card->getSkillName() == objectName() || !card_use.from) return false;

            QList<ServerPlayer *> players = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *skzr, players)
            {
                if (!skzr->isAlive() || skzr->getHandcardNum() < 1 || skzr->tag["sktaoluan_1"].toBool() || card_use.from->getHandcardNum() < skzr->getHandcardNum())
                    continue;

                QStringList used_flag = skzr->tag["sktaoluan"].toStringList();
                QStringList left = getLeft(card, used_flag);
                if (left.isEmpty() || !room->askForSkillInvoke(skzr, objectName(), data))
                    continue;

                if (room->askForDiscard(skzr, objectName(), 1, 1, true, false, QString("sktaoluan-promt:::::%1").arg(card->getEffectiveStringOrName()))) {
                    QString choice = room->askForTemplateCard(skzr, objectName(), left.join('+'), data);
                    used_flag << choice;
                    skzr->tag["sktaoluan_1"] = true;
                    skzr->tag["sktaoluan"] = used_flag;
                    room->broadcastSkillInvoke(objectName());

                    Card *wrap = Sanguosha->cloneCard(choice, card->getSuit(), card->getNumber());
                    if (!card->isVirtualCard())
                        wrap->addSubcard(card);
                    else if (card->subcardsLength() > 0) {
                        wrap->addSubcards(card->getSubcards());
                    }
                    wrap->setSkillName(objectName());

                    LogMessage log;
                    log.type = "#sktaoluan";
                    log.from = skzr;
                    log.card_str = card->toString();
                    log.arg = objectName();
                    log.arg2 = choice;
                    room->sendLog(log);

                    card_use.card = wrap;
                    card_use.m_isOwnerUse = card_use.from == skzr;
                    data = QVariant::fromValue(card_use);
                }
            }
        }
        else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::RoundStart) {
                QList<ServerPlayer *> players = room->findPlayersBySkillName(objectName());
                foreach (ServerPlayer *skzr, players)
                {
                    if (!skzr->isAlive())
                        continue;
                    skzr->tag["sktaoluan_1"] = false;
                }
            }
        }
        else if (player->hasSkill(this)) {
            player->tag["sktaoluan"] = QStringList();
        }

        return false;
    }
};

SKShejianCard::SKShejianCard()
{
    mute = true;
    will_throw = false;
    target_fixed = false;
}

bool SKShejianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty())
        return false;

    return to_select != Self && to_select->getCardCount() > 0 && !to_select->getMark("skshejian_e");
}

void SKShejianCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *target = effect.to;
    ServerPlayer *player = effect.from;
    Room *room = target->getRoom();

    room->setPlayerMark(target, "skshejian_e", 1);
    room->askForThrow(player, target, 1, "he", "skshejian");

    if (target->canSlash(player, false) && room->askForSimpleChoice(target, "skshejian", QString("invoke:%1").arg(player->objectName()), QVariant::fromValue(player))) {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("skshejian");
        room->useCard(CardUseStruct(slash, target, player));
    }
}

class SKShejian : public ZeroCardViewAsSkill
{
public:
    SKShejian() : ZeroCardViewAsSkill("skshejian")
    {
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getArmor();
    }

    const Card *viewAs() const
    {
        Card *skcard = new SKShejianCard;
        skcard->setSkillName(objectName());
        return skcard;
    }
};

class SKShejianEffect : public TriggerSkill
{
public:
    SKShejianEffect() : TriggerSkill("#skshejianeffect")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Play) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                room->setPlayerMark(p, "skshejian_e", 0);
            }
        }
        return false;
    }
};

class SKKuangao : public TriggerSkill
{
public:
    SKKuangao() : TriggerSkill("skkuangao")
    {
        events << PostCardEffected;
        frequency = Skill::NotFrequent;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        ServerPlayer *target = effect.from;
        if (effect.card->isKindOf("Slash") && target && effect.to == player &&
            room->askForSkillInvoke(player, objectName(), QVariant::fromValue(target))) {

            QStringList choicelist;
            int card1_length = player->getCardCount();
            if (card1_length > 0)
                choicelist << QString("discard:%1").arg(target->objectName());
            choicelist << QString("draw:%1").arg(target->objectName());
            QString choice = room->askForChoice(player, objectName(), choicelist.join("+"), QVariant::fromValue(target));

            if (choice == "draw") {
                room->broadcastSkillInvoke(objectName(), 2);
                int count = std::min(target->getMaxHp(), 5) - target->getHandcardNum();
                if (count > 0)
                    target->drawCards(count, objectName());
            } else if (choice == "discard") {
                room->broadcastSkillInvoke(objectName(), 1);
                int card2_length = target->getCardCount();

                room->askForDiscard(player, objectName(), card1_length, card1_length, false, true);
                room->askForDiscard(target, objectName(), card2_length, card2_length, false, true);

                if (card1_length >= card2_length) {
                    room->damage(DamageStruct(objectName(), player, target));
                }
            }
        }
        return false;
    }
};


class SKYingzi : public DrawCardsSkill
{
public:
    SKYingzi() : DrawCardsSkill("skyingzi")
    {
        frequency = Skill::NotFrequent;
    }

    int getDrawNum(ServerPlayer *zhouyu, int n) const
    {
        if (!zhouyu->isAlive() || !zhouyu->askForSkillInvoke(objectName()))
            return n;

        Room *room = zhouyu->getRoom();
        int index = qrand() % 2 + 1;
        if (!zhouyu->hasInnateSkill(this)) {
            if (zhouyu->hasSkill("hunzi"))
                index = 5;
            else if (zhouyu->hasSkill("mouduan"))
                index += 2;
        }
        room->broadcastSkillInvoke(objectName(), index);

        QList<int> drawlist;
        QList<Card::Suit> suitlist;
        Card *current_card = NULL;
        while (true) {
            QList<int> ids = room->getNCards(1, false);
            CardsMoveStruct move(ids, zhouyu, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, zhouyu->objectName(), "yingzi", QString()));
            room->moveCardsAtomic(move, true);

            current_card = Sanguosha->getCard(ids.first());
            Card::Suit suit = current_card->getSuit();
            if (!suitlist.contains(suit))
                suitlist.append(suit);

            if (suitlist.size() > 2)
                break;

            drawlist.append(current_card->getId());
        }
        //room->fillAG(drawlist, zhouyu);

        DummyCard dummy;
        dummy.addSubcards(drawlist);
        zhouyu->obtainCard(&dummy);
        dummy.clearSubcards();
        dummy.addSubcard(current_card);
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, zhouyu->objectName(), "yingzi", QString());
        room->throwCard(&dummy, reason, NULL);

        //room->clearAG(zhouyu);

        return 0;
    }
};

SKPackage::SKPackage() : Package("sk")
{
    // generals
    General *sk_mizhu = new General(this, "sk_mizhu", "shu", 3);
    sk_mizhu->addSkill(new SKZiguo);
    sk_mizhu->addSkill(new SKZiguoMaxCardsSkill);
    sk_mizhu->addSkill(new SKZiguoTriggerSkill);
    sk_mizhu->addSkill(new SKShangdao);
    related_skills.insertMulti("skziguo", "#skziguo");
    related_skills.insertMulti("skziguo", "#skziguo2");

    General *sk_yuji = new General(this, "sk_yuji", "qun", 3);
    sk_yuji->addSkill(new SKGuhuo);
    sk_yuji->addSkill(new SKFulu);

    General *sk_dongbai = new General(this, "sk_dongbai", "qun", 3, false);
    sk_dongbai->addSkill(new SKShemi);
    sk_dongbai->addSkill(new SKShemiEffect);
    related_skills.insertMulti("skshemi", "#skshemi-effect");
    sk_dongbai->addSkill(new SKJiaohui);

    General *sk_xushi = new General(this, "sk_xushi", "wu", 3, false);
    sk_xushi->addSkill(new SKWengua);
    sk_xushi->addSkill(new SKFuzhu);

    General *sk_tianfeng = new General(this, "sk_tianfeng", "qun", 3);
    sk_tianfeng->addSkill(new SKSijian);
    sk_tianfeng->addSkill(new SKGangzhi);

    General *sk_zhangning = new General(this, "sk_zhangning", "qun", 3, false);
    sk_zhangning->addSkill(new SKLeiji);
    sk_zhangning->addSkill(new SKShanxi);
    sk_zhangning->addSkill(new SKShanxiEffect);
    related_skills.insertMulti("skshanxi", "#skshanxi-effect");

    General *sk_zhangrang = new General(this, "sk_zhangrang", "qun", 3);
    sk_zhangrang->addSkill(new SKTaoluan);

    General *sk_miheng = new General(this, "sk_miheng", "qun", 3);
    sk_miheng->addSkill(new SKShejian);
    sk_miheng->addSkill(new SKShejianEffect);
    related_skills.insertMulti("skshejian", "#skshejianeffect");
    sk_miheng->addSkill(new SKKuangao);

    addMetaObject<SKZiguoCard>();
    addMetaObject<SKGuhuoCard>();
    addMetaObject<SKShemiCard>();
    addMetaObject<SKFuzhuCard>();
    addMetaObject<SKSijianCard>();
    addMetaObject<SKShejianCard>();

    skills << new SKShemiTargetSkill;
}


ADD_PACKAGE(SK)
