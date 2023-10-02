#include "wzboss.h"
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

class WZTriggerSkill : public TriggerSkill
{
public:
    WZTriggerSkill(const QString &name, int r_stage) : TriggerSkill(name), required_stage(r_stage) {}

    int required_stage;

    bool triggerable(const ServerPlayer *player, Room *) const { return triggerable(player); }

    bool triggerable(const ServerPlayer *player) const
    {
        return player != NULL && player->isAlive() && player->hasSkill(this) && player->getWZStage() >= required_stage;
    }
};

class WZZeroCardViewAsSkill : public ZeroCardViewAsSkill
{
public:
    WZZeroCardViewAsSkill(const QString &name, int r_stage) : ZeroCardViewAsSkill(name), required_stage(r_stage) { response_or_use = true; }

    int required_stage;

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getWZStage() >= required_stage && isEnabledAtPlayWZ(player);
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return player->getWZStage() >= required_stage && isEnabledAtResponseWZ(player, pattern);
    }

    virtual bool isEnabledAtPlayWZ(const Player *player) const { return ViewAsSkill::isEnabledAtPlay(player); }
    virtual bool isEnabledAtResponseWZ(const Player *player, const QString &pattern) const { return ViewAsSkill::isEnabledAtResponse(player, pattern); }
};

class WZOneCardViewAsSkill : public OneCardViewAsSkill
{
public:
    WZOneCardViewAsSkill(const QString &name, int r_stage) : OneCardViewAsSkill(name), required_stage(r_stage) { response_or_use = true; }

    int required_stage;

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getWZStage() >= required_stage && isEnabledAtPlayWZ(player);
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return player->getWZStage() >= required_stage && isEnabledAtResponseWZ(player, pattern);
    }

    virtual bool isEnabledAtPlayWZ(const Player *player) const { return ViewAsSkill::isEnabledAtPlay(player); }
    virtual bool isEnabledAtResponseWZ(const Player *player, const QString &pattern) const { return ViewAsSkill::isEnabledAtResponse(player, pattern); }
};

class WZYuanjun : public DrawCardsSkill
{
public:
    WZYuanjun() : DrawCardsSkill("wzyuanjun")
    {
        frequency = Skill::Compulsory;
    }

    int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());
        return n + 2;
    }
};

class WZQidun : public ProhibitSkill
{
public:
    WZQidun() : ProhibitSkill("wzqidun")
    {
    }

    bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->hasSkill(this) && (card->isKindOf("DelayedTrick") || card->isKindOf("IronChain")) && card->getSkillName() != "nosguhuo"; // Be care!!!!!!
    }
};

class WZTianwei : public TriggerSkill
{
public:
    WZTianwei() : TriggerSkill("wztianwei")
    {
        events << PreTurnedOver;
        frequency = Skill::Compulsory;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->faceUp()) {
            room->broadcastSkillInvoke(objectName());
            LogMessage log;
            log.type = "#WzTianwei";
            log.from = player;
            room->sendLog(log);
            return true;
        }
        return false;
    }
};

class WZFubing : public WZTriggerSkill
{
public:
    WZFubing() : WZTriggerSkill("wzfubing", 1)
    {
        events << EventPhaseProceeding;
        frequency = Skill::Compulsory;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Finish) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());
            room->drawCards(player, 2, objectName());
        }
        return false;
    }
};

class WZHuitian : public WZTriggerSkill
{
public:
    WZHuitian() : WZTriggerSkill("wzhuitian", 1)
    {
        events << EventPhaseProceeding;
        frequency = Skill::Compulsory;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::RoundStart) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());
            //room->drawCards(player, 1, objectName());
            room->recover(player, RecoverStruct(player, NULL, 1), true);
        }
        return false;
    }
};

class WZXiaoshou : public MasochismSkill
{
public:
    WZXiaoshou() : MasochismSkill("wzxiaoshou")
    {
    }

    void onDamaged(ServerPlayer *player, const DamageStruct &damage) const
    {
        ServerPlayer *from = damage.from;
        Room *room = player->getRoom();
        if (from->getEquips().length() > 0 && room->askForSkillInvoke(player, objectName())) {
            room->broadcastSkillInvoke(objectName());
            int card_id = room->askForCardChosen(player, from, "e", objectName());
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
            const Card *card = Sanguosha->getCard(card_id);
            room->obtainCard(player, card, reason, room->getCardPlace(card_id) != Player::PlaceHand);
            //if (card->getTypeId() == Card::TypeEquip && room->getCardOwner(card_id) == player && !player->isLocked(card)) {
            //    player->tag["WZXiaoShouGotCard"] = QVariant::fromValue(card);
            //    bool will_use = room->askForSkillInvoke(player, "wzxiaoshou_use", "use");
            //    player->tag.remove("WZXiaoShouGotCard");
            //    if (will_use)
            //        room->useCard(CardUseStruct(card, player, player));
            //}
        }
    }
};

WZFenweiCard::WZFenweiCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool WZFenweiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < Self->getEquips().length() && Self->canSlash(to_select, false);
}

void WZFenweiCard::use(Room *room, ServerPlayer *player, QList<ServerPlayer *> &targets) const
{
    player->throwAllEquips();
    foreach (ServerPlayer *p, targets) {
        if (p->isAlive()) {
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("wzfenwei");
            room->useCard(CardUseStruct(slash, player, p));
        }
    }
}

class WZFenwei : public ZeroCardViewAsSkill
{
public:
    WZFenwei() : ZeroCardViewAsSkill("wzfenwei")
    {
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getEquips().length() > 0 && !player->hasUsed("WZFenweiCard");
    }

    const Card *viewAs() const
    {
        Card *card = new WZFenweiCard;
        card->setSkillName(objectName());
        return card;
    }
};

class WZFenweiTrigger : public TriggerSkill
{
public:
    WZFenweiTrigger() : TriggerSkill("#wzfenwei-trigger")
    {
        events << Damage;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->getSkillName() == "wzfenwei") {
            room->broadcastSkillInvoke("wzfenwei");
            room->notifySkillInvoked(player, "wzfenwei");
            room->drawCards(player, 1, "wzfenwei");
        }
        return false;
    }
};

class WZShipo : public TriggerSkill
{
public:
    WZShipo() : TriggerSkill("wzshipo")
    {
        events << CardFinished;// << EventPhaseStart;
        mustskillowner = false;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *, QVariant &data) const
    {
        //QList<ServerPlayer *> players = room->findPlayersBySkillName(objectName());
        //if (triggerEvent == EventPhaseStart) {
        //    if (player->getPhase() == Player::RoundStart) {
        //        foreach (ServerPlayer *p, players) {
        //            if (!p->isAlive()) continue;
        //            p->tag["wzshipo-t"] = false;
        //        }
        //    }
        //} else {
            QList<ServerPlayer *> players = room->findPlayersBySkillName(objectName());
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && !use.card->isSkillOrDummy() && use.card->isBlack() && use.from && use.from->isAlive()) {
                foreach (ServerPlayer *p, players) {
                    if (!p->isAlive() || use.from == p || !use.to.contains(p)/* || p->tag["wzshipo-t"].toBool()*/) continue;
                    if (room->askForSkillInvoke(p, objectName(), data)) {
                        room->broadcastSkillInvoke(objectName());
                        room->notifySkillInvoked(p, objectName());
                        //p->tag["wzshipo-t"] = true;
                        room->askForThrow(p, use.from, 1, "he", objectName());
                    }
                }
            }
        //}

        return false;
    }
};

class WZZhengjiaoRecord : public TriggerSkill
{
public:
    WZZhengjiaoRecord() : TriggerSkill("#wzzhengjiaorec")
    {
        events << EventPhaseStart << CardFinished;
        frequency = Skill::Compulsory;
        global = true;
    }

    bool trigger(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::RoundStart) {
                player->tag.remove("wzzhengjiao_r");
            }
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from == player) {
                QList<ServerPlayer *> list = player->tag["wzzhengjiao_r"].value<QList<ServerPlayer *> >();
                foreach (ServerPlayer *target, use.to) {
                    if (!list.contains(target)) {
                        list << target;
                    }
                }
                player->tag["wzzhengjiao_r"] = QVariant::fromValue(list);
            }
        }
        return false;
    }
};

class WZZhengjiao : public TriggerSkill
{
public:
    WZZhengjiao() : TriggerSkill("wzzhengjiao")
    {
        events << EventPhaseProceeding;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Finish) {
            QList<ServerPlayer *> list = player->tag["wzzhengjiao_r"].value<QList<ServerPlayer *> >();
            QList<ServerPlayer *> available;
            bool wzstage = player->getWZStage() >= 1;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if ((wzstage || !list.contains(p)) && p != player && p->getHandcardNum() > 1)
                    available << p;
            }
            if (available.length() > 0) {
                ServerPlayer *target = room->askForPlayerChosen(player, available, objectName(), "wzzhengjiao-i", true);
                if (target && target->isAlive()) {
                    room->broadcastSkillInvoke(objectName());
                    room->notifySkillInvoked(player, objectName());
                    QList<const Card*> handcards = target->getHandcards();
                    qShuffle(handcards);
                    int count = handcards.length() / 2;
                    //QList<const Card*> obtains = handcards.mid(0, handcards.length() / 2);
                    DummyCard *dummy = new DummyCard(handcards.mid(0, count));
                    room->obtainCard(player, dummy, CardMoveReason(CardMoveReason::S_REASON_GOTCARD, player->objectName(), target->objectName(), objectName(), QString()), false);
                    delete dummy;
                    if (count >= 3)
                        room->loseHp(player, 1);
                }
            }
        }
        return false;
    }
};

WZSuoshiCard::WZSuoshiCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool WZSuoshiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    //QStringList list = Self->property("wzsuoshi_t").toString().split('+');
    return targets.isEmpty();// && !list.contains(to_select->objectName());
}

void WZSuoshiCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *startplayer = targets.first();
    QList<ServerPlayer *> alives = room->getAllPlayers(false, startplayer);
    //QStringList list = player->property("wzsuoshi_t").toString().split('+');
    //list << startplayer->objectName();
    //room->setPlayerProperty(player, "wzsuoshi_t", list.join('+'));

    int x = 0;
    foreach (ServerPlayer *p, alives) {
        int tmp = p->getCardCount();
        room->askForDiscard(p, "wzsuoshi", x + 1, x + 1, false, true, QString("wzsuoshi-p:::%1:%2").arg(x + 1).arg(x));
        room->drawCards(p, x, objectName());
        x = std::min(x + 1, tmp);
    }
}

class WZSuoshiEffect : public TriggerSkill
{
public:
    WZSuoshiEffect() : TriggerSkill("#wzsuoshieffect")
    {
        events << CardsMoveOneTime;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.reason.m_skillName == "wzsuoshi" && move.from && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            room->damage(DamageStruct("wzsuoshi", player, room->findPlayerByObjectName(move.from->objectName()), 1));
        }
        return false;
    }
};

class WZSuoshi : public WZZeroCardViewAsSkill
{
public:
    WZSuoshi() : WZZeroCardViewAsSkill("wzsuoshi", 1)
    {
        response_or_use = true;
    }

    bool isEnabledAtPlayWZ(const Player *player) const
    {
        return !player->hasUsed("WZSuoshiCard");
        //QStringList list = player->property("wzsuoshi_t").toString().split('+');
        //foreach (const ClientPlayer *p, ClientInstance->getPlayers()) {
        //    if (!list.contains(p->objectName())) {
        //        return !player->hasUsed("WZSuoshiCard");
        //    }
        //}
        //return false;
    }

    const Card *viewAs() const
    {
        WZSuoshiCard *card = new WZSuoshiCard;
        card->setSkillName(objectName());
        return card;
    }
};

class WZYudan : public WZOneCardViewAsSkill
{
public:
    WZYudan() : WZOneCardViewAsSkill("wzyudan", 1)
    {
        response_or_use = true;
    }

    bool isEnabledAtPlayWZ(const Player *player) const
    {
        Peach *p = new Peach(Card::NoSuit, 0);
        p->deleteLater();
        return p->isAvailable(player);
    }

    bool isEnabledAtResponseWZ(const Player *, const QString &pattern) const
    {
        return pattern.contains("peach");
    }

    bool viewFilter(const Card *to_select) const
    {
        return to_select->isRed() && !to_select->isEquipped();
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Peach *p = new Peach(originalCard->getSuit(), originalCard->getNumber());
        p->setSkillName(objectName());
        p->addSubcard(originalCard);
        return p;
    }
};

WzbossPackage::WzbossPackage() : Package("wzboss")
{
    // generals
    General *wz_huaxiong = new General(this, "wz_huaxiong", "god", 8, true, true);
    wz_huaxiong->mStageList << 4;
    wz_huaxiong->addSkill(new WZYuanjun);
    wz_huaxiong->addSkill(new WZTianwei);
    wz_huaxiong->addSkill(new WZQidun);
    wz_huaxiong->addSkill(new WZXiaoshou);
    wz_huaxiong->addSkill(new WZFenwei);
    wz_huaxiong->addSkill(new WZFenweiTrigger);
    related_skills.insertMulti("wzfenwei", "#wzfenwei-trigger");
    wz_huaxiong->addSkill(new WZFubing);
    wz_huaxiong->addSkill(new WZHuitian);

    General *wz_liru = new General(this, "wz_liru", "god", 6, true, true);
    wz_liru->mStageList << 3;
    wz_liru->addSkill("wzyuanjun");
    wz_liru->addSkill("wztianwei");
    wz_liru->addSkill("wzqidun");
    wz_liru->addSkill(new WZShipo);
    wz_liru->addSkill(new WZZhengjiao);
    wz_liru->addSkill(new WZSuoshi);
    wz_liru->addSkill(new WZSuoshiEffect);
    related_skills.insertMulti("wzsuoshi", "#wzsuoshieffect");
    wz_liru->addSkill(new WZYudan);

    skills << new WZZhengjiaoRecord;

    addMetaObject<WZFenweiCard>();
    addMetaObject<WZSuoshiCard>();
}

ADD_PACKAGE(Wzboss)

QList<const General *> WzbossPackage::getGenerals()
{
    Package *pkg = PackageAdder::packages()["Wzboss"];
    return pkg->findChildren<const General *>();
}

bool WzbossPackage::contains(const QString &general_name)
{
    QList<const General *> gens = getGenerals();
    foreach (const General *g, gens) {
        QString genobjectname = g->objectName();
        if (genobjectname == general_name) {
            return true;
        }
    }
    return false;
}
