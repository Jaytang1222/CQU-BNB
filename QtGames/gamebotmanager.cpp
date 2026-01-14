#include "gamebotmanager.h"
#include "gameengine.h"
#include "player.h"
#include "block.h"
#include "bomb.h"
#include <QGraphicsScene>
#include <QRandomGenerator>
#include <QBrush>
#include <QPen>
#include <cmath>
#include <limits>
#include <queue>

GameBotManager::GameBotManager(GameEngine *engine)
    : QObject(engine)
    , m_engine(engine)
{
}

GameBotManager::~GameBotManager()
{
    clearBots();
}

void GameBotManager::spawnBots()
{
    clearBots();
    if (!m_engine) return;

    // 三个角落（除玩家出生点外）
    int w = m_engine->mapWidth();
    int h = m_engine->mapHeight();
    QVector<QPoint> spawnCells = {
        QPoint(w - Block::SIZE_UNITS * 2, Block::SIZE_UNITS),
        QPoint(Block::SIZE_UNITS, h - Block::SIZE_UNITS * 2),
        QPoint(w - Block::SIZE_UNITS * 2, h - Block::SIZE_UNITS * 2)
    };

    for (const QPoint &cell : spawnCells) {
        if (!isCellWalkable(cell.x(), cell.y())) continue;
        Player *bot = new Player(cell.x(), cell.y());
        bot->setBrush(QBrush(Qt::darkGreen));
        bot->setPen(QPen(Qt::black, 1));
        bot->setZValue(5); // Bot 层级高于炸弹
        if (m_engine->scene()) {
            m_engine->scene()->addItem(bot);
        }
        m_bots.append(bot);
    }
}

void GameBotManager::clearBots()
{
    for (Player *bot : m_bots) {
        if (m_engine && m_engine->scene()) {
            m_engine->scene()->removeItem(bot);
        }
        delete bot;
    }
    m_bots.clear();
}

void GameBotManager::updateBots()
{
    if (!m_engine) return;
    Player *player = m_engine->getPlayer();
    if (!player) return;

    // 计算当前和未来的危险区域
    QSet<QPoint> currentDanger = computeDangerCells();
    QSet<QPoint> futureDanger = computeFutureDangerCells(2); // 预测2步后的危险区域
    QSet<QPoint> allDanger = currentDanger;
    allDanger.unite(futureDanger);
    
    QPoint playerCell = snapPlayerCell(player);

    // 收集所有可攻击的目标（玩家和其他机器人）
    QVector<Player*> allTargets;
    allTargets.append(player); // 添加人类玩家作为目标
    for (Player *bot : m_bots) {
        if (bot && bot != player) {
            allTargets.append(bot); // 添加其他机器人作为目标
        }
    }

    for (Player *bot : m_bots) {
        if (!bot) continue;
        QPoint botCell = snapPlayerCell(bot);

        // 1) 紧急躲避：立即危险
        if (currentDanger.contains(botCell)) {
            QPoint escapeStep;
            if (findEscapeRoute(botCell, allDanger, escapeStep)) {
                if (escapeStep == QPoint(1,0)) bot->moveRight();
                else if (escapeStep == QPoint(-1,0)) bot->moveLeft();
                else if (escapeStep == QPoint(0,1)) bot->moveDown();
                else if (escapeStep == QPoint(0,-1)) bot->moveUp();
                continue;
            } else {
                // 如果找不到明确的逃生路线，尝试任何可能的安全移动
                if (findSafeStep(botCell, allDanger, escapeStep)) {
                    if (escapeStep == QPoint(1,0)) bot->moveRight();
                    else if (escapeStep == QPoint(-1,0)) bot->moveLeft();
                    else if (escapeStep == QPoint(0,1)) bot->moveDown();
                    else if (escapeStep == QPoint(0,-1)) bot->moveUp();
                    continue;
                }
            }
        }

        // 2) 预测性躲避：避开预测危险区域
        if (futureDanger.contains(botCell)) {
            QPoint safeStep;
            if (findSafeStep(botCell, allDanger, safeStep)) {
                if (safeStep == QPoint(1,0)) bot->moveRight();
                else if (safeStep == QPoint(-1,0)) bot->moveLeft();
                else if (safeStep == QPoint(0,1)) bot->moveDown();
                else if (safeStep == QPoint(0,-1)) bot->moveUp();
                continue;
            }
        }

        // 3) 优先攻击最近的目标
        prioritizeTargets(allTargets, bot); // 按距离当前bot的远近排序目标
        
        bool movedToAttack = false;
        
        for (Player *target : allTargets) {
            if (!target || target == bot) continue; // 不攻击自己
            
            QPoint targetCell = snapPlayerCell(target);
            int distance = std::abs(targetCell.x() - botCell.x()) + std::abs(targetCell.y() - botCell.y());
            
            // 如果目标在爆炸范围内，考虑放置炸弹
            if (distance <= Block::SIZE_UNITS * 2 && isPlayerInBombRange(bot, target)) {
                if (shouldPlaceBomb(bot, allDanger)) {
                    botPlaceBomb(bot);
                    // 放置炸弹后逃离该区域
                    moveBotAway(bot, allDanger);
                    movedToAttack = true;
                    break;
                }
            }
            
            // 尝试接近目标
            if (moveBotToward(bot, targetCell, allDanger)) {
                movedToAttack = true;
                break;
            }
        }
        
        if (movedToAttack) {
            continue;
        }

        // 4) 如果没有合适的目标，去拆砖或探索
        QPoint brickStep;
        if (findNearestBrickStep(botCell, allDanger, brickStep)) {
            // 检查是否靠近砖块并且可以放置炸弹开路
            if (shouldPlaceBomb(bot, allDanger)) {
                botPlaceBomb(bot);
                // 放置炸弹后逃离该区域
                moveBotAway(bot, allDanger);
                continue;
            }
            
            if (brickStep == QPoint(1,0)) bot->moveRight();
            else if (brickStep == QPoint(-1,0)) bot->moveLeft();
            else if (brickStep == QPoint(0,1)) bot->moveDown();
            else if (brickStep == QPoint(0,-1)) bot->moveUp();
            continue;
        }

        // 5) 检查附近是否有可破坏的砖块，如果有则放置炸弹
        if (hasDestructibleBrickInRange(bot, allDanger)) {
            if (shouldPlaceBomb(bot, allDanger)) {
                botPlaceBomb(bot);
                // 放置炸弹后逃离该区域
                moveBotAway(bot, allDanger);
                continue;
            }
        }

        // 6) 最后尝试接近玩家的贪心一步
        // 在接近玩家的过程中，也可以考虑放置炸弹
        if (shouldPlaceBomb(bot, allDanger)) {
            botPlaceBomb(bot);
            // 放置炸弹后逃离该区域
            moveBotAway(bot, allDanger);
            continue;
        }
        
        moveBotToward(bot, playerCell, allDanger);
    }
}

QPoint GameBotManager::snapPlayerCell(Player *p) const
{
    int cx = static_cast<int>(std::round(p->x() / GameConstants::UNIT_SIZE));
    int cy = static_cast<int>(std::round(p->y() / GameConstants::UNIT_SIZE));
    return QPoint(cx, cy);
}

QSet<QPoint> GameBotManager::computeDangerCells() const
{
    QSet<QPoint> danger;
    const QList<Bomb*> &bombs = m_engine->bombs();
    for (Bomb *bomb : bombs) {
        if (!bomb) continue;
        QList<QPoint> cells = bomb->getExplosionPositions();
        for (const QPoint &c : cells) {
            danger.insert(c);
        }
    }
    return danger;
}

bool GameBotManager::isCellWalkable(int x, int y) const
{
    return m_engine->isCellWalkable(x, y);
}

QVector<QPoint> GameBotManager::neighbors(const QPoint &cell) const
{
    QVector<QPoint> res;
    QVector<QPoint> dirs = {QPoint(1,0), QPoint(-1,0), QPoint(0,1), QPoint(0,-1)};
    // 使用与玩家一致的步长（1 个逻辑单位），避免因为整格步长而卡住
    for (const QPoint &d : dirs) {
        res.append(cell + d * GameConstants::LOGIC_UNIT);
    }
    return res;
}

bool GameBotManager::bfsNextStep(const QPoint &start, const QPoint &goal, const QSet<QPoint> &danger, QPoint &nextStep) const
{
    if (start == goal) {
        nextStep = QPoint(0,0);
        return true;
    }
    QSet<QPoint> visited;
    std::queue<QPoint> q;
    QHash<QPoint, QPoint> parent;
    q.push(start);
    visited.insert(start);

    while (!q.empty()) {
        QPoint cur = q.front(); q.pop();
        for (const QPoint &nb : neighbors(cur)) {
            if (visited.contains(nb)) continue;
            if (!isCellWalkable(nb.x(), nb.y())) continue;
            if (danger.contains(nb)) continue;
            visited.insert(nb);
            parent.insert(nb, cur);
            if (nb == goal) {
                // 回溯找下一步
                QPoint step = nb;
                while (parent.value(step) != start) {
                    step = parent.value(step);
                }
                nextStep = step - start;
                return true;
            }
            q.push(nb);
        }
    }
    return false;
}

bool GameBotManager::canReachPlayer(const QPoint &botCell, const QPoint &playerCell, const QSet<QPoint> &danger) const
{
    QPoint step;
    return bfsNextStep(botCell, playerCell, danger, step);
}

bool GameBotManager::findSafeStep(const QPoint &botCell, const QSet<QPoint> &danger, QPoint &nextStep) const
{
    // 多目标 BFS，目标是任意不在 danger 的格
    QSet<QPoint> visited;
    std::queue<QPoint> q;
    QHash<QPoint, QPoint> parent;
    q.push(botCell);
    visited.insert(botCell);

    while (!q.empty()) {
        QPoint cur = q.front(); q.pop();
        if (!danger.contains(cur)) {
            if (cur == botCell) {
                nextStep = QPoint(0,0);
                return true;
            }
            QPoint step = cur;
            while (parent.value(step) != botCell) {
                step = parent.value(step);
            }
            nextStep = step - botCell;
            return true;
        }
        for (const QPoint &nb : neighbors(cur)) {
            if (visited.contains(nb)) continue;
            if (!isCellWalkable(nb.x(), nb.y())) continue;
            visited.insert(nb);
            parent.insert(nb, cur);
            q.push(nb);
        }
    }
    return false;
}

QSet<QPoint> GameBotManager::computeFutureDangerCells(int timeSteps) const
{
    QSet<QPoint> futureDanger;
    const QList<Bomb*> &bombs = m_engine->bombs();
    
    // 计算所有未爆炸的炸弹在timeSteps步后会爆炸的区域
    for (Bomb *bomb : bombs) {
        if (!bomb || bomb->isExploding()) continue;
        
        // 假设炸弹会在timeSteps步后爆炸，计算其影响范围
        QList<QPoint> cells = bomb->getExplosionPositions();
        for (const QPoint &c : cells) {
            futureDanger.insert(c);
        }
    }
    return futureDanger;
}

bool GameBotManager::isCellInBombRange(const QPoint &cell) const
{
    const QList<Bomb*> &bombs = m_engine->bombs();
    for (Bomb *bomb : bombs) {
        if (!bomb || bomb->isExploding()) continue;
        
        QList<QPoint> explosionPositions = bomb->getExplosionPositions();
        if (explosionPositions.contains(cell)) {
            return true;
        }
    }
    return false;
}

bool GameBotManager::findEscapeRoute(const QPoint &botCell, const QSet<QPoint> &danger, QPoint &nextStep) const
{
    // 使用更智能的逃生算法，优先选择远离炸弹的方向
    QVector<QPoint> directions = {QPoint(1,0), QPoint(-1,0), QPoint(0,1), QPoint(0,-1)};
    QVector<QPoint> safeOptions;
    
    // 检查所有相邻的可通行位置
    for (const QPoint &dir : directions) {
        QPoint nextPos = botCell + dir * GameConstants::LOGIC_UNIT;
        if (isCellWalkable(nextPos.x(), nextPos.y()) && !danger.contains(nextPos)) {
            safeOptions.append(dir);
        }
    }
    
    if (safeOptions.isEmpty()) {
        return false;
    }
    
    // 如果有多个安全选项，选择最远离当前危险的方向
    if (safeOptions.size() > 1) {
        // 计算每个方向的"安全值"（远离危险的程度）
        QPoint bestDir = safeOptions[0];
        int bestSafety = -1;
        
        for (const QPoint &dir : safeOptions) {
            QPoint testPos = botCell + dir * GameConstants::LOGIC_UNIT;
            int safety = 0;
            
            // 计算到最近危险点的距离
            for (const QPoint &dangerPos : danger) {
                int dist = std::abs(testPos.x() - dangerPos.x()) + std::abs(testPos.y() - dangerPos.y());
                safety += dist; // 累加所有危险点的距离
            }
            
            if (safety > bestSafety) {
                bestSafety = safety;
                bestDir = dir;
            }
        }
        
        nextStep = bestDir;
    } else {
        nextStep = safeOptions[0];
    }
    
    return true;
}

bool GameBotManager::shouldPlaceBomb(Player *bot, const QSet<QPoint> &danger) const
{
    if (!bot) return false;
    
    QPoint botCell = snapPlayerCell(bot);
    
    // 检查附近是否有可破坏的砖块或玩家/机器人
    bool hasTarget = false;
    bool hasDestructibleBlock = false;
    
    // 检查爆炸范围内是否有可破坏的砖块（用于开路）
    int bombRange = 1; // 默认炸弹范围为1个单位
    for (int i = 0; i <= bombRange; ++i) { // 检查包括当前位置在内的爆炸范围
        // 检查上下左右方向
        QPoint center(botCell.x(), botCell.y());
        QPoint up(botCell.x(), botCell.y() - i * Block::SIZE_UNITS);
        QPoint down(botCell.x(), botCell.y() + i * Block::SIZE_UNITS);
        QPoint left(botCell.x() - i * Block::SIZE_UNITS, botCell.y());
        QPoint right(botCell.x() + i * Block::SIZE_UNITS, botCell.y());
        
        QVector<QPoint> checkPositions;
        if (i == 0) {
            checkPositions.append(center); // 当i=0时，检查中心位置
        } else {
            checkPositions = {up, down, left, right}; // 当i>0时，检查四周
        }
        
        for (const QPoint &pos : checkPositions) {
            // 检查该位置是否有可破坏的砖块
            const QList<Block*> &blocks = m_engine->blocks();
            for (Block *block : blocks) {
                if (block->isDestructible() && 
                    static_cast<int>(block->x() / GameConstants::UNIT_SIZE) == pos.x() &&
                    static_cast<int>(block->y() / GameConstants::UNIT_SIZE) == pos.y()) {
                    hasDestructibleBlock = true;
                    hasTarget = true; // 将可破坏砖块也视为目标
                    break;
                }
            }
            
            // 检查该位置是否有玩家或机器人
            Player *player = m_engine->getPlayer();
            if (player && snapPlayerCell(player) == pos) {
                hasTarget = true;
                break;
            }
            
            for (Player *otherBot : m_bots) {
                if (otherBot && otherBot != bot && snapPlayerCell(otherBot) == pos) {
                    hasTarget = true;
                    break;
                }
            }
        }
    }
    
    // 检查放置炸弹后是否有逃生路线
    QSet<QPoint> bombDanger = getBombDangerArea(botCell);
    
    // 检查是否能在放置炸弹后逃离
    QPoint escapeStep;
    QSet<QPoint> totalDanger = danger;
    totalDanger.unite(bombDanger);
    if (findSafeStep(botCell, totalDanger, escapeStep)) {
        return hasTarget; // 如果有目标且能逃生，则放置炸弹
    }
    
    // 如果机器人在可破坏砖块旁边，即使暂时没有安全的逃生路线，也可以考虑放置炸弹
    // 因为机器人可能能够移动到安全位置后再引爆
    if (hasDestructibleBlock) {
        // 检查是否有任何可以移动的方向
        QVector<QPoint> directions = {QPoint(1,0), QPoint(-1,0), QPoint(0,1), QPoint(0,-1)};
        for (const QPoint &dir : directions) {
            QPoint nextPos = botCell + dir * GameConstants::LOGIC_UNIT;
            if (isCellWalkable(nextPos.x(), nextPos.y()) && !danger.contains(nextPos)) {
                // 如果可以移动到安全位置，可以放置炸弹然后移动
                return true;
            }
        }
    }
    
    return false;
}

bool GameBotManager::isPlayerInBombRange(Player *bot, Player *target) const
{
    if (!bot || !target) return false;
    
    QPoint botCell = snapPlayerCell(bot);
    QPoint targetCell = snapPlayerCell(target);
    
    // 检查目标是否在炸弹爆炸范围内（十字形范围）
    int dx = std::abs(targetCell.x() - botCell.x());
    int dy = std::abs(targetCell.y() - botCell.y());
    
    // 如果在同一行或同一列，且距离在爆炸范围内
    if ((dx == 0 && dy <= Block::SIZE_UNITS) || 
        (dy == 0 && dx <= Block::SIZE_UNITS)) {
        // 检查中间路径是否畅通（没有墙阻挡）
        int minX = std::min(botCell.x(), targetCell.x());
        int maxX = std::max(botCell.x(), targetCell.x());
        int minY = std::min(botCell.y(), targetCell.y());
        int maxY = std::max(botCell.y(), targetCell.y());
        
        bool pathClear = true;
        if (dx == 0) { // 同一列
            for (int y = minY; y <= maxY; y += GameConstants::LOGIC_UNIT) {
                if (y != botCell.y()) { // 跳过机器人自己的位置
                    QPoint pos(botCell.x(), y);
                    if (!isCellWalkable(pos.x(), pos.y()) && 
                        !isCellInBombRange(pos)) { // 如果是不可通行的墙而非空路径
                        // 检查是否是砖块
                        bool isBrick = false;
                        const QList<Block*> &blocks = m_engine->blocks();
                        for (Block *block : blocks) {
                            if (static_cast<int>(block->x() / GameConstants::UNIT_SIZE) == pos.x() &&
                                static_cast<int>(block->y() / GameConstants::UNIT_SIZE) == pos.y() &&
                                !block->isDestructible()) { // 是不可破坏的墙
                                isBrick = true;
                                break;
                            }
                        }
                        if (isBrick) {
                            pathClear = false;
                            break;
                        }
                    }
                }
            }
        } else if (dy == 0) { // 同一行
            for (int x = minX; x <= maxX; x += GameConstants::LOGIC_UNIT) {
                if (x != botCell.x()) { // 跳过机器人自己的位置
                    QPoint pos(x, botCell.y());
                    if (!isCellWalkable(pos.x(), pos.y()) && 
                        !isCellInBombRange(pos)) {
                        // 检查是否是砖块
                        bool isBrick = false;
                        const QList<Block*> &blocks = m_engine->blocks();
                        for (Block *block : blocks) {
                            if (static_cast<int>(block->x() / GameConstants::UNIT_SIZE) == pos.x() &&
                                static_cast<int>(block->y() / GameConstants::UNIT_SIZE) == pos.y() &&
                                !block->isDestructible()) { // 是不可破坏的墙
                                isBrick = true;
                                break;
                            }
                        }
                        if (isBrick) {
                            pathClear = false;
                            break;
                        }
                    }
                }
            }
        }
        
        return pathClear;
    }
    
    return false;
}

bool GameBotManager::canTrapTarget(Player *bot, const QPoint &targetPos) const
{
    if (!bot) return false;
    
    QPoint botCell = snapPlayerCell(bot);
    
    // 检查是否可以通过放置炸弹困住目标
    // 基本思路：检查目标周围是否有足够的障碍物，使得放置炸弹后目标无法逃脱
    
    // 获取目标周围的四个方向
    QVector<QPoint> dirs = {QPoint(1,0), QPoint(-1,0), QPoint(0,1), QPoint(0,-1)};
    int trappedDirections = 0;
    
    for (const QPoint &dir : dirs) {
        QPoint adjacentPos = targetPos + dir * Block::SIZE_UNITS;
        
        // 检查这个方向是否被阻挡（墙壁、边界或其他不可通行区域）
        if (!isCellWalkable(adjacentPos.x(), adjacentPos.y())) {
            trappedDirections++;
        } else {
            // 检查是否在当前bot的炸弹爆炸范围内，如果在范围内则也可以视为被阻挡
            int dx = std::abs(adjacentPos.x() - botCell.x());
            int dy = std::abs(adjacentPos.y() - botCell.y());
            
            if ((dx == 0 && dy <= Block::SIZE_UNITS) || 
                (dy == 0 && dx <= Block::SIZE_UNITS)) {
                // 检查中间路径是否畅通
                int minX = std::min(botCell.x(), adjacentPos.x());
                int maxX = std::max(botCell.x(), adjacentPos.x());
                int minY = std::min(botCell.y(), adjacentPos.y());
                int maxY = std::max(botCell.y(), adjacentPos.y());
                
                bool pathClear = true;
                if (dx == 0) { // 同一列
                    for (int y = minY; y <= maxY; y += GameConstants::LOGIC_UNIT) {
                        if (y != botCell.y()) {
                            QPoint pos(botCell.x(), y);
                            if (!isCellWalkable(pos.x(), pos.y())) {
                                pathClear = false;
                                break;
                            }
                        }
                    }
                } else if (dy == 0) { // 同一行
                    for (int x = minX; x <= maxX; x += GameConstants::LOGIC_UNIT) {
                        if (x != botCell.x()) {
                            QPoint pos(x, botCell.y());
                            if (!isCellWalkable(pos.x(), pos.y())) {
                                pathClear = false;
                                break;
                            }
                        }
                    }
                }
                
                if (pathClear) {
                    // 这个方向将被炸弹覆盖，所以也被视为被困住
                    trappedDirections++;
                }
            }
        }
    }
    
    // 如果超过2个方向被阻挡，则认为可能困住目标
    return trappedDirections >= 3;
}

QSet<QPoint> GameBotManager::getBombDangerArea(const QPoint &bombPos) const
{
    QSet<QPoint> dangerArea;
    QPoint bombCell(bombPos.x() / Block::SIZE_UNITS * Block::SIZE_UNITS, 
                    bombPos.y() / Block::SIZE_UNITS * Block::SIZE_UNITS);
    
    // 添加炸弹中心位置
    dangerArea.insert(bombCell);
    
    // 向四个方向扩展爆炸范围
    for (int i = 1; i <= 1; ++i) { // 默认爆炸范围为1个单位
        QPoint up(bombCell.x(), bombCell.y() - i * Block::SIZE_UNITS);
        QPoint down(bombCell.x(), bombCell.y() + i * Block::SIZE_UNITS);
        QPoint left(bombCell.x() - i * Block::SIZE_UNITS, bombCell.y());
        QPoint right(bombCell.x() + i * Block::SIZE_UNITS, bombCell.y());
        
        // 检查向上方向
        if (isCellWalkable(up.x(), up.y())) {
            dangerArea.insert(up);
        } else {
            // 如果遇到不可通行的障碍物（墙或砖块），停止扩展
            const QList<Block*> &blocks = m_engine->blocks();
            bool isWall = true;
            for (Block *block : blocks) {
                if (static_cast<int>(block->x() / GameConstants::UNIT_SIZE) == up.x() &&
                    static_cast<int>(block->y() / GameConstants::UNIT_SIZE) == up.y()) {
                    if (block->isDestructible()) {
                        // 可破坏的砖块，仍会爆炸到那里
                        dangerArea.insert(up);
                    }
                    isWall = false;
                    break;
                }
            }
            if (isWall) break; // 如果是不可破坏的墙，停止扩展
        }
        
        // 检查向下方向
        if (isCellWalkable(down.x(), down.y())) {
            dangerArea.insert(down);
        } else {
            const QList<Block*> &blocks = m_engine->blocks();
            bool isWall = true;
            for (Block *block : blocks) {
                if (static_cast<int>(block->x() / GameConstants::UNIT_SIZE) == down.x() &&
                    static_cast<int>(block->y() / GameConstants::UNIT_SIZE) == down.y()) {
                    if (block->isDestructible()) {
                        dangerArea.insert(down);
                    }
                    isWall = false;
                    break;
                }
            }
            if (isWall) break;
        }
        
        // 检查向左方向
        if (isCellWalkable(left.x(), left.y())) {
            dangerArea.insert(left);
        } else {
            const QList<Block*> &blocks = m_engine->blocks();
            bool isWall = true;
            for (Block *block : blocks) {
                if (static_cast<int>(block->x() / GameConstants::UNIT_SIZE) == left.x() &&
                    static_cast<int>(block->y() / GameConstants::UNIT_SIZE) == left.y()) {
                    if (block->isDestructible()) {
                        dangerArea.insert(left);
                    }
                    isWall = false;
                    break;
                }
            }
            if (isWall) break;
        }
        
        // 检查向右方向
        if (isCellWalkable(right.x(), right.y())) {
            dangerArea.insert(right);
        } else {
            const QList<Block*> &blocks = m_engine->blocks();
            bool isWall = true;
            for (Block *block : blocks) {
                if (static_cast<int>(block->x() / GameConstants::UNIT_SIZE) == right.x() &&
                    static_cast<int>(block->y() / GameConstants::UNIT_SIZE) == right.y()) {
                    if (block->isDestructible()) {
                        dangerArea.insert(right);
                    }
                    isWall = false;
                    break;
                }
            }
            if (isWall) break;
        }
    }
    
    return dangerArea;
}

void GameBotManager::prioritizeTargets(QVector<Player*> &targets, Player *currentBot) const
{
    if (!currentBot) return;
    
    QPoint botCell = snapPlayerCell(currentBot);
    
    // 按照与当前bot的距离排序目标
    std::sort(targets.begin(), targets.end(), [this, botCell](Player *a, Player *b) {
        if (!a || !b) return false; // 安全检查
        QPoint cellA = snapPlayerCell(a);
        QPoint cellB = snapPlayerCell(b);
        int distA = std::abs(cellA.x() - botCell.x()) + std::abs(cellA.y() - botCell.y());
        int distB = std::abs(cellB.x() - botCell.x()) + std::abs(cellB.y() - botCell.y());
        return distA < distB;
    });
}

QList<Player*> GameBotManager::getBots() const
{
    QList<Player*> result;
    for (Player *bot : m_bots) {
        if (bot) {
            result.append(bot);
        }
    }
    return result;
}

void GameBotManager::removeBot(Player *bot)
{
    if (bot) {
        m_bots.removeAll(bot);
    }
}

bool GameBotManager::findNearestBrickStep(const QPoint &botCell, const QSet<QPoint> &danger, QPoint &nextStep) const
{
    // 先收集所有砖块格
    QList<QPoint> bricks;
    const QList<Block*> &blocks = m_engine->blocks();
    for (Block *b : blocks) {
        if (!b->isDestructible()) continue;
        QPoint bc(static_cast<int>(b->x() / GameConstants::UNIT_SIZE),
                  static_cast<int>(b->y() / GameConstants::UNIT_SIZE));
        bricks.append(bc);
    }
    if (bricks.isEmpty()) return false;

    // 多目标 BFS：从bot出发，遇到任一砖块即返回
    QSet<QPoint> targets = QSet<QPoint>(bricks.begin(), bricks.end());
    QSet<QPoint> visited;
    std::queue<QPoint> q;
    QHash<QPoint, QPoint> parent;
    q.push(botCell);
    visited.insert(botCell);

    while (!q.empty()) {
        QPoint cur = q.front(); q.pop();
        if (targets.contains(cur)) {
            // 回溯找下一步
            if (cur == botCell) {
                nextStep = QPoint(0,0);
                return true;
            }
            QPoint step = cur;
            while (parent.value(step) != botCell) {
                step = parent.value(step);
            }
            nextStep = step - botCell;
            return true;
        }
        for (const QPoint &nb : neighbors(cur)) {
            if (visited.contains(nb)) continue;
            if (!isCellWalkable(nb.x(), nb.y())) continue;
            if (danger.contains(nb)) continue;
            visited.insert(nb);
            parent.insert(nb, cur);
            q.push(nb);
        }
    }
    return false;
}

bool GameBotManager::moveBotToward(Player *bot, const QPoint &target, const QSet<QPoint> &danger)
{
    QPoint botCell = snapPlayerCell(bot);
    int dx = target.x() - botCell.x();
    int dy = target.y() - botCell.y();
    QPoint step(
        (dx == 0) ? 0 : (dx > 0 ? 1 : -1),
        (dy == 0) ? 0 : (dy > 0 ? 1 : -1)
    );

    // 优先大轴向
    QVector<QPoint> options;
    if (std::abs(dx) >= std::abs(dy)) {
        options << QPoint(step.x(), 0) << QPoint(0, step.y());
    } else {
        options << QPoint(0, step.y()) << QPoint(step.x(), 0);
    }
    for (const QPoint &o : options) {
        QPoint nc = botCell + o * GameConstants::LOGIC_UNIT; // 与玩家同步的1单位步长
        if (!isCellWalkable(nc.x(), nc.y())) continue;
        if (danger.contains(nc)) continue;
        if (o == QPoint(1,0)) bot->moveRight();
        else if (o == QPoint(-1,0)) bot->moveLeft();
        else if (o == QPoint(0,1)) bot->moveDown();
        else if (o == QPoint(0,-1)) bot->moveUp();
        return true;
    }
    return false;
}

bool GameBotManager::moveBotAway(Player *bot, const QSet<QPoint> &danger)
{
    QPoint botCell = snapPlayerCell(bot);
    if (!danger.contains(botCell)) return false;
    QVector<QPoint> dirs = {QPoint(1,0), QPoint(-1,0), QPoint(0,1), QPoint(0,-1)};
    for (const QPoint &d : dirs) {
        QPoint nc = botCell + d * GameConstants::LOGIC_UNIT; // 与玩家同步的1单位步长
        if (!isCellWalkable(nc.x(), nc.y())) continue;
        if (danger.contains(nc)) continue;
        if (d == QPoint(1,0)) bot->moveRight();
        else if (d == QPoint(-1,0)) bot->moveLeft();
        else if (d == QPoint(0,1)) bot->moveDown();
        else if (d == QPoint(0,-1)) bot->moveUp();
        return true;
    }
    return false;
}

bool GameBotManager::hasDestructibleBrickInRange(Player *bot, const QSet<QPoint> &danger) const
{
    if (!bot) return false;
    
    QPoint botCell = snapPlayerCell(bot);
    
    // 检查炸弹爆炸范围内是否有可破坏的砖块
    int bombRange = 1; // 默认炸弹范围为1个单位
    
    for (int i = 0; i <= bombRange; ++i) { // 从当前位置开始（包括原地）
        // 检查上下左右方向
        QPoint up(botCell.x(), botCell.y() - i * Block::SIZE_UNITS);
        QPoint down(botCell.x(), botCell.y() + i * Block::SIZE_UNITS);
        QPoint left(botCell.x() - i * Block::SIZE_UNITS, botCell.y());
        QPoint right(botCell.x() + i * Block::SIZE_UNITS, botCell.y());
        
        QVector<QPoint> checkPositions = {botCell, up, down, left, right}; // 包括当前位置
        
        for (const QPoint &pos : checkPositions) {
            if (pos == botCell) continue; // 跳过机器人自身位置
            
            // 检查该位置是否有可破坏的砖块
            const QList<Block*> &blocks = m_engine->blocks();
            for (Block *block : blocks) {
                if (block->isDestructible() && 
                    static_cast<int>(block->x() / GameConstants::UNIT_SIZE) == pos.x() &&
                    static_cast<int>(block->y() / GameConstants::UNIT_SIZE) == pos.y()) {
                    return true; // 找到可破坏的砖块
                }
            }
        }
    }
    
    return false;
}

void GameBotManager::botPlaceBomb(Player *bot)
{
    QPoint cell = snapPlayerCell(bot);
    // 对齐到格子中心（4单位一格），否则放置会失败
    int gx = (cell.x() / Block::SIZE_UNITS) * Block::SIZE_UNITS;
    int gy = (cell.y() / Block::SIZE_UNITS) * Block::SIZE_UNITS;
    m_engine->createBombAtCell(gx, gy);
}
