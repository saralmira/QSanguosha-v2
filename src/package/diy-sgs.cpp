#include "diy-sgs.h"
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

class YaoJiazi : public PhaseChangeSkill
{
public:
    YaoJiazi() : PhaseChangeSkill("yaojiazi")
    {
        frequency = NotFrequent;
    }

    bool onPhaseChange(ServerPlayer *player) const
    {
        if (player->getPhase() == Player::Finish) {
            int div = player->getHp() - player->getHandcardNum();
            if (div <= 0)
                return false;

            Room *room = player->getRoom();
            if (room->askForSkillInvoke(player, objectName())) {
                room->broadcastSkillInvoke(objectName());
                player->drawCards(div, objectName());
            }
        }

        return false;
    }
};

class YaoTaiping : public ViewAsSkill
{
public:
    YaoTaiping() : ViewAsSkill("yaotaiping")
    {
        response_or_use = true;
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.isEmpty())
            return !to_select->isEquipped();
        else if (selected.length() == 1) {
            const Card *card = selected.first();
            return !to_select->isEquipped() && to_select->getSuit() == card->getSuit();
        } else
            return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            Card::Suit suit = cards.first()->getSuit();
            Card *res = NULL;
            switch (suit) {
            case Card::Heart:
                res = new ArcheryAttack(suit, 0);
                res->addSubcards(cards);
                res->setSkillName(objectName());
                break;
            case Card::Diamond:
                res = new GodSalvation(suit, 0);
                res->addSubcards(cards);
                res->setSkillName(objectName());
                break;
            case Card::Spade:
                res = new SavageAssault(suit, 0);
                res->addSubcards(cards);
                res->setSkillName(objectName());
                break;
            case Card::Club:
                res = new AmazingGrace(suit, 0);
                res->addSubcards(cards);
                res->setSkillName(objectName());
                break;
            default:
                break;
            }
            return res;
        }

        return NULL;
    }
};
/*
class YaoTuzhong : public DrawCardsSkill
{
public:
    YaoTuzhong() : DrawCardsSkill("yaotuzhong$")
    {
    }

    int getDrawNum(ServerPlayer *player, int n) const
    {
        if (player->isLord()) {
            Room *room = player->getRoom();
            QList<ServerPlayer *> targets;
            auto kingdom = player->getKingdom();
            foreach(ServerPlayer *p, room->getAlivePlayers())
                if (p->getHp() < p->getMaxHp() && p->getKingdom() == kingdom)
                    targets << p;
            if (targets.isEmpty())
                return n;

            if (room->askForSkillInvoke(player, objectName())) {
                auto t = room->askForPlayerChosen(player, targets, objectName(), "yaotuzhong-invoke", true);
                if (t) {
                    room->recover(t, RecoverStruct(player));
                    return n - 1;
                }
            }
        }
        return n;
    }
};
*/
class YaoTuzhong : public TriggerSkill
{
public:
    YaoTuzhong() : TriggerSkill("yaotuzhong$")
    {
        events << EventPhaseChanging;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Draw && !player->isSkipped(Player::Draw)) {

            QList<ServerPlayer *> targets;
            auto kingdom = player->getKingdom();
            foreach(ServerPlayer *p, room->getAlivePlayers())
                if (p->getHp() < p->getMaxHp() && p->getKingdom() == kingdom)
                    targets << p;
            if (targets.isEmpty())
                return false;

            if (room->askForSkillInvoke(player, objectName())) {
                auto t = room->askForPlayerChosen(player, targets, objectName(), "yaotuzhong-invoke", true);
                if (t) {
                    player->skip(Player::Draw, true);
                    room->recover(t, RecoverStruct(player));
                }
            }
        }
        return false;
    }
};

class YxsDili : public DrawCardsSkill
{
public:
    YxsDili() : DrawCardsSkill("yxsdili")
    {
        frequency = Skill::Compulsory;
    }

    int getPriority(TriggerEvent) const
    {
        return 1;
    }

    int getDrawNum(ServerPlayer *player, int) const
    {
        int count = 2 + player->getLostHp();
        Room *room = player->getRoom();

        if (count != 2) {
            LogMessage log;
            log.type = "#YxsDiliInvoke";
            log.from = player;
            log.arg = objectName();
            //log.arg2 = QString::number(count);
            room->sendLog(log);
            room->broadcastSkillInvoke(objectName());
        }

        return count;
    }
};

class YxsKuangchan : public TriggerSkill
{
public:
    YxsKuangchan() : TriggerSkill("yxskuangchan")
    {
        frequency = Skill::Compulsory;
        events << GameStart << MaxHpChanged;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == GameStart) {
            if (room->hasWelfare(player)) {
                LogMessage log;
                log.type = "#YxsKuangchanInvoke";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);
                room->broadcastSkillInvoke(objectName());
                room->loseMaxHp(player, 1);
            }
        } else {
            int lastmaxhp = player->tag.value("YXSKuangchan", -1).toInt();
            int maxhp = player->getMaxHp();
            if (lastmaxhp < 0) {
                lastmaxhp = maxhp;
            }
            if (lastmaxhp < maxhp) {
                LogMessage log;
                log.type = "#YxsKuangchanInvoke";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);
                room->broadcastSkillInvoke(objectName());
                room->loseMaxHp(player, maxhp - lastmaxhp);
            }
        }

        player->tag["YXSKuangchan"] = player->getMaxHp();
        return false;
    }
};


DiySgsPackage::DiySgsPackage() : Package("diysgs")
{
    // generals
    General *zhangjiao = new General(this, "yao_zhangjiao$", "god", 3);
    zhangjiao->addSkill(new YaoTaiping);
    zhangjiao->addSkill(new YaoJiazi);
    zhangjiao->addSkill(new YaoTuzhong);

    General *luzhishen = new General(this, "yxs_luzhishen", "god", 4);
    luzhishen->addSkill(new YxsDili);
    luzhishen->addSkill(new YxsKuangchan);
}

ADD_PACKAGE(DiySgs)

