﻿#ifndef _ROOM_H
#define _ROOM_H

class TriggerSkill;
class ProhibitSkill;
class Scenario;
class RoomThread3v3;
class RoomThreadXMode;
class RoomThread1v1;
class TrickCard;

struct lua_State;
struct LogMessage;
class ServerPlayer;
class RoomThread;

//#include "serverplayer.h"
//#include "roomthread.h"
#include "protocol.h"
#include "room-state.h"
#include "skill.h"

class Room : public QThread
{
    Q_OBJECT
        Q_ENUMS(GuanxingType)

public:
    enum GuanxingType
    {
        GuanxingUpOnly = 1, GuanxingBothSides = 0, GuanxingDownOnly = -1
    };

    enum ChangeHeroMode
    {
        Thouroughly = 0, KeepOriginMaxHp = 1, TransformMode
    };

    friend class RoomThread;
    friend class RoomThread3v3;
    friend class RoomThreadXMode;
    friend class RoomThread1v1;

    typedef void (Room::*Callback)(ServerPlayer *, const QVariant &);
    typedef bool (Room::*ResponseVerifyFunction)(ServerPlayer *, const QVariant &, void *);

    explicit Room(QObject *parent, const QString &mode);
    ~Room();
    ServerPlayer *addSocket(ClientSocket *socket);
    inline int getId() const
    {
        return _m_Id;
    }
    bool isFull() const;
    bool isFinished() const;
    bool canPause(ServerPlayer *p) const;
    void tryPause();
    void delay();
    int getLack() const;
    QString getMode() const;
    const Scenario *getScenario() const;
    RoomThread *getThread() const;
    ServerPlayer *getCurrent() const;
    void setCurrent(ServerPlayer *current);
    int alivePlayerCount() const;
    QList<ServerPlayer *> getOtherPlayers(ServerPlayer *except, bool include_dead = false) const;
    QList<ServerPlayer *> getPlayers() const;
    QList<ServerPlayer *> getAllPlayers(bool include_dead = false, ServerPlayer *start = NULL) const;
    QList<ServerPlayer *> getAlivePlayers() const;
    // custom
    QList<ServerPlayer *> getAllPlayersOfKingdom(const QString kingdoms, bool include_dead = false, ServerPlayer *start = NULL) const;
    void output(const QString &message);
    void outputEventStack();
    void enterDying(ServerPlayer *player, DamageStruct *reason);
    ServerPlayer *getCurrentDyingPlayer() const;
    void killPlayer(ServerPlayer *victim, DamageStruct *reason = NULL);
    void revivePlayer(ServerPlayer *player, bool sendlog = false);
    QStringList aliveRoles(ServerPlayer *except = NULL) const;
    void gameOver(const QString &winner);
    void slashEffect(const SlashEffectStruct &effect);
    void slashResult(const SlashEffectStruct &effect, const Card *jink);
    void attachSkillToPlayer(ServerPlayer *player, const QString &skill_name);
    void detachSkillFromPlayer(ServerPlayer *player, const QString &skill_name, bool is_equip = false, bool acquire_only = false);
    void handleAcquireDetachSkills(ServerPlayer *player, const QStringList &skill_names, bool acquire_only = false);
    void handleAcquireDetachSkills(ServerPlayer *player, const QString &skill_names, bool acquire_only = false);
    void setPlayerFlag(ServerPlayer *player, const QString &flag);
    void setPlayerProperty(ServerPlayer *player, const char *property_name, const QVariant &value);
    void setPlayerMark(ServerPlayer *player, const QString &mark, int value);
    void addPlayerMark(ServerPlayer *player, const QString &mark, int add_num = 1);
    void removePlayerMark(ServerPlayer *player, const QString &mark, int remove_num = 1);
    void setPlayerCardLimitation(ServerPlayer *player, const QString &limit_list,
        const QString &pattern, bool single_turn);
    void removePlayerCardLimitation(ServerPlayer *player, const QString &limit_list,
        const QString &pattern);
    void clearPlayerCardLimitation(ServerPlayer *player, bool single_turn);
    void setCardFlag(const Card *card, const QString &flag, ServerPlayer *who = NULL);
    void setCardFlag(int card_id, const QString &flag, ServerPlayer *who = NULL);
    void clearCardFlag(const Card *card, ServerPlayer *who = NULL);
    void clearCardFlag(int card_id, ServerPlayer *who = NULL);
    bool useCard(const CardUseStruct &card_use, bool add_history = false);
    void damage(const DamageStruct &data);
    void loseHp(ServerPlayer *victim, int lose = 1);
    void loseMaxHp(ServerPlayer *victim, int lose = 1);
    void gainMaxHp(ServerPlayer *player, int gain = 1);
    bool changeMaxHpForAwakenSkill(ServerPlayer *player, int magnitude = -1);
    void recover(ServerPlayer *player, const RecoverStruct &recover, bool set_emotion = false);
    bool cardEffect(const Card *card, ServerPlayer *from, ServerPlayer *to, bool multiple = false);
    bool cardEffect(const CardEffectStruct &effect);
    bool isJinkEffected(ServerPlayer *user, const Card *jink);
    void judge(JudgeStruct &judge_struct);
    void sendJudgeResult(const JudgeStruct *judge);
    QList<int> getNCards(int n, bool update_pile_number = true);
    QList<int> getNCardsBottom(int n, bool update_pile_number = true);
    ServerPlayer *getLord() const;
    void askForGuanxing(ServerPlayer *zhuge, const QList<int> &cards, GuanxingType guanxing_type = GuanxingBothSides);
    void returnToTopDrawPile(const QList<int> &cards);
    void returnToBottomDrawPile(const QList<int> &cards);
    int doGongxin(ServerPlayer *shenlvmeng, ServerPlayer *target, QList<int> enabled_ids = QList<int>(), QString skill_name = "gongxin");
    void showHandCards(ServerPlayer *player, QList<int> handcards_ids, ServerPlayer *to = NULL);
    int drawCard();
    int drawCardBottom();
    void fillAG(const QList<int> &card_ids, ServerPlayer *who = NULL, const QList<int> &disabled_ids = QList<int>());
    void takeAG(ServerPlayer *player, int card_id, bool move_cards = true, QList<ServerPlayer *> to_notify = QList<ServerPlayer *>());
    void clearAG(ServerPlayer *player = NULL);
    void provide(const Card *card);
    QList<ServerPlayer *> getLieges(const QString &kingdom, ServerPlayer *lord) const;
    void sendLog(const LogMessage &log, QList<ServerPlayer *> players = QList<ServerPlayer *>());
    void sendLog(const LogMessage &log, ServerPlayer *player);
    void testLog(QString str);
    void sendCompulsoryTriggerLog(ServerPlayer *player, const QString &skill_name, bool notify_skill = true);
    void showCard(ServerPlayer *player, int card_id, ServerPlayer *only_viewer = NULL);
    void showAllCards(ServerPlayer *player, ServerPlayer *to = NULL);
    const Card *showCardOfDrawPile(ServerPlayer *player, const QString &skillname, bool top = true);
    QList<int> showCardsOfDrawPile(ServerPlayer *player, int n, const QString &skillname, bool top = true);
    void retrial(const Card *card, ServerPlayer *player, JudgeStruct *judge,
        const QString &skill_name, bool exchange = false);
    void setTargetNoJink(ServerPlayer *player, ServerPlayer *target, CardUseStruct &card_use);
    void makePlayerTurnOver(ServerPlayer *player);

    // Ask a player to send a server request and returns the client response. Call is blocking until client
    // replies or server times out, whichever is earlier.
    // @param player
    //        The server player to carry out the command.
    // @param command
    //        Command to be executed on client side.
    // @param arg
    //        Command args.
    // @param timeOut
    //        Maximum milliseconds that server should wait for client response before returning.
    // @param wait
    //        If true, return immediately after sending the request without waiting for client reply.
    // @return True if the a valid response is returned from client.
    // Usage note: when you need a round trip request-response vector with a SINGLE client, use this command
    // with wait = true and read the reponse from player->getClientReply(). If you want to initiate a poll
    // where more than one clients can respond simultaneously, you have to do it in two steps:
    // 1. Use this command with wait = false once for each client involved in the poll (or you can call this
    //    command only once in all with broadcast = true if the poll is to everypody).
    // 2. Call getResult(player, timeout) on each player to retrieve the result. Read manual for getResults
    //    before you use.
    bool doRequest(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg, time_t timeOut, bool wait);
    bool doRequest(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg, bool wait);

    // Broadcast a request to a list of players and get the client responses. Call is blocking until all client
    // replies or server times out, whichever is earlier. Check each player's m_isClientResponseReady to see if a valid
    // result has been received. The client response can be accessed by calling each player's getClientReply() function.
    // @param players
    //        The list of server players to carry out the command.
    // @param command
    //        Command to be executed on client side. Command arguments should be stored in players->m_commandArgs.
    // @param timeOut
    //        Maximum total milliseconds that server will wait for all clients to respond before returning. Any client
    //        response after the timeOut will be rejected.
    // @return True if the a valid response is returned from client.
    bool doBroadcastRequest(QList<ServerPlayer *> &players, QSanProtocol::CommandType command, time_t timeOut);
    bool doBroadcastRequest(QList<ServerPlayer *> &players, QSanProtocol::CommandType command);

    // Broadcast a request to a list of players and get the first valid client response. Call is blocking until the first
    // client response is received or server times out, whichever is earlier. Any client response is verified by the validation
    // function and argument passed in. When a response is verified to be invalid, the function will continue to wait for
    // the next client response.
    // @param validateFunc
    //        Validation function that verifies whether the reply is a valid one. The first parameter passed to the function
    //        is the response sender, the second parameter is the response content, the third parameter is funcArg passed in.
    // @return The player that first send a legal request to the server. NULL if no such request is received.
    ServerPlayer *doBroadcastRaceRequest(QList<ServerPlayer *> &players, QSanProtocol::CommandType command,
        time_t timeOut, ResponseVerifyFunction validateFunc = NULL, void *funcArg = NULL);

    // Notify a player of a event by sending S_SERVER_NOTIFICATION packets. No reply should be expected from
    // the client for S_SERVER_NOTIFICATION as it's a one way notice. Any message from the client in reply to this call
    // will be rejected.
    bool doNotify(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg);

    // Broadcast a event to a list of players by sending S_SERVER_NOTIFICATION packets. No replies should be expected from
    // the clients for S_SERVER_NOTIFICATION as it's a one way notice. Any message from the client in reply to this call
    // will be rejected.
    bool doBroadcastNotify(QSanProtocol::CommandType command, const QVariant &arg);
    bool doBroadcastNotify(const QList<ServerPlayer *> &players, QSanProtocol::CommandType command, const QVariant &arg);

    bool doNotify(ServerPlayer *player, int command, const char *arg);
    bool doBroadcastNotify(int command, const char *arg);
    bool doBroadcastNotify(const QList<ServerPlayer *> &players, int command, const char *arg);

    bool doNotify(ServerPlayer *player, int command, const QVariant &arg);
    bool doBroadcastNotify(int command, const QVariant &arg);
    bool doBroadcastNotify(const QList<ServerPlayer *> &players, int command, const QVariant &arg);

    // Ask a server player to wait for the client response. Call is blocking until client replies or server times out,
    // whichever is earlier.
    // @param player
    //        The server player to retrieve the client response.
    // @param timeOut
    //        Maximum milliseconds that server should wait for client response before returning.
    // @return True if the a valid response is returned from client.

    // Usage note: this function is only supposed to be either internally used by doRequest (wait = true) or externally
    // used in pair with doRequest (wait = false). Any other usage could result in unexpected synchronization errors.
    // When getResult returns true, it's guaranteed that the expected client response has been stored and can be accessed by
    // calling player->getClientReply(). If getResult returns false, the value stored in player->getClientReply() could be
    // corrupted or in response to an unrelevant server request. Therefore, if the return value is false, do not poke into
    // player->getClientReply(), use the default value directly. If the return value is true, the reply value should still be
    // examined as a malicious client can have tampered with the content of the package for cheating purposes.
    bool getResult(ServerPlayer *player, time_t timeOut);
    ServerPlayer *getRaceResult(QList<ServerPlayer *> &players, QSanProtocol::CommandType command, time_t timeOut,
        ResponseVerifyFunction validateFunc = NULL, void *funcArg = NULL);

    // Verification functions
    bool verifyNullificationResponse(ServerPlayer *, const QVariant &, void *);

    // Notification functions
    bool notifyMoveFocus(ServerPlayer *player);
    bool notifyMoveFocus(ServerPlayer *player, QSanProtocol::CommandType command);
    bool notifyMoveFocus(const QList<ServerPlayer *> &players, QSanProtocol::CommandType command, QSanProtocol::Countdown countdown);

    // Notify client side to move cards from one place to another place. A movement should always be completed by
    // calling notifyMoveCards in pairs, one with isLostPhase equaling true followed by one with isLostPhase
    // equaling false. The two phase design is needed because the target player doesn't necessarily gets the
    // cards that the source player lost. Any trigger during the movement can cause either the target player to
    // be dead or some of the cards to be moved to another place before the target player actually gets it.
    // @param isLostPhase
    //        Specify whether this is a S_COMMAND_LOSE_CARD notification.
    // @param move
    //        Specify all movements need to be broadcasted.
    // @param forceVisible
    //        If true, all players will be able to see the face of card regardless of whether the movement is
    //        relevant or not.
    bool notifyMoveCards(bool isLostPhase, QList<CardsMoveStruct> &move, bool forceVisible, QList<ServerPlayer *> players = QList<ServerPlayer *>());
    bool notifyProperty(ServerPlayer *playerToNotify, const ServerPlayer *propertyOwner, const char *propertyName, const QString &value = QString());
    bool notifyUpdateCard(ServerPlayer *player, int cardId, const Card *newCard);
    bool broadcastUpdateCard(const QList<ServerPlayer *> &players, int cardId, const Card *newCard);
    bool notifyResetCard(ServerPlayer *player, int cardId);
    bool broadcastResetCard(const QList<ServerPlayer *> &players, int cardId);

    bool broadcastProperty(ServerPlayer *player, const char *property_name, const QString &value = QString());
    void notifySkillInvoked(ServerPlayer *player, const QString &skill_name);
    void broadcastSkillInvoke(const QString &skillName);
    void broadcastSkillInvoke(const QString &skillName, const QString &category);
    void broadcastSkillInvoke(const QString &skillName, int type);
    void broadcastSkillInvoke(const QString &skillName, bool isMale, int type);
    void doLightbox(const QString &lightboxName, int duration = 2000, int pixelSize = 0);
    void doSuperLightbox(const QString &heroName, const QString &skillName);
    void doAnimate(QSanProtocol::AnimateType type, const QString &arg1 = QString(), const QString &arg2 = QString(),
        QList<ServerPlayer *> players = QList<ServerPlayer *>());

    inline void doAnimate(int type, const QString &arg1 = QString(), const QString &arg2 = QString(),
        QList<ServerPlayer *> players = QList<ServerPlayer *>())
    {
        doAnimate((QSanProtocol::AnimateType)type, arg1, arg2, players);
    }

    void preparePlayers();
    void changePlayerGeneral(ServerPlayer *player, const QString &new_general, bool loseskill = true);
    void changePlayerGeneral2(ServerPlayer *player, const QString &new_general, bool loseskill = true);
    void filterCards(ServerPlayer *player, QList<const Card *> cards, bool refilter);

    void acquireSkill(ServerPlayer *player, const Skill *skill, bool open = true);
    void acquireSkill(ServerPlayer *player, const QString &skill_name, bool open = true);
    void adjustSeats();
    void swapPile();
    inline QList<int> getDiscardPile()
    {
        return *m_discardPile;
    }
    inline QList<int> &getDrawPile()
    {
        return *m_drawPile;
    }
    int getCardFromPile(const QString &card_pattern);
    int newGetCardFromPile(const QString &card_pattern, bool include_discardpile = false) const;
    ServerPlayer *findPlayer(const QString &general_name, bool include_dead = false) const;
    QList<ServerPlayer *> findPlayersBySkillName(const QString &skill_name) const;
    ServerPlayer *findPlayerBySkillName(const QString &skill_name) const;
    ServerPlayer *findPlayerByObjectName(const QString &object_name) const;
    void installEquip(ServerPlayer *player, const QString &equip_name);
    void resetAI(ServerPlayer *player);
    void changeHero(ServerPlayer *player, const QString &new_general, bool full_state, bool invoke_start = true,
        bool isSecondaryHero = false, bool sendLog = true, ChangeHeroMode mode = Room::Thouroughly);
    void swapSeat(ServerPlayer *a, ServerPlayer *b);
    lua_State *getLuaState() const;
    void setFixedDistance(Player *from, const Player *to, int distance);
    void removeFixedDistance(Player *from, const Player *to, int distance);
    void insertAttackRangePair(Player *from, const Player *to);
    void removeAttackRangePair(Player *from, const Player *to);
    void reverseFor3v3(const Card *card, ServerPlayer *player, QList<ServerPlayer *> &list);
    bool hasWelfare(const ServerPlayer *player) const;
    ServerPlayer *getFront(ServerPlayer *a, ServerPlayer *b) const;
    void signup(ServerPlayer *player, const QString &screen_name, const QString &avatar, bool is_robot);
    ServerPlayer *getOwner() const;
    void updateStateItem();

    void reconnect(ServerPlayer *player, ClientSocket *socket);
    void marshal(ServerPlayer *player);

    void sortByActionOrder(QList<ServerPlayer *> &players);

    const ProhibitSkill *isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;

    void setTag(const QString &key, const QVariant &value);
    QVariant getTag(const QString &key) const;
    void removeTag(const QString &key);

    void setEmotion(ServerPlayer *target, const QString &emotion);

    Player::Place getCardPlace(int card_id) const;
    ServerPlayer *getCardOwner(int card_id) const;
    void setCardMapping(int card_id, ServerPlayer *owner, Player::Place place);

    void drawCards(ServerPlayer *player, int n, const QString &reason = QString());
    void drawCards(QList<ServerPlayer *> players, int n, const QString &reason = QString());
    void drawCards(QList<ServerPlayer *> players, QList<int> n_list, const QString &reason = QString());
    QList<int> newDrawCards(ServerPlayer *player, int n, const QString &reason = QString());
    void obtainCard(ServerPlayer *target, const Card *card, bool unhide = true);
    void obtainCard(ServerPlayer *target, int card_id, bool unhide = true);
    void obtainCard(ServerPlayer *target, const Card *card, const CardMoveReason &reason, bool unhide = true);

    void throwCard(int card_id, ServerPlayer *who, ServerPlayer *thrower = NULL);
    void throwCard(const Card *card, ServerPlayer *who, ServerPlayer *thrower = NULL);
    void throwCard(const Card *card, const CardMoveReason &reason, ServerPlayer *who, ServerPlayer *thrower = NULL);
    void throwCards(QList<int> card_ids, ServerPlayer *who, ServerPlayer *thrower);
    void throwCards(QList<int> card_ids, const CardMoveReason &reason, ServerPlayer *who, ServerPlayer *thrower = NULL);
    void throwCards(QList<const Card *> cards, const CardMoveReason &reason, ServerPlayer *who, ServerPlayer *thrower = NULL);

    void moveCardTo(const Card *card, ServerPlayer *dstPlayer, Player::Place dstPlace, bool forceMoveVisible = false);
    void moveCardTo(const Card *card, ServerPlayer *dstPlayer, Player::Place dstPlace, const CardMoveReason &reason,
        bool forceMoveVisible = false);
    void moveCardTo(const Card *card, ServerPlayer *srcPlayer, ServerPlayer *dstPlayer, Player::Place dstPlace,
        const CardMoveReason &reason, bool forceMoveVisible = false);
    void moveCardTo(const Card *card, ServerPlayer *srcPlayer, ServerPlayer *dstPlayer, Player::Place dstPlace,
        const QString &pileName, const CardMoveReason &reason, bool forceMoveVisible = false);
    void moveCardsAtomic(QList<CardsMoveStruct> cards_move, bool forceMoveVisible);
    void moveCardsAtomic(CardsMoveStruct cards_move, bool forceMoveVisible);
    QList<CardsMoveStruct> _breakDownCardMoves(QList<CardsMoveStruct> &cards_moves);

    // custom methods
    void setInfinityAttackRange(ServerPlayer *player, bool is_infinity);
    void setClientCustomData(QString str1, QString str2, int int1, int int2);
    QList<const Card *> getDiscardableCardsOfPlayer(ServerPlayer *from, ServerPlayer *to, bool include_equips = true, bool include_handcards = true);
    QList<const Skill *> searchSkillsWithText(const QString &text, bool include_banPackage = false, bool include_lordskill = true, bool include_activeskill = false, const char *skillType = NULL) const;
    QList<const TriggerSkill *> searchTriggerSkillsWithEvent(TriggerEvent triggerEvent = NonTrigger, bool include_banPackage = false, bool include_lordskill = true, bool include_activeskill = false) const;
    QList<const Skill *> searchSkillsWithSkillType(Skill::SkillType type, bool include_banPackage = false, bool include_lordskill = true, bool include_activeskill = false, const char *skillType = NULL) const;
    bool isArmorNullified(ServerPlayer *player);
    // make player's armor nullified.
    void setArmorNullified(ServerPlayer *player, bool is_nullified = true);
    QStack<DamageStruct> getCurrentDamageStack() const;
    QSet<QString> getAllGenerals(bool include_hidden, bool include_banPackage) const;
    // temporary lose skills
    void makePlayerOutOfGame(ServerPlayer *player, int round_num = 1);
    bool isSomeoneDying(ServerPlayer *player = NULL) const;
    int getDefaultDrawCards() const;

    // interactive methods
    void activate(ServerPlayer *player, CardUseStruct &card_use);
    void askForLuckCard();
    Card::Suit askForSuit(ServerPlayer *player, const QString &reason);
    QString askForKingdom(ServerPlayer *player, const QString &reason = QString());
    bool askForSkillInvokeSpecified(ServerPlayer *player, const QString &skill_name, bool AIInvokeValue, const QVariant &data = QVariant());
    bool askForSkillInvoke(ServerPlayer *player, const QString &skill_name, const QVariant &data = QVariant());
    bool askForSimpleChoice(ServerPlayer *player, const QString &skill_name, const QString &prompt, const QVariant &data = QVariant());
    QString askForChoice(ServerPlayer *player, const QString &skill_name, const QString &choices, const QVariant &data = QVariant());
    bool askForDiscard(ServerPlayer *target, const QString &reason, int discard_num, int min_num,
        bool optional = false, bool include_equip = false, const QString &prompt = QString(), const QString &pattern = ".");
    const Card *askForExchange(ServerPlayer *player, const QString &reason, int discard_num, int min_num,
        bool include_equip = false, const QString &prompt = QString(), bool optional = false, const QString &pattern = ".");
    bool askForNullification(const Card *trick, ServerPlayer *from, ServerPlayer *to, bool positive);
    bool isCanceled(const CardEffectStruct &effect);
    int askForCardChosen(ServerPlayer *player, ServerPlayer *who, const QString &flags, const QString &reason,
        bool handcard_visible = false, Card::HandlingMethod method = Card::MethodNone, const QList<int> &disabled_ids = QList<int>());
    const Card *askForCard(ServerPlayer *player, const QString &pattern, const QString &prompt, const QVariant &data, const QString &skill_name);
    const Card *askForCard(ServerPlayer *player, const QString &pattern, const QString &prompt, const QVariant &data = QVariant(),
        Card::HandlingMethod method = Card::MethodDiscard, ServerPlayer *to = NULL, bool isRetrial = false,
        const QString &skill_name = QString(), bool isProvision = false);
    const Card *askForUseCard(ServerPlayer *player, const QString &pattern, const QString &prompt, int notice_index = -1,
        Card::HandlingMethod method = Card::MethodUse, bool addHistory = true);
    const Card *askForUseSlashTo(ServerPlayer *slasher, ServerPlayer *victim, const QString &prompt,
        bool distance_limit = true, bool disable_extra = false, bool addHistory = false);
    const Card *askForUseSlashTo(ServerPlayer *slasher, QList<ServerPlayer *> victims, const QString &prompt,
        bool distance_limit = true, bool disable_extra = false, bool addHistory = false);
    int askForAG(ServerPlayer *player, const QList<int> &card_ids, bool refusable, const QString &reason);
    const Card *askForCardShow(ServerPlayer *player, ServerPlayer *requestor, const QString &reason);
    bool askForYiji(ServerPlayer *guojia, QList<int> &cards, const QString &skill_name = QString(),
        bool is_preview = false, bool visible = false, bool optional = true, int max_num = -1,
        QList<ServerPlayer *> players = QList<ServerPlayer *>(), CardMoveReason reason = CardMoveReason(),
        const QString &prompt = QString(), bool notify_skill = false);
    const Card *askForPindian(ServerPlayer *player, ServerPlayer *from, ServerPlayer *to, const QString &reason);
    QList<const Card *> askForPindianRace(ServerPlayer *from, ServerPlayer *to, const QString &reason);
    ServerPlayer *askForPlayerChosen(ServerPlayer *player, const QList<ServerPlayer *> &targets, const QString &reason,
        const QString &prompt = QString(), bool optional = false, bool notify_skill = false);
    QList<ServerPlayer *> askForPlayersChosen(ServerPlayer *player, const QList<ServerPlayer *> &targets, const QString &reason,
        const QString &prompt = QString(), bool optional = false, bool notify_skill = false);
    QString askForGeneral(ServerPlayer *player, QList<const General*> generals, QString default_choice = QString());
    QString askForGeneral(ServerPlayer *player, const QStringList &generals, QString default_choice = QString());
    QString askForGeneral(ServerPlayer *player, const QString &generals, QString default_choice = QString());
    const Card *askForSinglePeach(ServerPlayer *player, ServerPlayer *dying);
    void addPlayerHistory(ServerPlayer *player, const QString &key, int times = 1, bool broadcast = false);
    void askForThrow(ServerPlayer *player, ServerPlayer *target, int num, const QString &flags, const QString &reason,
                     bool handcard_visible = false, const QList<int> &disabled_ids = QList<int>());
    QString askForTemplateCard(ServerPlayer *player, const QString &skill_name, const QString &cards_name, const QVariant &data = QVariant());

    void toggleReadyCommand(ServerPlayer *player, const QVariant &);
    void speakCommand(ServerPlayer *player, const QString &arg);
    void speakCommand(ServerPlayer *player, const QVariant &arg);
    void trustCommand(ServerPlayer *player, const QVariant &arg);
    void pauseCommand(ServerPlayer *player, const QVariant &arg);
    void processResponse(ServerPlayer *player, const QSanProtocol::Packet *arg);
    void addRobotCommand(ServerPlayer *player, const QVariant &arg);
    void broadcastInvoke(const QSanProtocol::AbstractPacket *packet, ServerPlayer *except = NULL);
    void broadcastInvoke(const char *method, const QString &arg = ".", ServerPlayer *except = NULL);
    void networkDelayTestCommand(ServerPlayer *player, const QVariant &);

    inline RoomState *getRoomState()
    {
        return &_m_roomState;
    }
    inline Card *getCard(int cardId) const
    {
        return _m_roomState.getCard(cardId);
    }
    inline void resetCard(int cardId)
    {
        _m_roomState.resetCard(cardId);
    }
    void updateCardsOnLose(const CardsMoveStruct &move);
    void updateCardsOnGet(const CardsMoveStruct &move);

    int getBossModeExpMult(int level) const;

    void transformPlayer(ServerPlayer *player);
    // for transform mode.
    QStringList trans_AvailableGenerals;
    QStringList trans_CurrentUsedGenerals;
    QStringList trans_UsedGenerals;

protected:
    virtual void run();
    int _m_Id;

private:
    struct _MoveSourceClassifier
    {
        inline _MoveSourceClassifier(const CardsMoveStruct &move)
        {
            m_from = move.from; m_from_place = move.from_place;
            m_from_pile_name = move.from_pile_name; m_from_player_name = move.from_player_name;
        }
        inline void copyTo(CardsMoveStruct &move)
        {
            move.from = m_from; move.from_place = m_from_place;
            move.from_pile_name = m_from_pile_name; move.from_player_name = m_from_player_name;
        }
        inline bool operator ==(const _MoveSourceClassifier &other) const
        {
            return m_from == other.m_from && m_from_place == other.m_from_place
                && m_from_pile_name == other.m_from_pile_name && m_from_player_name == other.m_from_player_name;
        }
        inline bool operator <(const _MoveSourceClassifier &other) const
        {
            return m_from < other.m_from || m_from_place < other.m_from_place
                || m_from_pile_name < other.m_from_pile_name || m_from_player_name < other.m_from_player_name;
        }
        Player *m_from;
        Player::Place m_from_place;
        QString m_from_pile_name;
        QString m_from_player_name;
    };

    struct _MoveMergeClassifier
    {
        inline _MoveMergeClassifier(const CardsMoveStruct &move)
        {
            m_from = move.from; m_to = move.to;
            m_to_place = move.to_place;
            m_to_pile_name = move.to_pile_name;
        }
        inline bool operator ==(const _MoveMergeClassifier &other) const
        {
            return m_from == other.m_from && m_to == other.m_to
                && m_to_place == other.m_to_place && m_to_pile_name == other.m_to_pile_name;
        }
        inline bool operator < (const _MoveMergeClassifier &other) const
        {
            return m_from < other.m_from || m_to < other.m_to
                || m_to_place < other.m_to_place || m_to_pile_name < other.m_to_pile_name;
        }
        Player *m_from;
        Player *m_to;
        Player::Place m_to_place;
        QString m_to_pile_name;
    };

    struct _MoveSeparateClassifier
    {
        inline _MoveSeparateClassifier(const CardsMoveOneTimeStruct &moveOneTime, int index)
        {
            m_from = moveOneTime.from; m_to = moveOneTime.to;
            m_from_place = moveOneTime.from_places[index]; m_to_place = moveOneTime.to_place;
            m_from_pile_name = moveOneTime.from_pile_names[index]; m_to_pile_name = moveOneTime.to_pile_name;
            m_open = moveOneTime.open[index];
            m_reason = moveOneTime.reason;
            m_is_last_handcard = moveOneTime.is_last_handcard;
        }

        inline bool operator ==(const _MoveSeparateClassifier &other) const
        {
            return m_from == other.m_from && m_to == other.m_to
                && m_from_place == other.m_from_place && m_to_place == other.m_to_place
                && m_from_pile_name == other.m_from_pile_name && m_to_pile_name == other.m_to_pile_name
                && m_open == other.m_open && m_reason == other.m_reason && m_is_last_handcard == other.m_is_last_handcard;
        }
        inline bool operator < (const _MoveSeparateClassifier &other) const
        {
            return m_from < other.m_from && m_to < other.m_to
                && m_from_place < other.m_from_place && m_to_place < other.m_to_place
                && m_from_pile_name < other.m_from_pile_name && m_to_pile_name < other.m_to_pile_name
                && m_open < other.m_open && m_is_last_handcard < other.m_is_last_handcard;
        }
        Player *m_from;
        Player *m_to;
        Player::Place m_from_place, m_to_place;
        QString m_from_pile_name, m_to_pile_name;
        bool m_open;
        CardMoveReason m_reason;
        bool m_is_last_handcard;
    };

    int _m_lastMovementId;
    void _fillMoveInfo(CardsMoveStruct &moves, int card_index) const;
    QList<CardsMoveOneTimeStruct> _mergeMoves(QList<CardsMoveStruct> cards_moves);
    QList<CardsMoveStruct> _separateMoves(QList<CardsMoveOneTimeStruct> moveOneTimes);
    QString _chooseDefaultGeneral(ServerPlayer *player) const;
    bool _setPlayerGeneral(ServerPlayer *player, const QString &generalName, bool isFirst);
    QString mode;
    QList<ServerPlayer *> m_players, m_alivePlayers;
    int player_count;
    int default_draw_cards;
    ServerPlayer *current;
    QList<int> pile1, pile2;
    QList<int> table_cards;
    QList<int> *m_drawPile, *m_discardPile;
    QStack<DamageStruct> m_damageStack;
    bool game_started;
    bool game_finished;
    bool game_paused;
    lua_State *L;
    QList<AI *> ais;

    RoomThread *thread;
    RoomThread3v3 *thread_3v3;
    RoomThreadXMode *thread_xmode;
    RoomThread1v1 *thread_1v1;
    QSemaphore _m_semRaceRequest; // When race starts, server waits on his semaphore for the first replier
    QSemaphore _m_semRoomMutex; // Provide per-room  (rather than per-player) level protection of any shared variables


    QHash<QSanProtocol::CommandType, Callback> m_callbacks; // Stores the callbacks for client request. Do not use this
    // this map for anything else but S_CLIENT_REQUEST!!!!!
    QHash<QSanProtocol::CommandType, QSanProtocol::CommandType> m_requestResponsePair;
    // Stores the expected client response for each server request, any unmatched client response will be discarded.

    QTime _m_timeSinceLastSurrenderRequest; // Timer used to ensure that surrender polls are not initiated too frequently
    bool _m_isFirstSurrenderRequest; // We allow the first surrender poll to go through regardless of the timer.

    //helper variables for race request function
    bool _m_raceStarted;
    ServerPlayer *_m_raceWinner;

    QMap<int, Player::Place> place_map;
    QMap<int, ServerPlayer *> owner_map;

    const Card *provided;
    bool has_provided;

    QVariantMap tag;
    const Scenario *scenario;

    bool m_surrenderRequestReceived;
    bool _virtual;
    RoomState _m_roomState;

    QVariant m_fillAGarg;
    QVariant m_takeAGargs;

    QWaitCondition m_waitCond;
    mutable QMutex m_mutex;

    volatile bool playerPropertySet;
    //QMutex mutexPlayerProperty;
    //QWaitCondition wcPlayerProperty;

    static QString generatePlayerName();
    void prepareForStart();
    void assignGeneralsForPlayers(const QList<ServerPlayer *> &to_assign);
    void assignGeneralsForPlayersOfJianGeDefenseMode(const QList<ServerPlayer *> &to_assign);
    void assignGeneralsForPlayersOfHulaoMode(const QList<ServerPlayer *> &to_assign, const QString &pkgname);
    void assignGeneralsForPlayersOfTransformMode(const QList<ServerPlayer *> &to_assign);
    void chooseGenerals(QList<ServerPlayer *> players = QList<ServerPlayer *>());
    void chooseGeneralsOfJianGeDefenseMode();
    void chooseGeneralsOfHulaoMode(ServerPlayer *lord_player, const QString &lord_package = "hulao", bool enable2ndgeneral = true);
    AI *cloneAI(ServerPlayer *player);
    void broadcast(const QString &message, ServerPlayer *except = NULL);
    void initCallbacks();
    QString askForOrder(ServerPlayer *player, const QString &default_choice);
    QString askForRole(ServerPlayer *player, const QStringList &roles, const QString &scheme);

    //process client requests
    void processRequestCheat(ServerPlayer *player, const QVariant &arg);
    void processRequestSurrender(ServerPlayer *player, const QVariant &arg);

    bool makeSurrender(ServerPlayer *player);
    bool makeCheat(ServerPlayer *player);
    void makeDamage(const QString &source, const QString &target, QSanProtocol::CheatCategory nature, int point);
    void makeKilling(const QString &killer, const QString &victim);
    void makeReviving(const QString &name);
    void doScript(const QString &script);

    //helper functions and structs
    struct _NullificationAiHelper
    {
        const Card *m_trick;
        ServerPlayer *m_from;
        ServerPlayer *m_to;
    };
    bool _askForNullification(const Card *trick, ServerPlayer *from, ServerPlayer *to, bool positive, _NullificationAiHelper helper);
    void _setupChooseGeneralRequestArgs(ServerPlayer *player);
    int _getCardFromPile(const QString &card_pattern, const QList<int> &pile) const;

private slots:
    void reportDisconnection();
    void processClientPacket(const QString &packet);
    void assignRoles();
    void startGame();
    void slotSetProperty(ServerPlayer *player, const char *property_name, const QVariant &value);

signals:
    void room_message(const QString &msg);
    void game_start();
    void game_over(const QString &winner);
    void signalSetProperty(ServerPlayer *player, const char *property_name, const QVariant &value);
};

#endif

