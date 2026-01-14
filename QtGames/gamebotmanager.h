#ifndef GAMEBOTMANAGER_H
#define GAMEBOTMANAGER_H

#include <QObject>
#include <QSet>
#include <QPoint>
#include <QVector>
#include <QQueue>
#include <QHash>
#include "gameconstants.h"

class GameEngine;
class Player;

class GameBotManager : public QObject
{
    Q_OBJECT
public:
    explicit GameBotManager(GameEngine *engine);
    ~GameBotManager();

    void spawnBots();
    void clearBots();
    void updateBots();

public:
    QList<Player*> getBots() const; // 获取所有AI机器人列表
    void removeBot(Player *bot);    // 移除指定的AI机器人

private:
    GameEngine *m_engine;
    QVector<Player*> m_bots;

    QPoint snapPlayerCell(Player *p) const;
    QSet<QPoint> computeDangerCells() const;
    QSet<QPoint> computeFutureDangerCells(int timeSteps = 3) const; // 预测未来危险区域
    bool isCellWalkable(int x, int y) const;
    bool isCellInBombRange(const QPoint &cell) const; // 检查单元格是否在炸弹爆炸范围内
    bool bfsNextStep(const QPoint &start, const QPoint &goal, const QSet<QPoint> &danger, QPoint &nextStep) const;
    bool canReachPlayer(const QPoint &botCell, const QPoint &playerCell, const QSet<QPoint> &danger) const;
    bool findNearestBrickStep(const QPoint &botCell, const QSet<QPoint> &danger, QPoint &nextStep) const;
    bool findSafeStep(const QPoint &botCell, const QSet<QPoint> &danger, QPoint &nextStep) const;
    bool findEscapeRoute(const QPoint &botCell, const QSet<QPoint> &danger, QPoint &nextStep) const; // 寻找最佳逃生路径
    bool hasDestructibleBrickInRange(Player *bot, const QSet<QPoint> &danger) const; // 检查机器人附近是否有可破坏的砖块
    QVector<QPoint> neighbors(const QPoint &cell) const;

    // 改进的AI行为函数
    bool moveBotToward(Player *bot, const QPoint &target, const QSet<QPoint> &danger);
    bool moveBotAway(Player *bot, const QSet<QPoint> &danger);
    void botPlaceBomb(Player *bot);
    bool shouldPlaceBomb(Player *bot, const QSet<QPoint> &danger) const; // 决定是否放置炸弹
    bool canTrapTarget(Player *bot, const QPoint &targetPos) const; // 检查是否可以困住目标
    QSet<QPoint> getBombDangerArea(const QPoint &bombPos) const; // 获取炸弹爆炸危险区域
    bool isPlayerInBombRange(Player *bot, Player *target) const; // 检查目标是否在爆炸范围内
    void prioritizeTargets(QVector<Player*> &targets, Player *currentBot) const; // 优先级排序目标
};

#endif // GAMEBOTMANAGER_H
