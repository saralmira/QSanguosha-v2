#include "yitian.h"
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

class YitianSwordSkill : public WeaponSkill
{
public:
    YitianSwordSkill() :WeaponSkill("yitian_sword")
    {
        events << DamageComplete << CardsMoveOneTime;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == DamageComplete) {
            if (WeaponSkill::triggerable(player) && player->getPhase() == Player::NotActive) {
                room->askForUseCard(player, "slash", "@YitianSword-slash");
            }
        } else {
            if (player->hasFlag("YitianSwordDamage")) {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (move.from != player || !move.from_places.contains(Player::PlaceEquip))
                    return false;
                for (int i = 0; i < move.card_ids.size(); i++) {
                    if (move.from_places[i] != Player::PlaceEquip) continue;
                    const Card *card = Sanguosha->getEngineCard(move.card_ids[i]);
                    if (card->objectName() == objectName()) {
                        player->setFlags("-YitianSwordDamage");
                        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), "yitian_sword", "@YitianSword-lost", true, true);
                        if (target != NULL)
                            room->damage(DamageStruct("yitian_sword", player, target));
                        return false;
                    }
                }
            }
        }
        return false;
    }
};

YitianSword::YitianSword(Suit suit, int number)
    :Weapon(suit, number, 2)
{
    setObjectName("yitian_sword");
}

void YitianSword::onUninstall(ServerPlayer *player) const
{
    if (player->isAlive() && player->getMark("Equips_Nullified_to_Yourself") == 0 && player->hasWeapon(objectName()))
        player->setFlags("YitianSwordDamage");
}

YTChengxiangCard::YTChengxiangCard()
{
}

bool YTChengxiangCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.length() < subcardsLength();
}

void YTChengxiangCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    if (effect.to->isWounded()) {
        RecoverStruct recover;
        recover.card = this;
        recover.who = effect.from;
        room->recover(effect.to, recover);
    } else
        effect.to->drawCards(2);
}

class YTChengxiangViewAsSkill : public ViewAsSkill
{
public:
    YTChengxiangViewAsSkill() :ViewAsSkill("ytchengxiang")
    {

    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return  pattern == "@@ytchengxiang";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() >= 3)
            return false;

        int sum = 0;
        foreach(const Card *card, selected)
            sum += card->getNumber();

        sum += to_select->getNumber();

        return sum <= Self->getMark("ytchengxiang");
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        int sum = 0;
        foreach(const Card *card, cards)
            sum += card->getNumber();

        if (sum == Self->getMark("ytchengxiang")) {
            YTChengxiangCard *card = new YTChengxiangCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }
};

class YTChengxiang : public MasochismSkill
{
public:
    YTChengxiang() :MasochismSkill("ytchengxiang")
    {
        view_as_skill = new YTChengxiangViewAsSkill;
    }

    void onDamaged(ServerPlayer *caochong, const DamageStruct &damage) const
    {
        const Card *card = damage.card;
        if (card == NULL)
            return;

        int point = card->getNumber();
        if (point < 1 || point > 13)
            return;

        if (caochong->isNude())
            return;

        Room *room = caochong->getRoom();
        room->setPlayerMark(caochong, objectName(), point);

        QString prompt = QString("@ytchengxiang-card:::%1").arg(point);
        room->askForUseCard(caochong, "@@ytchengxiang", prompt);
    }
};

class Conghui : public TriggerSkill
{
public:
    Conghui() :TriggerSkill("conghui")
    {
        frequency = Compulsory;
        events << EventPhaseChanging;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Discard) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());
            player->skip(change.to);
        }
        return false;
    }
};

class Zaoyao : public PhaseChangeSkill
{
public:
    Zaoyao() :PhaseChangeSkill("zaoyao")
    {
        frequency = Compulsory;
    }

    bool onPhaseChange(ServerPlayer *caochong) const
    {
        if (caochong->getPhase() == Player::Finish && caochong->getHandcardNum() > 13) {
            caochong->getRoom()->broadcastSkillInvoke(objectName());
            caochong->getRoom()->notifySkillInvoked(caochong, objectName());
            caochong->throwAllHandCards();
            caochong->getRoom()->loseHp(caochong);
        }

        return false;
    }
};

class WeiwudiGuixin : public PhaseChangeSkill
{
public:
    WeiwudiGuixin() :PhaseChangeSkill("weiwudi_guixin")
    {

    }

    bool onPhaseChange(ServerPlayer *weiwudi) const
    {
        if (weiwudi->getPhase() != Player::Finish)
            return false;

        Room *room = weiwudi->getRoom();
        if (!room->askForSkillInvoke(weiwudi, objectName()))
            return false;

        QString choice = room->askForChoice(weiwudi, objectName(), "modify+obtain");

        int index = qrand() % 2;

        if (choice == "modify") {
            ServerPlayer * to_modify = room->askForPlayerChosen(weiwudi, room->getOtherPlayers(weiwudi), objectName());
            room->setTag("Guixin2Modify", QVariant::fromValue(to_modify));
            QStringList kingdomList = Sanguosha->getKingdoms();
            kingdomList.removeOne("god");
            QString kingdom = room->askForChoice(weiwudi, objectName(), kingdomList.join("+"));
            room->removeTag("Guixin2Modify");
            QString old_kingdom = to_modify->getKingdom();
            room->setPlayerProperty(to_modify, "kingdom", kingdom);

            room->broadcastSkillInvoke(objectName(), index);

            LogMessage log;
            log.type = "#ChangeKingdom";
            log.from = weiwudi;
            log.to << to_modify;
            log.arg = old_kingdom;
            log.arg2 = kingdom;
            room->sendLog(log);
        } else if (choice == "obtain") {
            room->broadcastSkillInvoke(objectName(), index + 2);
            QStringList lords = Sanguosha->getLords();
            foreach (ServerPlayer *player, room->getAlivePlayers()) {
                QString name = player->getGeneralName();
                if (Sanguosha->isGeneralHidden(name)) {
                    QString fname = Sanguosha->findConvertFrom(name);
                    if (!fname.isEmpty()) name = fname;
                }
                lords.removeOne(name);

                if (!player->getGeneral2()) continue;

                name = player->getGeneral2Name();
                if (Sanguosha->isGeneralHidden(name)) {
                    QString fname = Sanguosha->findConvertFrom(name);
                    if (!fname.isEmpty()) name = fname;
                }
                lords.removeOne(name);
            }

            QStringList lord_skills;
            foreach (QString lord, lords) {
                const General *general = Sanguosha->getGeneral(lord);
                QList<const Skill *> skills = general->findChildren<const Skill *>();
                foreach (const Skill *skill, skills) {
                    if (skill->isLordSkill() && !weiwudi->hasSkill(skill->objectName()))
                        lord_skills << skill->objectName();
                }
            }

            if (!lord_skills.isEmpty()) {
                QString skill_name = room->askForChoice(weiwudi, objectName(), lord_skills.join("+"));

                const Skill *skill = Sanguosha->getSkill(skill_name);
                room->acquireSkill(weiwudi, skill);
            }
        }
        return false;
    }
};

JuejiCard::JuejiCard()
{
}


bool JuejiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void JuejiCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *to = effect.to;
    QVariant data = QVariant::fromValue(to);
    while (effect.from->pindian(effect.to, "jueji", NULL)) {
        if (effect.from->isKongcheng() || effect.to->isKongcheng() || !effect.from->askForSkillInvoke("jueji", data))
            break;
    }
}

class JuejiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    JuejiViewAsSkill() :ZeroCardViewAsSkill("jueji")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("JuejiCard");
    }

    const Card *viewAs() const
    {
        return new JuejiCard;
    }
};


class Jueji : public TriggerSkill
{
public:
    Jueji() :TriggerSkill("jueji")
    {
        events << Pindian;
        view_as_skill = new JuejiViewAsSkill;
    }

    bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const
    {
        PindianStruct * pindian = data.value<PindianStruct *>();
        if (pindian->reason == "jueji") {
            if (pindian->isSuccess()) {
                player->obtainCard(pindian->to_card);
            } else
                room->broadcastSkillInvoke(objectName(), 2);
        }

        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return 1;
    }
};

class LukangWeiyan : public TriggerSkill
{
public:
    LukangWeiyan() :TriggerSkill("lukang_weiyan")
    {
        events << EventPhaseChanging;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        switch (change.to) {
        case Player::Draw: {
            if (!target->isSkipped(Player::Draw) && target->askForSkillInvoke("lukang_weiyan", "draw2play")) {
                room->broadcastSkillInvoke(objectName(), 1);
                change.to = Player::Play;
                data = QVariant::fromValue(change);
            }
            break;
        }

        case Player::Play:{
            if (!target->isSkipped(Player::Play) && target->askForSkillInvoke("lukang_weiyan", "play2draw")) {
                room->broadcastSkillInvoke(objectName(), 2);
                change.to = Player::Draw;
                data = QVariant::fromValue(change);
            }
            break;
        }

        default:
            return false;
        }

        return false;
    }
};

class Kegou : public PhaseChangeSkill
{
public:
    Kegou() :PhaseChangeSkill("kegou")
    {
        frequency = Wake;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && target->getMark("kegou") == 0
            && target->getKingdom() == "wu"
            && !target->isLord();
    }

    bool onPhaseChange(ServerPlayer *lukang) const
    {
        foreach (const Player *player, lukang->getSiblings()) {
            if (player->isAlive() && player->getKingdom() == "wu"  && !player->isLord() && player != lukang)
                return false;
        }

        Room *room = lukang->getRoom();

        LogMessage log;
        log.type = "#KegouWake";
        log.from = lukang;
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(lukang, objectName());
        //room->doLightbox("$KegouAnimate", 4000);
        room->doSuperLightbox("lukang", "kegou");
        room->setPlayerMark(lukang, objectName(), 1);

        if (room->changeMaxHpForAwakenSkill(lukang) && lukang->getMark(objectName()) > 0)
            room->acquireSkill(lukang, "lianying");

        return false;
    }
};

// ---------- Lianli related skills
/*

LianliCard::LianliCard(){

}

bool LianliCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
return targets.isEmpty() && to_select->isMale();
}

void LianliCard::onEffect(const CardEffectStruct &effect) const{
Room *room = effect.from->getRoom();

LogMessage log;
log.type = "#LianliConnection";
log.from = effect.from;
log.to << effect.to;
room->sendLog(log);

if(effect.from->getMark("@tied") == 0)
effect.from->gainMark("@tied");

if(effect.to->getMark("@tied") == 0){
QList<ServerPlayer *> players = room->getOtherPlayers(effect.from);
foreach(ServerPlayer *player, players){
if(player->getMark("@tied") > 0){
player->loseMark("@tied");
break;
}
}

effect.to->gainMark("@tied");
}
}
*/

LianliSlashCard::LianliSlashCard()
{

}

bool LianliSlashCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

const Card *LianliSlashCard::validate(CardUseStruct &cardUse) const
{
    cardUse.m_isOwnerUse = false;
    ServerPlayer *zhangfei = cardUse.from;
    Room *room = zhangfei->getRoom();

    ServerPlayer *xiahoujuan = room->findPlayerBySkillName("lianli");
    if (xiahoujuan) {
        const Card *slash = room->askForCard(xiahoujuan, "slash", "@lianli-slash", QVariant(), Card::MethodResponse, NULL, false, QString(), true);
        if (slash)
            return slash;
    }
    room->setPlayerFlag(zhangfei, "Global_LianliFailed");
    return NULL;
}

class LianliSlashViewAsSkill :public ZeroCardViewAsSkill
{
public:
    LianliSlashViewAsSkill() :ZeroCardViewAsSkill("lianli-slash")
    {
        attached_lord_skill = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@tied") > 0 && Slash::IsAvailable(player) && !player->hasFlag("Global_LianliFailed");
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "slash"
            && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
            && !player->hasFlag("Global_LianliFailed");
    }

    const Card *viewAs() const
    {
        return new LianliSlashCard;
    }
};

class LianliSlash : public TriggerSkill
{
public:
    LianliSlash() :TriggerSkill("#lianli-slash")
    {
        events << CardAsked;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getMark("@tied") > 0 && !target->hasSkill("lianli");
    }

    bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const
    {
        QString pattern = data.toStringList().first();
        if (pattern != "slash")
            return false;

        if (!player->askForSkillInvoke("lianli-slash", data))
            return false;

        ServerPlayer *xiahoujuan = room->findPlayerBySkillName("lianli");
        if (xiahoujuan) {
            const Card *slash = room->askForCard(xiahoujuan, "slash", "@lianli-slash", data, Card::MethodResponse, NULL, false, QString(), true);
            if (slash) {
                room->provide(slash);
                return true;
            }
        }

        return false;
    }
};

class LianliJink : public TriggerSkill
{
public:
    LianliJink() :TriggerSkill("#lianli-jink")
    {
        events << CardAsked;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && TriggerSkill::triggerable(target) && target->getMark("@tied") > 0;
    }

    bool trigger(TriggerEvent, Room* room, ServerPlayer *xiahoujuan, QVariant &data) const
    {
        QString pattern = data.toStringList().first();
        if (pattern != "jink")
            return false;

        if (!xiahoujuan->askForSkillInvoke("lianli-jink", data))
            return false;

        QList<ServerPlayer *> players = room->getOtherPlayers(xiahoujuan);
        foreach (ServerPlayer *player, players) {
            if (player->getMark("@tied") > 0) {
                ServerPlayer *zhangfei = player;

                const Card *jink = room->askForCard(zhangfei, "jink", "@lianli-jink", data, Card::MethodResponse, NULL, false, QString(), true);
                if (jink) {
                    room->provide(jink);
                    return true;
                }

                break;
            }
        }

        return false;
    }
};
/*

class LianliViewAsSkill: public ZeroCardViewAsSkill{
public:
LianliViewAsSkill():ZeroCardViewAsSkill("lianli"){

}

bool isEnabledAtPlay(const Player *player) const{
return false;
}

bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
return pattern == "@@lianli";
}

const Card *viewAs() const{
return new LianliCard;
}
};
*/

class Lianli : public PhaseChangeSkill
{
public:
    Lianli() :PhaseChangeSkill("lianli")
    {
        //view_as_skill = new LianliViewAsSkill;
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        if (target->getPhase() == Player::Start) {
            Room *room = target->getRoom();
            //bool used = room->askForUseCard(target, "@@lianli", "@lianli-card");

            QList<ServerPlayer *> males;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->isMale())
                    males << p;
            }

            ServerPlayer *zhangfei;
            if (males.isEmpty() || ((zhangfei = room->askForPlayerChosen(target, males, objectName(), "@lianli-card", true, true)) == NULL)) {
                if (target->hasSkill("liqian") && target->getKingdom() != "wei")
                    room->setPlayerProperty(target, "kingdom", "wei");

                QList<ServerPlayer *> players = room->getAllPlayers();
                foreach (ServerPlayer *player, players) {
                    if (player->getMark("@tied") > 0) {
                        player->loseMark("@tied");
                        if (player->isMale())
                            room->detachSkillFromPlayer(player, "lianli-slash", true, true);
                    }
                }
                return false;
            }

            room->broadcastSkillInvoke(objectName());
            LogMessage log;
            log.type = "#LianliConnection";
            log.from = target;
            log.to << zhangfei;
            room->sendLog(log);

            if (target->getMark("@tied") == 0)
                target->gainMark("@tied");

            if (zhangfei->getMark("@tied") == 0) {
                QList<ServerPlayer *> players = room->getOtherPlayers(target);
                foreach (ServerPlayer *player, players) {
                    if (player->getMark("@tied") > 0) {
                        player->loseMark("@tied");
                        room->detachSkillFromPlayer(player, "lianli-slash", true, true);
                        break;
                    }
                }

                zhangfei->gainMark("@tied");
                room->attachSkillToPlayer(zhangfei, "lianli-slash");
            }

            if (target->hasSkill("liqian") && target->getKingdom() != zhangfei->getKingdom())
                room->setPlayerProperty(target, "kingdom", zhangfei->getKingdom());
        }
        return false;
    }
};

class Tongxin : public MasochismSkill
{
public:
    Tongxin() :MasochismSkill("tongxin")
    {
        frequency = Frequent;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getMark("@tied") > 0;
    }

    void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        Room *room = target->getRoom();
        ServerPlayer *xiahoujuan = room->findPlayerBySkillName(objectName());
        ServerPlayer *zhangfei = NULL;
        if (target == xiahoujuan) {
            QList<ServerPlayer *> players = room->getOtherPlayers(xiahoujuan);
            foreach (ServerPlayer *player, players) {
                if (player->getMark("@tied") > 0) {
                    zhangfei = player;
                    break;
                }
            }
        } else
            zhangfei = target;
        for (int i = 0; i < damage.damage; i++) {
            if (xiahoujuan && xiahoujuan->askForSkillInvoke(this, QVariant::fromValue(damage))) {
                room->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(xiahoujuan, objectName());
                xiahoujuan->drawCards(1);
                if (zhangfei)
                    zhangfei->drawCards(1);
            } else
                break;
        }

    }
};

class LianliClear : public TriggerSkill
{
public:
    LianliClear() :TriggerSkill("#lianli-clear")
    {
        events << Death << EventLoseSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || !player->hasSkill(this))
                return false;
        } else if (triggerEvent == EventLoseSkill) {
            if (data.toString() != "lianli")
                return false;
        }

        foreach (ServerPlayer *player, room->getAlivePlayers()) {
            if (player->getMark("@tied") > 0) {
                player->loseMark("@tied");
                if (player->isMale())
                    room->detachSkillFromPlayer(player, "lianli-slash", true, true);
            }
        }

        return false;
    }
};

// -------- end of Lianli related skills

class WulingExEffect : public TriggerSkill
{
public:
    WulingExEffect() :TriggerSkill("#wuling-ex-effect")
    {
        events << PreHpRecover << DamageInflicted;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const
    {
        ServerPlayer *xuandi = room->findPlayerBySkillName(objectName());
        if (xuandi == NULL)
            return false;

        QString wuling = xuandi->tag.value("wuling").toString();
        if (triggerEvent == PreHpRecover && wuling == "water") {
            RecoverStruct rec = data.value<RecoverStruct>();
            if (rec.card && rec.card->isKindOf("Peach")) {
                LogMessage log;
                log.type = "#WulingWater";
                log.from = player;
                room->sendLog(log);

                rec.recover++;

                data = QVariant::fromValue(rec);
            }
        } else if (triggerEvent == DamageInflicted && wuling == "earth") {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature != DamageStruct::Normal && damage.damage > 1) {
                damage.damage = 1;
                data = QVariant::fromValue(damage);

                LogMessage log;
                log.type = "#WulingEarth";
                log.from = player;
                room->sendLog(log);
            }
        }

        return false;
    }
};

class WulingEffect : public TriggerSkill
{
public:
    WulingEffect() :TriggerSkill("#wuling-effect")
    {
        events << DamageInflicted;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room* room, ServerPlayer *, QVariant &data) const
    {
        ServerPlayer *xuandi = room->findPlayerBySkillName(objectName());
        if (xuandi == NULL)
            return false;

        QString wuling = xuandi->tag.value("wuling").toString();
        DamageStruct damage = data.value<DamageStruct>();

        if (wuling == "wind") {
            if (damage.nature == DamageStruct::Fire) {
                damage.damage++;
                data = QVariant::fromValue(damage);

                LogMessage log;
                log.type = "#WulingWind";
                log.from = damage.to;
                log.arg = QString::number(damage.damage - 1);
                log.arg2 = QString::number(damage.damage);
                room->sendLog(log);
            }
        } else if (wuling == "thunder") {
            if (damage.nature == DamageStruct::Thunder) {
                damage.damage++;
                data = QVariant::fromValue(damage);

                LogMessage log;
                log.type = "#WulingThunder";
                log.from = damage.to;
                log.arg = QString::number(damage.damage - 1);
                log.arg2 = QString::number(damage.damage);
                room->sendLog(log);
            }
        } else if (wuling == "fire") {
            if (damage.nature != DamageStruct::Fire) {
                damage.nature = DamageStruct::Fire;
                data = QVariant::fromValue(damage);

                LogMessage log;
                log.type = "#WulingFire";
                log.from = damage.to;
                room->sendLog(log);
            }
        }

        return false;
    }
};

class Wuling : public PhaseChangeSkill
{
public:
    Wuling() :PhaseChangeSkill("wuling")
    {

    }

    bool onPhaseChange(ServerPlayer *xuandi) const
    {
        static QStringList effects;
        if (effects.isEmpty()) {
            effects << "wind" << "thunder" << "water" << "fire" << "earth";
        }

        if (xuandi->getPhase() == Player::Start) {
            QString current = xuandi->tag.value("wuling").toString();
            QStringList choices;
            foreach (QString effect, effects) {
                if (effect != current)
                    choices << effect;
            }

            Room *room = xuandi->getRoom();
            QString choice = room->askForChoice(xuandi, objectName(), choices.join("+"));
            if (!current.isEmpty())
                xuandi->loseMark("@" + current);

            xuandi->gainMark("@" + choice);
            xuandi->tag["wuling"] = choice;

            room->broadcastSkillInvoke(objectName(), effects.indexOf(choice) + 1);
        }

        return false;
    }
};

GuihanCard::GuihanCard()
{
}

void GuihanCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->getRoom()->swapSeat(effect.from, effect.to);
}

class Guihan : public ViewAsSkill
{
public:
    Guihan() :ViewAsSkill("guihan")
    {

    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("GuihanCard");
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (to_select->isEquipped())
            return false;

        if (selected.isEmpty())
            return to_select->isRed();
        else if (selected.length() == 1) {
            Card::Suit suit = selected.first()->getSuit();
            return to_select->getSuit() == suit;
        } else
            return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        GuihanCard *card = new GuihanCard;
        card->addSubcards(cards);
        return card;
    }
};

class CaizhaojiHujia : public TriggerSkill
{
public:
    CaizhaojiHujia() :TriggerSkill("caizhaoji_hujia")
    {
        events << EventPhaseStart << FinishJudge;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *caizhaoji, QVariant &data) const
    {

        if (triggerEvent == EventPhaseStart && caizhaoji->getPhase() == Player::Finish) {
            int times = 0;
            bool canRetrial = caizhaoji->hasSkills("guicai|nosguicai|guidao|huanshi");
            bool first = true;
            while (caizhaoji->askForSkillInvoke("caizhaoji_hujia")) {
                if (first) {
                    room->broadcastSkillInvoke(objectName());
                    first = false;
                }

                times++;
                if (times == 3)
                    caizhaoji->turnOver();

                JudgeStruct judge;
                judge.pattern = ".|red";
                judge.good = true;
                judge.reason = objectName();
                judge.play_animation = false;
                judge.who = caizhaoji;
                judge.time_consuming = true;

                if (canRetrial)
                    caizhaoji->setFlags("HujiaRetrial");
                try {
                    room->judge(judge);
                }
                catch (TriggerEvent triggerEvent) {
                    if ((triggerEvent == TurnBroken || triggerEvent == StageChange) && caizhaoji->hasFlag("HujiaRetrial"))
                        caizhaoji->setFlags("-HujiaRetrial");
                    throw triggerEvent;
                }

                if (judge.isBad())
                    break;
            }
            if (canRetrial && caizhaoji->tag.contains(objectName())) {
                DummyCard *dummy = new DummyCard(VariantList2IntList(caizhaoji->tag[objectName()].toList()));
                if (dummy->subcardsLength() > 0)
                    caizhaoji->obtainCard(dummy);
                caizhaoji->tag.remove(objectName());
                delete dummy;
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName()) {
                bool canRetrial = caizhaoji->hasFlag("HujiaRetrial");
                if (judge->card->isRed()) {
                    if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
                        if (canRetrial) {
                            CardMoveReason reason(CardMoveReason::S_REASON_JUDGEDONE, caizhaoji->objectName(), QString(), judge->reason);
                            room->moveCardTo(judge->card, caizhaoji, NULL, Player::PlaceTable, reason, true);
                            QVariantList luoshen_list = caizhaoji->tag[objectName()].toList();
                            luoshen_list << judge->card->getEffectiveId();
                            caizhaoji->tag[objectName()] = luoshen_list;
                        } else {
                            caizhaoji->obtainCard(judge->card);
                        }
                    }
                } else {
                    if (canRetrial) {
                        DummyCard *dummy = new DummyCard(VariantList2IntList(caizhaoji->tag[objectName()].toList()));
                        if (dummy->subcardsLength() > 0)
                            caizhaoji->obtainCard(dummy);
                        caizhaoji->tag.remove(objectName());
                        delete dummy;
                    }
                }
            }
        }

        return false;
    }
};

class Shenjun : public TriggerSkill
{
public:
    Shenjun() :TriggerSkill("shenjun")
    {
        events << GameStart << EventPhaseStart << DamageInflicted;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == GameStart) {
            QString gender = room->askForChoice(player, objectName(), "male+female");
            bool is_male = player->isMale();
            if (gender == "female") {
                if (is_male) {
                    player->setGender(General::Female);
                }
            } else if (gender == "male") {
                if (!is_male) {
                    player->setGender(General::Male);
                }
            }
            LogMessage log;
            log.type = "#ShenjunChoose";
            log.from = player;
            log.arg = gender;
            room->sendLog(log);
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Start) {
                LogMessage log;
                log.from = player;
                log.type = "#ShenjunFlip";
                log.arg = objectName();
                room->sendLog(log);

                if (player->isMale())
                    player->setGender(General::Female);
                else
                    player->setGender(General::Male);
            }
        } else if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature != DamageStruct::Thunder && damage.from &&
                damage.from->isMale() != player->isMale()) {

                LogMessage log;
                log.type = "#ShenjunProtect";
                log.to << player;
                log.from = damage.from;
                log.arg = objectName();
                room->sendLog(log);

                return true;
            }
        }

        return false;
    }
};

class Shaoying : public TriggerSkill
{
public:
    Shaoying() :TriggerSkill("shaoying")
    {
        events << PreDamageDone << DamageComplete;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == PreDamageDone) {
            if (!player->isChained() && damage.from && damage.nature == DamageStruct::Fire && TriggerSkill::triggerable(damage.from)) {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (player->distanceTo(p) == 1)
                        targets << p;
                }

                if (targets.isEmpty())
                    return false;

                ServerPlayer *target = room->askForPlayerChosen(damage.from, targets, objectName(), "@shaoying", true, true);
                if (target != NULL) {

                    LogMessage log;
                    log.type = "#Shaoying";
                    log.from = damage.from;
                    log.to << target;
                    log.arg = objectName();
                    room->sendLog(log);

                    damage.from->tag["ShaoyingTarget"] = QVariant::fromValue(target);
                }
            }

            return false;
        } else if (triggerEvent == DamageComplete) {
            if (damage.from == NULL)
                return false;
            ServerPlayer * target = damage.from->tag.value("ShaoyingTarget", QVariant()).value<ServerPlayer *>();
            damage.from->tag.remove("ShaoyingTarget");
            if (!target || !damage.from || damage.from->isDead())
                return false;

            JudgeStruct judge;
            judge.pattern = ".|red";
            judge.good = true;
            judge.reason = objectName();
            judge.who = damage.from;

            room->judge(judge);

            if (judge.isGood()) {
                room->broadcastSkillInvoke(objectName());
                DamageStruct shaoying_damage;
                shaoying_damage.nature = DamageStruct::Fire;
                shaoying_damage.from = damage.from;
                shaoying_damage.to = target;

                room->damage(shaoying_damage);
            }

            return false;
        }

        return false;
    }
};

class Zonghuo : public TriggerSkill
{
public:
    Zonghuo() :TriggerSkill("zonghuo")
    {
        frequency = Compulsory;
        events << CardUsed;
    }

    bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && !use.card->isKindOf("FireSlash")) {
            FireSlash *fire_slash = new FireSlash(Card::SuitToBeDecided, 0);
            if (!use.card->isVirtualCard())
                fire_slash->addSubcard(use.card);
            else if (use.card->subcardsLength() > 0) {
                foreach(int id, use.card->getSubcards())
                    fire_slash->addSubcard(id);
            }
            fire_slash->setSkillName(objectName());

            //room->broadcastSkillInvoke(objectName());
            LogMessage log;
            log.type = "#Zonghuo";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            use.card = fire_slash;
            data = QVariant::fromValue(use);
        }

        return false;
    }
};

class Gongmou : public PhaseChangeSkill
{
public:
    Gongmou() :PhaseChangeSkill("gongmou")
    {

    }

    bool onPhaseChange(ServerPlayer *zhongshiji) const
    {
        switch (zhongshiji->getPhase()) {
        case Player::Finish:{
            Room *room = zhongshiji->getRoom();
            QList<ServerPlayer *> players = room->getOtherPlayers(zhongshiji);
            ServerPlayer *target = room->askForPlayerChosen(zhongshiji, players, "gongmou", "@gongmou", true, true);
            if (target) {
                room->broadcastSkillInvoke(objectName());
                target->gainMark("@conspiracy");
            }
            break;
        }

        case Player::Start:{
            Room *room = zhongshiji->getRoom();
            QList<ServerPlayer *> players = room->getOtherPlayers(zhongshiji);
            foreach (ServerPlayer *player, players) {
                if (player->getMark("@conspiracy") > 0)
                    player->loseMark("@conspiracy");
            }
        }

        default:
            break;
        }


        return false;
    }
};

class GongmouExchange :public TriggerSkill
{
public:
    GongmouExchange() :TriggerSkill("#gongmou-exchange")
    {
        events << EventPhaseEnd;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getMark("@conspiracy") > 0;
    }

    bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Draw)
            return false;

        player->loseMark("@conspiracy");

        Room *room = player->getRoom();
        ServerPlayer *zhongshiji = room->findPlayerBySkillName("gongmou");
        if (zhongshiji) {
            int x = qMin(zhongshiji->getHandcardNum(), player->getHandcardNum());
            if (x == 0)
                return false;

            const Card *to_exchange = NULL;
            if (player->getHandcardNum() == x)
                to_exchange = player->wholeHandCards();
            else
                to_exchange = room->askForExchange(player, "gongmou", x, x);

            room->moveCardTo(to_exchange, zhongshiji, Player::PlaceHand, false);

            delete to_exchange;

            to_exchange = room->askForExchange(zhongshiji, "gongmou", x, x);
            room->moveCardTo(to_exchange, player, Player::PlaceHand, false);

            delete to_exchange;

            LogMessage log;
            log.type = "#GongmouExchange";
            log.from = zhongshiji;
            log.to << player;
            log.arg = QString::number(x);
            log.arg2 = "gongmou";
            room->sendLog(log);
        }

        return false;
    }
};

LexueCard::LexueCard()
{
    mute = true;
}

bool LexueCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void LexueCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    room->broadcastSkillInvoke("lexue", 1);
    const Card *card = room->askForCardShow(effect.to, effect.from, "lexue");
    int card_id = card->getEffectiveId();
    room->showCard(effect.to, card_id);

    if ((card->getTypeId() == Card::TypeBasic && !card->isKindOf("Jink")) || card->isNDTrick()) {
        room->setPlayerMark(effect.from, "lexue", card_id);
        room->setPlayerFlag(effect.from, "lexue");
    } else {
        effect.from->obtainCard(card);
        room->setPlayerFlag(effect.from, "-lexue");
    }
}

class LexueTrigger : public TriggerSkill
{
public:
    LexueTrigger() : TriggerSkill("#lexue-trigger")
    {
        frequency = Compulsory;
        events << CardUsed << CardResponded;
    }

    bool trigger(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->getSkillName() == "lexue" && player->hasFlag("lexue")) {
                player->drawCards(1, "lexue");
            }
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card && resp.m_card->getSkillName() == "lexue" && player->hasFlag("lexue")) {
                player->drawCards(1, "lexue");
            }
        }
        return false;
    }
};

class Lexue : public ViewAsSkill
{
public:
    Lexue() :ViewAsSkill("lexue")
    {

    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->getTypeId() == Card::TypeBasic)
            return 2;
        else
            return 3;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasUsed("LexueCard")) {
            if (player->hasFlag("lexue")) {
                int card_id = player->getMark("lexue");
                const Card *card = Sanguosha->getCard(card_id);
                return card->isAvailable(player);
            }
            return false;
        } else
            return true;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (!player->hasFlag("lexue"))
            return false;

        if (player->hasUsed("LexueCard") && player->hasFlag("lexue")) {
            int card_id = player->getMark("lexue");
            const Card *card = Sanguosha->getCard(card_id);
            QString name = card->objectName();
            if (name.contains("slash"))
                name = "slash";
            return pattern.contains(name);
        } else
            return false;
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        if (player->hasFlag("lexue")) {
            int card_id = player->getMark("lexue");
            const Card *card = Sanguosha->getCard(card_id);
            if (card->objectName() == "nullification") {
                foreach (const Card *c, player->getHandcards() + player->getEquips()) {
                    if (c->objectName() == "nullification" || c->getSuit() == card->getSuit())
                        return true;
                }
            }
        }
        return false;
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Self->hasUsed("LexueCard") && selected.isEmpty() && Self->hasFlag("lexue")) {
            int card_id = Self->getMark("lexue");
            const Card *card = Sanguosha->getCard(card_id);
            return to_select->getSuit() == card->getSuit();
        } else
            return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (Self->hasUsed("LexueCard")) {
            if (!Self->hasFlag("lexue"))
                return 0;

            if (cards.length() != 1)
                return NULL;

            int card_id = Self->getMark("lexue");
            const Card *card = Sanguosha->getCard(card_id);
            const Card *first = cards.first();

            Card *new_card = Sanguosha->cloneCard(card->objectName(), first->getSuit(), first->getNumber());
            new_card->addSubcards(cards);
            new_card->setSkillName(objectName());
            return new_card;
        } else {
            return new LexueCard;
        }
    }
};

XunzhiCard::XunzhiCard()
{
    target_fixed = true;
    mute = true;
}

void XunzhiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    int index = qrand() % 2 + 1;
    room->broadcastSkillInvoke("xunzhi", index);
    //room->doLightbox("$XunzhiAnimate");
    //room->getThread()->delay(2000);
    room->doSuperLightbox("jiangboyue", "xunzhi");
    source->drawCards(3);

    QList<ServerPlayer *> players = room->getAlivePlayers();
    QSet<QString> general_names;
    foreach(ServerPlayer *player, players)
        general_names << player->getGeneralName();

    QStringList all_generals = Sanguosha->getLimitedGeneralNames();
    QStringList shu_generals;
    foreach (QString name, all_generals) {
        const General *general = Sanguosha->getGeneral(name);
        if (general->getKingdom() == "shu" && !general_names.contains(name))
            shu_generals << name;
    }

    QString general = room->askForGeneral(source, shu_generals);
    source->tag["newgeneral"] = general;
    bool isSecondaryHero = (source->getGeneralName() != "jiangboyue");
    room->changeHero(source, general, false, true, isSecondaryHero);
    room->acquireSkill(source, "xunzhi", false);
    room->setPlayerFlag(source, "xunzhi");
}

class XunzhiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    XunzhiViewAsSkill() :ZeroCardViewAsSkill("xunzhi")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("xunzhi");
    }

    const Card *viewAs() const
    {
        return new XunzhiCard;
    }
};

class Xunzhi : public TriggerSkill
{
public:
    Xunzhi() :TriggerSkill("xunzhi")
    {
        events << EventPhaseChanging;
        view_as_skill = new XunzhiViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive) return false;
        if (target->hasFlag("xunzhi")) {
            bool isSecondaryHero = !(target->getGeneralName() == target->tag.value("newgeneral", "").toString());
            room->changeHero(target, parent()->objectName(), false, false, isSecondaryHero);
            room->killPlayer(target);
        }

        return false;
    }
};

class Dongcha : public PhaseChangeSkill
{
public:
    Dongcha() :PhaseChangeSkill("dongcha")
    {

    }

    bool onPhaseChange(ServerPlayer *player) const
    {
        switch (player->getPhase()) {
        case Player::Start:{
            Room *room = player->getRoom();
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            ServerPlayer *dongchaee = room->askForPlayerChosen(player, players, objectName(), "@dongcha", true);
            if (dongchaee != NULL) {
                room->notifySkillInvoked(player, objectName());
                room->broadcastSkillInvoke(objectName());

                LogMessage log;
                log.type = "#ChoosePlayerWithSkill";
                log.from = player;
                log.to << dongchaee;
                log.arg = objectName();
                room->doNotify(player, QSanProtocol::S_COMMAND_LOG_SKILL, log.toVariant());

                room->setPlayerFlag(dongchaee, "dongchaee");
                room->setTag("Dongchaee", dongchaee->objectName());
                room->setTag("Dongchaer", player->objectName());

                room->showAllCards(dongchaee, player);
            }
            break;
        }

        case Player::NotActive:{
            Room *room = player->getRoom();
            QString dongchaee_name = room->getTag("Dongchaee").toString();
            if (!dongchaee_name.isEmpty()) {
                ServerPlayer *dongchaee = room->findChild<ServerPlayer *>(dongchaee_name);
                room->setPlayerFlag(dongchaee, "-dongchaee");

                room->setTag("Dongchaee", QVariant());
                room->setTag("Dongchaer", QVariant());
            }

            break;
        }

        default:
            break;
        }

        return false;
    }
};

class Dushi : public TriggerSkill
{
public:
    Dushi() :TriggerSkill("dushi")
    {
        events << Death;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasSkill(this);
    }

    bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *killer = death.damage ? death.damage->from : NULL;

        if (death.who == player && killer)
            if (killer != player) {
                room->broadcastSkillInvoke(objectName());
                killer->gainMark("@collapse");
                room->acquireSkill(killer, "benghuai");
            }


        return false;
    }
};

class Sizhan : public TriggerSkill
{
public:
    Sizhan() :TriggerSkill("sizhan")
    {
        events << DamageInflicted << EventPhaseStart;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *elai, QVariant &data) const
    {
        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();

            LogMessage log;
            log.type = "#SizhanPrevent";
            log.from = elai;
            log.arg = QString::number(damage.damage);
            log.arg2 = objectName();
            room->sendLog(log);

            room->broadcastSkillInvoke(objectName());

            elai->gainMark("@struggle", damage.damage);

            return true;
        } else if (triggerEvent == EventPhaseStart && elai->getPhase() == Player::Finish) {
            int x = elai->getMark("@struggle");
            if (x > 0) {
                elai->loseMark("@struggle", x);

                LogMessage log;
                log.type = "#SizhanLoseHP";
                log.from = elai;
                log.arg = QString::number(x);
                log.arg2 = objectName();

                room->sendLog(log);
                room->loseHp(elai, x);
            }
        }

        return false;
    }
};

class Shenli : public TriggerSkill
{
public:
    Shenli() :TriggerSkill("shenli")
    {
        events << ConfirmDamage;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room* room, ServerPlayer *elai, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel")) &&
            elai->getPhase() == Player::Play && !elai->getHistory("shenli")) {
            room->addPlayerHistory(elai, "shenli");

            int x = elai->getMark("@struggle");
            if (x > 0) {
                x = qMin(3, x);
                damage.damage += x;
                data = QVariant::fromValue(damage);

                room->broadcastSkillInvoke(objectName());

                LogMessage log;
                log.type = "#ShenliBuff";
                log.from = elai;
                log.arg = QString::number(x);
                log.arg2 = QString::number(damage.damage);
                room->sendLog(log);
            }
        }

        return false;
    }
};

class Zhenggong : public TriggerSkill
{
public:
    Zhenggong() :TriggerSkill("zhenggong")
    {
        events << TurnStart;
        //frequency = Frequent;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && !target->hasSkill(this);
    }

    bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &) const
    {
        if (player == NULL) return false;
        ServerPlayer *dengshizai = room->findPlayerBySkillName(objectName());

        if (dengshizai && dengshizai->faceUp() && dengshizai->askForSkillInvoke(this)) {
            room->broadcastSkillInvoke(objectName());

            dengshizai->gainAnExtraTurn();

            dengshizai->turnOver();
        }

        return false;
    }
};

TouduCard::TouduCard()
{
    mute = true;
}

bool TouduCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.empty() && Self->canSlash(to_select, NULL, false);
}

void TouduCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->turnOver();
    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->setSkillName("toudu");

    CardUseStruct use;
    use.card = slash;
    use.from = effect.from;
    use.to << effect.to;
    effect.from->getRoom()->useCard(use);
}

class TouduViewAsSkill : public OneCardViewAsSkill
{
public:
    TouduViewAsSkill() : OneCardViewAsSkill("toudu")
    {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@toudu";
    }

    const Card *viewAs(const Card *card) const
    {
        TouduCard *toudu = new TouduCard;
        toudu->addSubcard(card);
        return toudu;
    }
};

class Toudu : public MasochismSkill
{
public:
    Toudu() :MasochismSkill("toudu")
    {
        view_as_skill = new TouduViewAsSkill;
    }

    void onDamaged(ServerPlayer *player, const DamageStruct &) const
    {

        if (player->faceUp() || player->isKongcheng())
            return;

        player->getRoom()->askForUseCard(player, "@@toudu", "@toudu", -1, Card::MethodDiscard, false);

    }
};

YtYisheCard::YtYisheCard()
{
    target_fixed = false;
    will_throw = true;
    handling_method = Card::MethodNone;
}

bool YtYisheCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && to_select->getHandcardNum() <= Self->getHandcardNum();
}

void YtYisheCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    //room->broadcastSkillInvoke("ytyishe", (qrand() % 2) + 1);

    QList<int> cards_self = source->handCards();
    if (!cards_self.size() || targets.size() != 1)
        return;

    ServerPlayer *target = targets.first();
    QList<int> cards_t = target->handCards();

    DummyCard dummy_s(cards_self);
    DummyCard dummy_t(cards_t);
    CardMoveReason r(CardMoveReason::S_REASON_SWAP, source->objectName());
    room->moveCardTo(&dummy_s, source, target, Player::PlaceHand, r, false);
    room->moveCardTo(&dummy_t, target, source, Player::PlaceHand, r, false);
}

/*

class YtYisheViewAsSkill : public ViewAsSkill
{
public:
    YtYisheViewAsSkill() :ViewAsSkill("ytyishe")
    {
        expand_pile = "ytrice";
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return true;
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() >= 5)
            return false;

        return (Self->getPile("ytrice").contains(to_select->getId())) || (Self->getHandcards().contains(to_select));
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty() && Self->getPile("ytrice").isEmpty())
            return NULL;

        YtYisheCard *card = new YtYisheCard;
        card->addSubcards(cards);
        return card;
    }
};

YtYisheAskCard::YtYisheAskCard()
{
    target_fixed = true;
    handling_method = Card::MethodNone;
    will_throw = false;
    m_skillName = "ytyishe_ask";
}

void YtYisheAskCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    ServerPlayer *zhanglu = room->findPlayerBySkillName("ytyishe");
    if (zhanglu == NULL)
        return;

    int card_id = subcards.first();

    room->showCard(zhanglu, card_id);

    if (room->askForChoice(zhanglu, "ytyishe_ask", "allow+disallow") == "allow") {
        source->obtainCard(Sanguosha->getCard(card_id));
        room->showCard(source, card_id);
    }
}

class YtYisheAsk : public OneCardViewAsSkill
{
public:
    YtYisheAsk() :OneCardViewAsSkill("ytyishe_ask")
    {
        attached_lord_skill = true;
        expand_pile = "%ytrice";
        filter_pattern = ".|.|.|%ytrice";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasSkill("ytyishe"))
            return false;

        if (player->usedTimes("YtYisheAskCard") >= 2)
            return false;

        const Player *zhanglu = NULL;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasSkill("ytyishe")) {
                zhanglu = p;
                break;
            }
        }

        return zhanglu && !zhanglu->getPile("ytrice").isEmpty();
    }

    const Card *viewAs(const Card *c) const
    {
        YtYisheAskCard *ys = new YtYisheAskCard;
        ys->addSubcard(c);
        return ys;
    }
};

void YtYisheCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    if (subcards.length() > 5)
        return;

    QList<int> rice = source->getPile("ytrice");

    QList<int> to_handcard;
    QList<int> to_rice;

    foreach (int id, (rice + subcards).toSet()) {
        if (!rice.contains(id))
            to_rice << id;
        else if (!subcards.contains(id))
            to_handcard << id;
    }

    Q_ASSERT(rice.length() + to_rice.length() <= 5);

    if (!to_rice.isEmpty())
        source->addToPile("ytrice", to_rice);

    if (!to_handcard.isEmpty()) {
        DummyCard dummy(to_handcard);
        CardMoveReason r(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, source->objectName());
        room->moveCardTo(&dummy, source, Player::PlaceHand, r, true);
    }
}

class YtYishe : public GameStartSkill
{
public:
    YtYishe() :GameStartSkill("ytyishe")
    {
        view_as_skill = new YtYisheViewAsSkill;
    }

    void onGameStart(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            room->attachSkillToPlayer(p, "ytyishe_ask");
    }
};

class Xiliang : public TriggerSkill
{
public:
    Xiliang() :TriggerSkill("xiliang")
    {
        events << CardsMoveOneTime;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && !target->hasSkill(this);
    }

    bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPhase() != Player::Discard)
            return false;

        ServerPlayer *zhanglu = room->findPlayerBySkillName(objectName());

        if (zhanglu == NULL)
            return false;

        DummyCard dummy;

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            foreach (int id, move.card_ids) {
                const Card *c = Sanguosha->getCard(id);
                if (room->getCardPlace(id) == Player::DiscardPile && c->isRed())
                    dummy.addSubcard(id);
            }
        }

        if (dummy.subcardsLength() == 0 || !zhanglu->askForSkillInvoke(this, data))
            return false;

        bool can_put = 5 - zhanglu->getPile("ytrice").length() >= dummy.subcardsLength();
        if (can_put && room->askForChoice(zhanglu, objectName(), "put+obtain") == "put") {
            zhanglu->addToPile("ytrice", &dummy);
        } else {
            zhanglu->obtainCard(&dummy);
        }

        return false;
    }
};

*/

class YtYishe : public ZeroCardViewAsSkill
{
public:
    YtYishe() : ZeroCardViewAsSkill("ytyishe") { }

    const Card *viewAs() const
    {
        return new YtYisheCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("YtYisheCard");
    }

    bool isEnabledAtResponse(const Player *, const QString &) const { return false; }
};

YtMidaoCard::YtMidaoCard()
{
    target_fixed = true;
    will_throw = true;
    handling_method = Card::MethodNone;
}

void YtMidaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    //room->broadcastSkillInvoke("ytmidao", (qrand() % 2) + 1);

    QList<ServerPlayer *> others = room->getOtherPlayers(source);
    int count = source->getHandcardNum();
    int maxcount = count;

    foreach (ServerPlayer *p, others)
    {
        if (p->getHandcardNum() <= count)
            continue;

        room->delay();
        const Card *card = room->askForExchange(p, "ytmidao", 1, 1, true, "ytmidao_ask::" + source->objectName(), false);
        if (!card)
            continue;

        CardMoveReason movereason(CardMoveReason::S_REASON_GIVE, source->objectName(), p->objectName(), "ytmidao", QString());
        room->moveCardTo(card, p, source, Player::PlaceHand, movereason);

        int newhcnum = p->getHandcardNum();
        maxcount = newhcnum > maxcount ? newhcnum : maxcount;
    }

    if (source->getHandcardNum() > maxcount)
        room->loseHp(source, 1);
}

class YtMidao : public ZeroCardViewAsSkill
{
public:
    YtMidao() : ZeroCardViewAsSkill("ytmidao") { }

    const Card *viewAs() const
    {
        return new YtMidaoCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasUsed("YtMidaoCard"))
            return false;
        bool res = false;
        int count = player->getHandcardNum();
        foreach (const ClientPlayer *p, ClientInstance->getPlayers()) {
            if (p->getHandcardNum() > count) {
                res = true;
                break;
            }
        }
        return res;
    }

    bool isEnabledAtResponse(const Player *, const QString &) const { return false; }
};

YtPuduCard::YtPuduCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void YtPuduCard::use(Room *room, ServerPlayer *player, QList<ServerPlayer *> &) const
{
    room->broadcastSkillInvoke("ytpudu");
    room->notifySkillInvoked(player, "ytpudu");
    room->doSuperLightbox("zhanggongqi", "ytpudu");
    room->removePlayerMark(player, "@ytpudu");

    QList<ServerPlayer *> others = room->getOtherPlayers(player);
    foreach (ServerPlayer *p, others) {
        if (p->getHandcardNum() > 0) {
            DummyCard dummy_card(p->handCards());
            CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, player->objectName(), p->objectName(), "ytpudu", QString());
            room->moveCardTo(&dummy_card, p, player, Player::PlaceHand, reason, false);
        }
    }

    bool ismaxcards = false;
    for (int i = 0; ; ) {
        if (others.size() == 0)
            break;

        foreach (ServerPlayer *p2, others) {
            if (p2->isAlive() && p2->getHandcardNum() >= player->getHandcardNum()) {
                ismaxcards = true;
                break;
            }
        }
        if (ismaxcards)
            break;

        ServerPlayer *p = others[i];
        if (!p->isAlive()) {
            others.removeAt(i);
            if (others.size() <= i)
                i = 0;
            continue;
        }

        const Card *card = room->askForExchange(player, "ytpudu", 1, 1, true, "ytpudu_ask::" + p->objectName(), false);
        if (!card)
            break;

        CardMoveReason movereason(CardMoveReason::S_REASON_GIVE, player->objectName(), p->objectName(), "ytpudu", QString());
        room->moveCardTo(card, player, p, Player::PlaceHand, movereason);

        ++i;
        if (others.size() <= i)
            i = 0;
    }
}

class YtPudu : public ZeroCardViewAsSkill
{
public:
    YtPudu() : ZeroCardViewAsSkill("ytpudu")
    {
        response_or_use = true;
        frequency = Limited;
        limit_mark = "@ytpudu";
    }

    const Card *viewAs() const
    {
        return new YtPuduCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark("@ytpudu") <= 0)
            return false;
        bool res = false;
        foreach (const ClientPlayer *p, ClientInstance->getPlayers()) {
            if (p->getHandcardNum() > 0) {
                res = true;
                break;
            }
        }
        return res;
    }

    bool isEnabledAtResponse(const Player *, const QString &) const { return false; }
};

class Zhengfeng : public AttackRangeSkill
{
public:
    Zhengfeng() : AttackRangeSkill("zhengfeng")
    {

    }

    int getFixed(const Player *target, bool include_weapon) const
    {
        if (target->hasSkill(this) && !include_weapon && target->getHp() > 0)
            return target->getHp();
        return -1;
    }
};

class YTZhenwei : public TriggerSkill
{
public:
    YTZhenwei() :TriggerSkill("ytzhenwei")
    {
        events << SlashMissed;
        //frequency = Frequent;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        if (effect.jink && player->getRoom()->getCardPlace(effect.jink->getEffectiveId()) == Player::DiscardPile
            && player->askForSkillInvoke(this, data)) {
            room->broadcastSkillInvoke(objectName());
            player->obtainCard(effect.jink);
        }

        return false;
    }
};

class Yitian : public TriggerSkill
{
public:
    Yitian() :TriggerSkill("yitian")
    {
        events << DamageCaused;
    }

    bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->getGeneralName().contains("caocao") && player->askForSkillInvoke(this, data)) {
            LogMessage log;
            log.type = "#YitianSolace";
            log.from = player;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(--damage.damage);
            room->sendLog(log);

            room->broadcastSkillInvoke(objectName());
            if (damage.damage <= 0)
                return true;

            data.setValue(damage);
        }

        return false;
    }
};

TaichenCard::TaichenCard()
{
    will_throw = false;
}

bool TaichenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || !Self->canDiscard(to_select, "hej"))
        return false;
    if (!subcards.isEmpty() && Self->getWeapon() && subcards.first() == Self->getWeapon()->getId())
        return Self->distanceTo(to_select) == Self->getAttackRange(false);
    else
        return Self->inMyAttackRange(to_select);
}

void TaichenCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();

    if (subcards.isEmpty())
        room->loseHp(effect.from);
    else
        room->throwCard(this, effect.from);

    for (int i = 0; i < 2; i++) {
        if (effect.from->canDiscard(effect.to, "hej"))
            room->throwCard(room->askForCardChosen(effect.from, effect.to, "hej", "taichen"), effect.to, effect.from);
    }
}

class Taichen : public ViewAsSkill
{
public:
    Taichen() :ViewAsSkill("taichen")
    {

    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.isEmpty() && (to_select->isKindOf("Weapon") || to_select->isKindOf("Armor"));
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() <= 1) {
            TaichenCard *taichen_card = new TaichenCard;
            taichen_card->addSubcards(cards);
            return taichen_card;
        } else
            return NULL;
    }
};

YitianCardPackage::YitianCardPackage()
    :Package("yitian_cards")
{
    (new YitianSword)->setParent(this);

    type = CardPack;

    skills << new YitianSwordSkill;
}

ADD_PACKAGE(YitianCard)

YitianPackage::YitianPackage()
:Package("yitian")
{
    // generals
    General *weiwudi = new General(this, "yt_shencaocao", "god", 3);
    weiwudi->addSkill(new WeiwudiGuixin);
    weiwudi->addSkill("feiying");

    General *caochong = new General(this, "yt_caochong", "wei", 3);
    caochong->addSkill(new YTChengxiang);
    caochong->addSkill(new Conghui);
    caochong->addSkill(new Zaoyao);

    General *zhangjunyi = new General(this, "zhangjunyi", "qun");
    zhangjunyi->addSkill(new Jueji);

    General *lukang = new General(this, "lukang", "wu", 4);
    lukang->addSkill(new LukangWeiyan);
    lukang->addSkill(new Kegou);

    General *jinxuandi = new General(this, "jinxuandi", "god");
    jinxuandi->addSkill(new Wuling);
    jinxuandi->addSkill(new WulingEffect);
    jinxuandi->addSkill(new WulingExEffect);

    related_skills.insertMulti("wuling", "#wuling-effect");
    related_skills.insertMulti("wuling", "#wuling-ex-effect");

    General *xiahoujuan = new General(this, "xiahoujuan", "wei", 3, false);
    xiahoujuan->addSkill(new Lianli);
    xiahoujuan->addSkill(new LianliSlash);
    xiahoujuan->addSkill(new LianliJink);
    xiahoujuan->addSkill(new LianliClear);
    xiahoujuan->addSkill(new Tongxin);
    xiahoujuan->addSkill(new Skill("liqian", Skill::Compulsory));

    related_skills.insertMulti("lianli", "#lianli-slash");
    related_skills.insertMulti("lianli", "#lianli-jink");
    related_skills.insertMulti("lianli", "#lianli-clear");
    //related_skills.insertMulti("lianli", "liqian");

    General *caizhaoji = new General(this, "caizhaoji", "qun", 3, false);
    caizhaoji->addSkill(new Guihan);
    caizhaoji->addSkill(new CaizhaojiHujia);

    General *luboyan = new General(this, "luboyan", "wu", 3);
    luboyan->addSkill(new Shenjun);
    luboyan->addSkill(new Shaoying);
    luboyan->addSkill(new Zonghuo);

    General *zhongshiji = new General(this, "zhongshiji", "wei");
    zhongshiji->addSkill(new Gongmou);
    zhongshiji->addSkill(new GongmouExchange);

    related_skills.insertMulti("gongmou", "#gongmou-exchange");

    General *jiangboyue = new General(this, "jiangboyue", "shu");
    jiangboyue->addSkill(new Lexue);
    jiangboyue->addSkill(new LexueTrigger);
    related_skills.insertMulti("lexue", "#lexue-trigger");
    jiangboyue->addSkill(new Xunzhi);

    General *jiawenhe = new General(this, "jiawenhe", "qun");
    jiawenhe->addSkill(new Dongcha);
    jiawenhe->addSkill(new Dushi);

    General *elai = new General(this, "guzhielai", "wei");
    elai->addSkill(new Sizhan);
    elai->addSkill(new Shenli);

    General *dengshizai = new General(this, "dengshizai", "wei", 3);
    dengshizai->addSkill(new Zhenggong);
    dengshizai->addSkill(new Toudu);
    /*dengshizai->addSkill(new SlashNoDistanceLimitSkill("toudu"));
    related_skills.insertMulti("toudu", "#toudu-slash-ndl");*/

    General *zhanggongqi = new General(this, "zhanggongqi", "qun", 3);
    zhanggongqi->addSkill(new YtMidao);
    zhanggongqi->addSkill(new YtYishe);
    zhanggongqi->addSkill(new YtPudu);
    //zhanggongqi->addSkill(new Xiliang);

    General *yitianjian = new General(this, "yitianjian", "wei");
    yitianjian->addSkill(new Zhengfeng);
    yitianjian->addSkill(new YTZhenwei);
    yitianjian->addSkill(new Yitian);

    General *panglingming = new General(this, "panglingming", "wei");
    panglingming->addSkill(new Taichen);

    skills << new LianliSlashViewAsSkill;// << new YtYisheAsk;

    addMetaObject<YTChengxiangCard>();
    addMetaObject<JuejiCard>();
    /*addMetaObject<LianliCard>();*/
    addMetaObject<LianliSlashCard>();
    addMetaObject<GuihanCard>();
    addMetaObject<LexueCard>();
    addMetaObject<XunzhiCard>();
    //addMetaObject<YtYisheAskCard>();
    addMetaObject<YtYisheCard>();
    addMetaObject<YtMidaoCard>();
    addMetaObject<YtPuduCard>();
    addMetaObject<TaichenCard>();
    addMetaObject<TouduCard>();
}

ADD_PACKAGE(Yitian)
