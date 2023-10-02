#include "hulao.h"
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


HLZongyuCard::HLZongyuCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void HLZongyuCard::use(Room *room, ServerPlayer *player, QList<ServerPlayer *> &) const
{
    room->loseHp(player, 1);
    Analeptic *ana = new Analeptic(Card::NoSuit, 0);
    ana->setSkillName("hlzongyu");
    room->useCard(CardUseStruct(ana, player, player, true));
}

class HLZongyu : public ZeroCardViewAsSkill
{
public:
    HLZongyu() : ZeroCardViewAsSkill("hlzongyu")
    {
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return Analeptic::IsAvailable(player);
    }

    const Card *viewAs() const
    {
        Card *card = new HLZongyuCard;
        card->setSkillName(objectName());
        return card;
    }
};

class HLLingnveTargetMod : public TargetModSkill
{
public:
    HLLingnveTargetMod() : TargetModSkill("#hllingnve-target")
    {
        pattern = "Slash";
        frequency = Compulsory;
    }

    int getResidueNum(const Player *from, const Card *) const
    {
        if (from->hasSkill("hllingnve"))
            return from->getMark("hllingnve");
        else
            return 0;
    }
};

class HLLingnve : public TriggerSkill
{
public:
    HLLingnve() : TriggerSkill("hllingnve")
    {
        events << Damage << EventPhaseStart;
        frequency = Skill::NotCompulsory;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            player->setMark("hllingnve", 0);
        } else if (player->getPhase() == Player::Play) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && room->askForSkillInvoke(player, objectName(), data)) {
                room->broadcastSkillInvoke(objectName());

                JudgeStruct judge;
                judge.pattern = ".|black";
                judge.good = true;
                judge.reason = objectName();
                judge.who = player;
                judge.play_animation = true;

                room->judge(judge);
                if (judge.isGood()) {
                    player->obtainCard(judge.card);
                    room->addPlayerMark(player, "hllingnve", 1);
                }
            }
        }

        return false;
    }
};

class HLBaozheng : public TriggerSkill
{
public:
    HLBaozheng() : TriggerSkill("hlbaozheng")
    {
        events << EventPhaseEnd;
        frequency = Skill::Compulsory;
        mustskillowner = false;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Draw) {
            QList<ServerPlayer *> players = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *p, players) {
                if (!p->isAlive() || player == p || player->getHandcardNum() <= p->getHandcardNum())
                    continue;

                room->notifySkillInvoked(p, objectName());
                room->broadcastSkillInvoke(objectName());
                const Card *card = room->askForExchange(player, objectName(), 1, 1, true, QString("hlbaozheng-invoke:%1").arg(p->objectName()), true, ".|diamond");
                if (card) {
                    p->obtainCard(card, false);
                } else {
                    room->damage(DamageStruct(objectName(), p, player, 1));
                }
            }
        }

        return false;
    }
};

class HLNishi : public DrawCardsSkill
{
public:
    HLNishi() : DrawCardsSkill("hlnishi")
    {
        frequency = Skill::Compulsory;
    }

    int getDrawNum(ServerPlayer *player, int) const
    {
        int count = std::min(4, player->getHp());
        Room *room = player->getRoom();
        LogMessage log;
        log.type = "#HLNishiInvoke";
        log.from = player;
        log.arg = objectName();
        log.arg2 = QString::number(count);
        room->sendLog(log);
        room->broadcastSkillInvoke(objectName());

        return count;
    }
};

class HLHengxing : public TriggerSkill
{
public:
    HLHengxing() : TriggerSkill("hlhengxing")
    {
        events << TargetConfirmed;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card && use.card->isKindOf("Slash") && use.to.contains(player) && use.from != player) {
            int x = player->getHp();
            if (room->askForDiscard(player, objectName(), x, x, true, true, QString("hlhengxing-invoke:::%1").arg(x))) {
                room->notifySkillInvoked(player, objectName());
                room->broadcastSkillInvoke(objectName());
                use.nullified_list << player->objectName();
                data = QVariant::fromValue(use);
            }
        }
        return false;
    }
};

HulaoPackage::HulaoPackage() : Package("hulao")
{
    // generals
    General *hulao_dongzhuo1 = new General(this, "hulao_dongzhuo1", "god", 8, true, true);
    hulao_dongzhuo1->addSkill(new HLZongyu);
    hulao_dongzhuo1->addSkill(new HLLingnve);
    hulao_dongzhuo1->addSkill(new HLLingnveTargetMod);
    hulao_dongzhuo1->addSkill(new HLBaozheng);
    related_skills.insertMulti("hllingnve", "#hllingnve-target");

    General *hulao_dongzhuo2 = new General(this, "hulao_dongzhuo2", "god", 4, true, true);
    hulao_dongzhuo2->addSkill("hlzongyu");
    hulao_dongzhuo2->addSkill("hllingnve");
    hulao_dongzhuo2->addSkill("#hllingnve-target");
    hulao_dongzhuo2->addSkill("hlbaozheng");
    hulao_dongzhuo2->addSkill(new HLNishi);
    hulao_dongzhuo2->addSkill(new HLHengxing);

    addMetaObject<HLZongyuCard>();
}

ADD_PACKAGE(Hulao)

QList<const General *> HulaoPackage::getGenerals()
{
    Package *hulao = PackageAdder::packages()["Hulao"];
    return hulao->findChildren<const General *>();
}

bool HulaoPackage::contains(const QString &general_name)
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
