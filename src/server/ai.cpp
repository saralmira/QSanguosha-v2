#include "ai.h"
#include "serverplayer.h"
#include "engine.h"
#include "standard.h"
#include "maneuvering.h"
#include "lua.hpp"
#include "scenario.h"
#include "aux-skills.h"
#include "settings.h"
#include "roomthread.h"
#include "wrapped-card.h"
#include "room.h"

AI::AI(ServerPlayer *player)
    : self(player)
{
    room = player->getRoom();
}

typedef QPair<QString, QString> RolePair;

struct RoleMapping : public QMap < RolePair, AI::Relation >
{
    void set(const QString &role1, const QString &role2, AI::Relation relation, bool bidirectional = false)
    {
        insert(qMakePair(role1, role2), relation);
        if (bidirectional)
            insert(qMakePair(role2, role1), relation);
    }

    AI::Relation get(const QString &role1, const QString &role2)
    {
        return value(qMakePair(role1, role2), AI::Neutrality);
    }
};

AI::Relation AI::GetRelation3v3(const ServerPlayer *a, const ServerPlayer *b)
{
    QChar c = a->getRole().at(0);
    if (b->getRole().startsWith(c))
        return Friend;
    else
        return Enemy;
}

AI::Relation AI::GetRelationHegemony(const ServerPlayer *a, const ServerPlayer *b)
{
    const bool aShown = a->getRoom()->getTag(a->objectName()).toStringList().isEmpty();
    const bool bShown = b->getRoom()->getTag(b->objectName()).toStringList().isEmpty();

    const QString aName = aShown ? a->getGeneralName() :
        a->getRoom()->getTag(a->objectName()).toStringList().first();
    const QString bName = bShown ? b->getGeneralName() :
        b->getRoom()->getTag(b->objectName()).toStringList().first();

    const QString aKingdom = Sanguosha->getGeneral(aName)->getKingdom();
    const QString bKingdom = Sanguosha->getGeneral(bName)->getKingdom();

    qDebug() << aKingdom << bKingdom << aShown << bShown;

    return aKingdom == bKingdom ? Friend : Enemy;
}

AI::Relation AI::GetRelation(const ServerPlayer *a, const ServerPlayer *b)
{
    if (a == b) return Friend;
    static RoleMapping map, map_good, map_bad;
    if (map.isEmpty()) {
        map.set("lord", "lord", Friend);
        map.set("lord", "rebel", Enemy);
        map.set("lord", "loyalist", Friend);
        map.set("lord", "renegade", Neutrality);

        map.set("loyalist", "loyalist", Friend);
        map.set("loyalist", "lord", Friend);
        map.set("loyalist", "rebel", Enemy);
        map.set("loyalist", "renegade", Neutrality);

        map.set("rebel", "rebel", Friend);
        map.set("rebel", "lord", Enemy);
        map.set("rebel", "loyalist", Enemy);
        map.set("rebel", "renegade", Neutrality);

        map.set("renegade", "lord", Friend);
        map.set("renegade", "loyalist", Neutrality);
        map.set("renegade", "rebel", Neutrality);
        map.set("renegade", "renegade", Neutrality);

        map_good = map;
        map_good.set("renegade", "loyalist", Enemy, false);
        map_good.set("renegade", "lord", Neutrality, true);
        map_good.set("renegade", "rebel", Friend, false);

        map_bad = map;
        map_bad.set("renegade", "loyalist", Neutrality, true);
        map_bad.set("renegade", "rebel", Enemy, true);
    }

    if (a->aliveCount() == 2) {
        return Enemy;
    }

    QString roleA = a->getRole();
    QString roleB = b->getRole();

    Room *room = a->getRoom();

    int good = 0, bad = 0;
    QList<ServerPlayer *> players = room->getAlivePlayers();
    foreach (ServerPlayer *player, players) {
        switch (player->getRoleEnum()) {
        case Player::Lord:
        case Player::Loyalist: good++; break;
        case Player::Rebel: bad++; break;
        case Player::Renegade: good++; break;
        }
    }

    if (bad > good)
        return map_bad.get(roleA, roleB);
    else if (good > bad)
        return map_good.get(roleA, roleB);
    else
        return map.get(roleA, roleB);
}

AI::Relation AI::relationTo(const ServerPlayer *other) const
{
    if (self == other)
        return Friend;

    const Scenario *scenario = room->getScenario();
    if (scenario)
        return scenario->relationTo(self, other);

    if (room->getMode() == "06_3v3" || room->getMode() == "06_XMode" || room->getMode() == "08_defense")
        return GetRelation3v3(self, other);
    else if (Config.EnableHegemony)
        return GetRelationHegemony(self, other);

    return GetRelation(self, other);
}

bool AI::isFriend(const ServerPlayer *other) const
{
    return relationTo(other) == Friend;
}

bool AI::isEnemy(const ServerPlayer *other) const
{
    return relationTo(other) == Enemy;
}

QList<ServerPlayer *> AI::getEnemies() const
{
    QList<ServerPlayer *> players = room->getOtherPlayers(self);
    QList<ServerPlayer *> enemies;
    foreach(ServerPlayer *p, players)
        if (isEnemy(p)) enemies << p;

    return enemies;
}

QList<ServerPlayer *> AI::getFriends() const
{
    QList<ServerPlayer *> players = room->getOtherPlayers(self);
    QList<ServerPlayer *> friends;
    foreach(ServerPlayer *p, players)
        if (isFriend(p)) friends << p;

    return friends;
}

void AI::filterEvent(TriggerEvent, ServerPlayer *, const QVariant &)
{
    // dummy
}

TrustAI::TrustAI(ServerPlayer *player)
    : AI(player)
{
    response_skill = new ResponseSkill;
    response_skill->setParent(this);
}

void TrustAI::activate(CardUseStruct &card_use)
{
    QList<const Card *> cards = self->getHandcards();
    foreach (const Card *card, cards) {
        if (card->targetFixed()) {
            if (useCard(card)) {
                card_use.card = card;
                card_use.from = self;
                return;
            }
        }
    }
}

bool TrustAI::useCard(const Card *card)
{
    if (card->isKindOf("EquipCard")) {
        const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
        switch (equip->location()) {
        case EquipCard::WeaponLocation: {
            WrappedCard *weapon = self->getWeapon();
            if (weapon == NULL)
                return true;
            const Weapon *new_weapon = qobject_cast<const Weapon *>(equip);
            const Weapon *ole_weapon = qobject_cast<const Weapon *>(weapon->getRealCard());
            return new_weapon->getRange() > ole_weapon->getRange();
        }
        case EquipCard::ArmorLocation: return !self->getArmor();
        case EquipCard::OffensiveHorseLocation: return !self->getOffensiveHorse();
        case EquipCard::DefensiveHorseLocation: return !self->getDefensiveHorse();
        case EquipCard::TreasureLocation: return !self->getTreasure();
        default:
            return true;
        }
    }
    return false;
}

Card::Suit TrustAI::askForSuit(const QString &)
{
    return Card::AllSuits[qrand() % 4];
}

QString TrustAI::askForKingdom()
{
    QString role;
    ServerPlayer *lord = room->getLord();
    QStringList kingdoms = Sanguosha->getKingdoms();
    kingdoms.removeOne("god");
    if (!lord) return kingdoms.at(qrand() % kingdoms.length());

    switch (self->getRoleEnum()) {
    case Player::Lord: role = kingdoms.at(qrand() % kingdoms.length()); break;
    case Player::Renegade:
    case Player::Rebel: {
        if ((lord->hasLordSkill("xueyi") && self->getRoleEnum() == Player::Rebel) || lord->hasLordSkill("shichou"))
            role = "wei";
        else
            role = lord->getKingdom();
        break;
    }
    case Player::Loyalist: {
        if (lord->getGeneral()->isLord())
            role = lord->getKingdom();
        else if (lord->getGeneral2() && lord->getGeneral2()->isLord())
            role = lord->getGeneral2()->getKingdom();
        else {
            if (lord->hasSkill("yongsi")) kingdoms.removeOne(lord->getKingdom());
            role = kingdoms.at(qrand() % kingdoms.length());
        }
        break;
    }
    default:
        break;
    }

    return role;
}

bool TrustAI::askForSkillInvoke(const QString &, const QVariant &)
{
    return false;
}

QString TrustAI::askForChoice(const QString &, const QString &choice, const QVariant &)
{
    QStringList choices = choice.split("+");
    return choices.at(qrand() % choices.length());
}

QList<int> TrustAI::askForDiscard(const QString &, int, int min_num, bool optional, bool include_equip, const QString &pattern)
{
    QList<int> to_discard;
    if (optional)
        return to_discard;
    else
        return self->forceToDiscard(min_num, include_equip, self->hasFlag("Global_AIDiscardExchanging"), pattern);
}

const Card *TrustAI::askForNullification(const Card *, ServerPlayer *, ServerPlayer *, bool)
{
    return NULL;
}

int TrustAI::askForCardChosen(ServerPlayer *, const QString &, const QString &, Card::HandlingMethod)
{
    return -1;
}

const Card *TrustAI::askForCard(const QString &pattern, const QString &prompt, const QVariant &data)
{
    Q_UNUSED(prompt);
    Q_UNUSED(data);

    response_skill->setPattern(pattern);
    QList<const Card *> cards = self->getHandcards();
    foreach(const Card *card, cards)
        if (response_skill->matchPattern(self, card)) return card;

    return NULL;
}

QString TrustAI::askForUseCard(const QString &, const QString &, const Card::HandlingMethod)
{
    return ".";
}

int TrustAI::askForAG(const QList<int> &card_ids, bool refusable, const QString &)
{
    if (refusable)
        return -1;

    int r = qrand() % card_ids.length();
    return card_ids.at(r);
}

const Card *TrustAI::askForCardShow(ServerPlayer *, const QString &)
{
    return self->getRandomHandCard();
}

static bool CompareByNumber(const Card *c1, const Card *c2)
{
    return c1->getNumber() < c2->getNumber();
}

const Card *TrustAI::askForPindian(ServerPlayer *requestor, const QString &reason)
{
    QList<const Card *> cards = self->getHandcards();
    qSort(cards.begin(), cards.end(), CompareByNumber);

    // zhiba special case
    if (reason == "zhiba_pindian" && self->hasLordSkill("zhiba"))
        return cards.last();

    if (requestor != self && isFriend(requestor))
        return cards.first();
    else
        return cards.last();
}

ServerPlayer *TrustAI::askForPlayerChosen(const QList<ServerPlayer *> &targets, const QString &reason)
{
    Q_UNUSED(reason);

    int r = qrand() % targets.length();
    return targets.at(r);
}

QList<ServerPlayer *> TrustAI::askForPlayersChosen(const QList<ServerPlayer *> &targets, const QString &reason)
{
    Q_UNUSED(reason);

    QList<ServerPlayer *> ts;
    ts.append(targets);
    return ts;
}

const Card *TrustAI::askForSinglePeach(ServerPlayer *dying)
{
    if (isFriend(dying)) {
        QList<const Card *> cards = self->getHandcards();
        foreach (const Card *card, cards) {
            if (card->isKindOf("Peach") && self->getMark("Global_PreventPeach") == 0)
                return card;
            if (card->isKindOf("Analeptic") && dying == self)
                return card;
        }
    }

    return NULL;
}

ServerPlayer *TrustAI::askForYiji(const QList<int> &, const QString &, int &)
{
    return NULL;
}

void TrustAI::askForGuanxing(const QList<int> &cards, QList<int> &up, QList<int> &bottom, int guanxing_type)
{
    Q_UNUSED(bottom);
    Q_UNUSED(guanxing_type);

    if (guanxing_type == Room::GuanxingDownOnly) {
        bottom = cards;
        up.clear();
    } else {
        up = cards;
        bottom.clear();
    }
}

LuaAI::LuaAI(ServerPlayer *player)
    : TrustAI(player), callback(0)
{
}

QString LuaAI::askForUseCard(const QString &pattern, const QString &prompt, const Card::HandlingMethod method)
{
    if (callback == 0)
        return TrustAI::askForUseCard(pattern, prompt, method);

    lua_State *L = room->getLuaState();

    pushCallback(L, __FUNCTION__);
    lua_pushstring(L, pattern.toLatin1());
    lua_pushstring(L, prompt.toLatin1());
    lua_pushinteger(L, method);

    int error = lua_pcall(L, 4, 1, 0);
    const char *result = lua_tostring(L, -1);
    lua_pop(L, 1);

    if (error) {
        const char *error_msg = result;
        room->output(error_msg);
        return ".";
    }

    return result;
}

QList<int> LuaAI::askForDiscard(const QString &reason, int discard_num, int min_num, bool optional, bool include_equip, const QString &pattern)
{
    lua_State *L = room->getLuaState();

    pushCallback(L, __FUNCTION__);
    lua_pushstring(L, reason.toLatin1());
    lua_pushinteger(L, discard_num);
    lua_pushinteger(L, min_num);
    lua_pushboolean(L, optional);
    lua_pushboolean(L, include_equip);
    lua_pushstring(L, pattern.toLatin1());

    int error = lua_pcall(L, 7, 1, 0);
    if (error) {
        reportError(L);
        return TrustAI::askForDiscard(reason, discard_num, min_num, optional, include_equip, pattern);
    }

    QList<int> result;
    if (getTable(L, result))
        return result;
    else
        return TrustAI::askForDiscard(reason, discard_num, min_num, optional, include_equip, pattern);
}

bool LuaAI::getTable(lua_State *L, QList<int> &table)
{
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return false;
    }

    size_t len = lua_rawlen(L, -1);
    size_t i;
    for (i = 0; i < len; i++) {
        lua_rawgeti(L, -1, i + 1);
        table << lua_tointeger(L, -1);
        lua_pop(L, 1);
    }

    lua_pop(L, 1);

    return true;
}

int LuaAI::askForAG(const QList<int> &card_ids, bool refusable, const QString &reason)
{
    lua_State *L = room->getLuaState();

    pushCallback(L, __FUNCTION__);
    pushQIntList(L, card_ids);
    lua_pushboolean(L, refusable);
    lua_pushstring(L, reason.toLatin1());

    int error = lua_pcall(L, 4, 1, 0);
    if (error) {
        reportError(L);
        return TrustAI::askForAG(card_ids, refusable, reason);
    }

    int card_id = lua_tointeger(L, -1);
    lua_pop(L, 1);

    return card_id;
}

void LuaAI::pushCallback(lua_State *L, const char *function_name)
{
    Q_ASSERT(callback);

    lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
    lua_pushstring(L, function_name);
}

void LuaAI::pushQIntList(lua_State *L, const QList<int> &list)
{
    lua_createtable(L, list.length(), 0);

    for (int i = 0; i < list.length(); i++) {
        lua_pushinteger(L, list.at(i));
        lua_rawseti(L, -2, i + 1);
    }
}

void LuaAI::reportError(lua_State *L)
{
    const char *error_msg = lua_tostring(L, -1);
    room->output(error_msg);
    lua_pop(L, 1);
}

void LuaAI::askForGuanxing(const QList<int> &cards, QList<int> &up, QList<int> &bottom, int guanxing_type)
{
    lua_State *L = room->getLuaState();

    pushCallback(L, __FUNCTION__);
    pushQIntList(L, cards);
    lua_pushinteger(L, guanxing_type);

    int error = lua_pcall(L, 3, 2, 0);
    if (error) {
        reportError(L);
        return TrustAI::askForGuanxing(cards, up, bottom, guanxing_type);
    }

    getTable(L, bottom);
    getTable(L, up);
}

