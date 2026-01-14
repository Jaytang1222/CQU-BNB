#include "gameengine.h"
#include "gameconstants.h"
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QRandomGenerator>
#include <QDebug>
#include <cmath>
#include "gamebotmanager.h"

GameEngine::GameEngine(QGraphicsScene *scene, QObject *parent)
    : QObject(parent)
    , m_scene(scene)
    , m_player(nullptr)
    , m_mapWidth(GameConstants::MAP_SIZE_UNITS)  // 100个单位（25个方格 * 4个单位）
    , m_mapHeight(GameConstants::MAP_SIZE_UNITS)  // 100个单位
    , m_dirX(0)
    , m_dirY(0)
    , m_lastMoveX(0)
    , m_lastMoveY(0)
    , m_moveTimer(new QTimer(this))
    , m_botTimer(new QTimer(this))
    , m_botManager(new GameBotManager(this))
{
    m_moveTimer->setInterval(10); // 10ms 检查一次，衔接动画完成后继续移动
    connect(m_moveTimer, &QTimer::timeout, this, &GameEngine::tryMoveStep);
    m_moveTimer->start();
    
    // Bot AI 定时器
    m_botTimer->setInterval(150); // 约每150ms做一次决策
    connect(m_botTimer, &QTimer::timeout, this, [this]() {
        if (m_botManager) m_botManager->updateBots();
    });
    m_botTimer->start();
}

GameEngine::~GameEngine()
{
    // 清理资源
    if (m_botManager) {
        m_botManager->clearBots();
    }
    for (Bomb *bomb : m_bombs) {
        delete bomb;
    }
    for (Block *block : m_blocks) {
        delete block;
    }
    if (m_player) {
        delete m_player;
    }
}

void GameEngine::initializeGame()
{
    if (!m_scene) return;
    
    // 清空场景
    m_scene->clear();
    m_bombs.clear();
    m_blocks.clear();
    if (m_botManager) {
        m_botManager->clearBots();
    }
    
    // 创建地图
    createMap();
    
    // 创建玩家（放在左上角安全位置，使用逻辑单位，避开内侧墙）
    m_player = new Player(Block::SIZE_UNITS, Block::SIZE_UNITS);
    m_scene->addItem(m_player);
    
    // 创建AI机器人，出生在其余三个角
    if (m_botManager) {
        m_botManager->spawnBots();
    }
}

void GameEngine::createMap()
{
    auto inCornerSafe = [&](int x, int y) {
        // 预留四角 2x2 内部空区（不含边界墙），边界占1格，因此取3格跨度
        int span = Block::SIZE_UNITS * 3; // 1格边界 + 2格空区
        bool left   = x < span;
        bool right  = x >= m_mapWidth  - span;
        bool top    = y < span;
        bool bottom = y >= m_mapHeight - span;
        return (left && top) || (right && top) || (left && bottom) || (right && bottom);
    };

    // 创建边界墙（使用逻辑单位）
    for (int x = 0; x < m_mapWidth; x += Block::SIZE_UNITS) {
        // 上边界
        Block *wall1 = new Block(x, 0, Block::WALL);
        m_scene->addItem(wall1);
        m_blocks.append(wall1);
        
        // 下边界
        Block *wall2 = new Block(x, m_mapHeight - Block::SIZE_UNITS, Block::WALL);
        m_scene->addItem(wall2);
        m_blocks.append(wall2);
    }
    
    for (int y = Block::SIZE_UNITS; y < m_mapHeight - Block::SIZE_UNITS; y += Block::SIZE_UNITS) {
        // 左边界
        Block *wall1 = new Block(0, y, Block::WALL);
        m_scene->addItem(wall1);
        m_blocks.append(wall1);
        
        // 右边界
        Block *wall2 = new Block(m_mapWidth - Block::SIZE_UNITS, y, Block::WALL);
        m_scene->addItem(wall2);
        m_blocks.append(wall2);
    }
    
    // 创建内部网格墙（每隔一个位置放一个不可破坏的墙）
    for (int x = Block::SIZE_UNITS * 2; x < m_mapWidth - Block::SIZE_UNITS; x += Block::SIZE_UNITS * 2) {
        for (int y = Block::SIZE_UNITS * 2; y < m_mapHeight - Block::SIZE_UNITS; y += Block::SIZE_UNITS * 2) {
            // 避开四个角的 3x3 空区
            if (inCornerSafe(x, y)) {
                continue;
            }
            Block *wall = new Block(x, y, Block::WALL);
            m_scene->addItem(wall);
            m_blocks.append(wall);
        }
    }
    
    // 随机放置可破坏的砖块（避开玩家起始位置）
    QRandomGenerator *rng = QRandomGenerator::global();
    for (int x = Block::SIZE_UNITS; x < m_mapWidth - Block::SIZE_UNITS; x += Block::SIZE_UNITS) {
        for (int y = Block::SIZE_UNITS; y < m_mapHeight - Block::SIZE_UNITS; y += Block::SIZE_UNITS) {
            // 跳过网格墙的位置和玩家起始位置
            if ((x % (Block::SIZE_UNITS * 2) == 0 && y % (Block::SIZE_UNITS * 2) == 0) ||
                inCornerSafe(x, y)) {
                continue;
            }
            
            // 随机放置砖块（70%概率）
            if (rng->bounded(100) < 70) {
                Block *brick = new Block(x, y, Block::BRICK);
                m_scene->addItem(brick);
                m_blocks.append(brick);
            }
        }
    }
}

void GameEngine::handleKeyPress(QKeyEvent *event)
{
    if (!m_player) return;
    
    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_W:
        m_dirX = 0; m_dirY = -1;
        interruptForTurn();
        break;
    case Qt::Key_Down:
    case Qt::Key_S:
        m_dirX = 0; m_dirY = 1;
        interruptForTurn();
        break;
    case Qt::Key_Left:
    case Qt::Key_A:
        m_dirX = -1; m_dirY = 0;
        interruptForTurn();
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        m_dirX = 1; m_dirY = 0;
        interruptForTurn();
        break;
    case Qt::Key_Space:
        placeBomb();
        break;
    }
}

void GameEngine::handleKeyRelease(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_W:
        if (m_dirY == -1) m_dirY = 0;
        break;
    case Qt::Key_Down:
    case Qt::Key_S:
        if (m_dirY == 1) m_dirY = 0;
        break;
    case Qt::Key_Left:
    case Qt::Key_A:
        if (m_dirX == -1) m_dirX = 0;
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        if (m_dirX == 1) m_dirX = 0;
        break;
    default:
        break;
    }
}

void GameEngine::placeBomb()
{
    if (!m_player) return;
    
    // 计算玩家覆盖最多的方格（使用像素计算重叠面积）
    QRectF playerRect(m_player->x(), m_player->y(), Player::SIZE_PIXELS, Player::SIZE_PIXELS);
    const int cellSizePx = Block::SIZE_PIXELS; // 32 像素

    int minCellX = static_cast<int>(std::floor(playerRect.left() / cellSizePx));
    int maxCellX = static_cast<int>(std::floor((playerRect.right() - 1) / cellSizePx));
    int minCellY = static_cast<int>(std::floor(playerRect.top() / cellSizePx));
    int maxCellY = static_cast<int>(std::floor((playerRect.bottom() - 1) / cellSizePx));

    int bestCellX = minCellX;
    int bestCellY = minCellY;
    qreal bestArea = -1.0;

    for (int cx = minCellX; cx <= maxCellX; ++cx) {
        for (int cy = minCellY; cy <= maxCellY; ++cy) {
            QRectF cellRect(cx * cellSizePx, cy * cellSizePx, cellSizePx, cellSizePx);
            QRectF inter = playerRect.intersected(cellRect);
            qreal area = inter.width() * inter.height();
            if (area > bestArea) {
                bestArea = area;
                bestCellX = cx;
                bestCellY = cy;
            }
        }
    }

    // 转回逻辑单位（一个网格是 Block::SIZE_UNITS 个逻辑单位）
    int alignedX = bestCellX * Block::SIZE_UNITS;
    int alignedY = bestCellY * Block::SIZE_UNITS;
    
    // 检查是否可以在此位置放置炸弹
    if (!canPlaceBomb(alignedX, alignedY)) {
        return;
    }
    
    // 创建炸弹（初始范围上下左右各1，使用逻辑单位）
    Bomb *bomb = new Bomb(alignedX, alignedY, 1);
    m_scene->addItem(bomb);
    m_bombs.append(bomb);
    
    connect(bomb, &Bomb::explosionFinished, this, &GameEngine::onBombExploded);
    
    // 延迟爆炸（2秒后）
    QTimer::singleShot(2000, bomb, &Bomb::explode);
}

bool GameEngine::isValidPosition(int x, int y) const
{
    // x, y是逻辑单位
    // 检查边界
    if (x < 0 || y < 0 || x + Player::SIZE_UNITS > m_mapWidth || y + Player::SIZE_UNITS > m_mapHeight) {
        return false;
    }
    
    // 转换为像素进行比较
    QRectF playerRect(x * GameConstants::UNIT_SIZE, y * GameConstants::UNIT_SIZE, 
                      Player::SIZE_PIXELS, Player::SIZE_PIXELS);
    
    // 检查是否与方块碰撞
    for (Block *block : m_blocks) {
        QRectF blockRect(block->x(), block->y(), Block::SIZE_PIXELS, Block::SIZE_PIXELS);
        if (playerRect.intersects(blockRect)) {
            return false;
        }
    }
    
    // 检查是否与炸弹碰撞
    QRectF currentPlayerRect;
    if (m_player) {
        currentPlayerRect = QRectF(m_player->x(), m_player->y(), Player::SIZE_PIXELS, Player::SIZE_PIXELS);
    }
    for (Bomb *bomb : m_bombs) {
        if (!bomb->isExploding()) {
            QRectF bombRect(bomb->x(), bomb->y(), Bomb::SIZE_PIXELS, Bomb::SIZE_PIXELS);
            // 允许玩家从自己脚下的炸弹上离开
            if (currentPlayerRect.isValid() && currentPlayerRect.intersects(bombRect)) {
                continue;
            }
            if (playerRect.intersects(bombRect)) {
                return false;
            }
        }
    }
    
    return true;
}

bool GameEngine::isCellWalkable(int x, int y) const
{
    // 边界（逻辑单位）
    if (x < 0 || y < 0 || x + Player::SIZE_UNITS > m_mapWidth || y + Player::SIZE_UNITS > m_mapHeight) {
        return false;
    }
    QRectF rect(x * GameConstants::UNIT_SIZE, y * GameConstants::UNIT_SIZE, Player::SIZE_PIXELS, Player::SIZE_PIXELS);
    for (Block *b : m_blocks) {
        QRectF r(b->x(), b->y(), Block::SIZE_PIXELS, Block::SIZE_PIXELS);
        if (rect.intersects(r)) return false;
    }
    for (Bomb *bomb : m_bombs) {
        if (!bomb->isExploding()) {
            QRectF r(bomb->x(), bomb->y(), Bomb::SIZE_PIXELS, Bomb::SIZE_PIXELS);
            if (rect.intersects(r)) return false;
        }
    }
    return true;
}

void GameEngine::tryMoveStep()
{
    if (!m_player) return;
    if (!m_player->canMove()) return;
    if (m_dirX == 0 && m_dirY == 0) return;

    int currentX = static_cast<int>(std::round(m_player->x() / GameConstants::UNIT_SIZE));
    int currentY = static_cast<int>(std::round(m_player->y() / GameConstants::UNIT_SIZE));
    // 每次移动1个逻辑单位（1/4格 = 8像素）
    int stepUnits = GameConstants::LOGIC_UNIT;
    int newX = currentX + m_dirX * stepUnits;
    int newY = currentY + m_dirY * stepUnits;

    if (!isValidPosition(newX, newY)) return;

    if (m_dirX == 1) m_player->moveRight();
    else if (m_dirX == -1) m_player->moveLeft();
    else if (m_dirY == 1) m_player->moveDown();
    else if (m_dirY == -1) m_player->moveUp();

    // 记录本次移动方向，用于判断拐弯时是否需要打断
    m_lastMoveX = m_dirX;
    m_lastMoveY = m_dirY;
}

void GameEngine::interruptForTurn()
{
    if (!m_player) return;

    // 如果正在动画中，提前终止当前步，并允许立即拐弯
    if (!m_player->canMove() && m_player->isMoving()) {
        // 只有方向变化时才打断，防止同向长按被反复打断造成回弹
        if (m_dirX != m_lastMoveX || m_dirY != m_lastMoveY) {
        m_player->interruptMove();
    }
    }

    // 方向已更新，尝试立即迈出下一步
    tryMoveStep();
}

bool GameEngine::canPlaceBomb(int x, int y) const
{
    // x, y是逻辑单位，需要转换为像素进行比较
    int bombX_pixels = x * GameConstants::UNIT_SIZE;
    int bombY_pixels = y * GameConstants::UNIT_SIZE;
    
    // 检查该位置是否已有炸弹
    for (Bomb *bomb : m_bombs) {
        if (!bomb->isExploding()) {
            int bombX = static_cast<int>(bomb->x());
            int bombY = static_cast<int>(bomb->y());
            if (bombX == bombX_pixels && bombY == bombY_pixels) {
                return false;
            }
        }
    }
    return true;
}

void GameEngine::onBombExploded(Bomb *bomb)
{
    if (!bomb) return;
    
    // 获取爆炸范围
    QList<QPoint> explosionPositions = bomb->getExplosionPositions();
    
    // 检查玩家是否在爆炸范围内（检查玩家矩形是否与爆炸位置重叠）
    // explosionPositions是逻辑单位，需要转换为像素
    if (m_player) {
        QRectF playerRect(m_player->x(), m_player->y(), Player::SIZE_PIXELS, Player::SIZE_PIXELS);
        for (const QPoint &expPos : explosionPositions) {
            // expPos是逻辑单位，转换为像素
            QRectF expRect(expPos.x() * GameConstants::UNIT_SIZE, 
                          expPos.y() * GameConstants::UNIT_SIZE, 
                          Block::SIZE_PIXELS, Block::SIZE_PIXELS);
            if (playerRect.intersects(expRect)) {
                emit gameOver();
                // 移除炸弹
                m_bombs.removeAll(bomb);
                if (m_scene) {
                    m_scene->removeItem(bomb);
                }
                bomb->deleteLater();
                return;
            }
        }
    }
    
    // 检查AI机器人是否在爆炸范围内
    if (m_botManager) {
        // 获取当前所有AI机器人
        QList<Player*> bots = m_botManager->getBots(); // 假设GameBotManager有这个方法
        QList<Player*> botsToRemove;
        
        for (Player *bot : bots) {
            if (!bot) continue;
            
            QRectF botRect(bot->x(), bot->y(), Player::SIZE_PIXELS, Player::SIZE_PIXELS);
            for (const QPoint &expPos : explosionPositions) {
                QRectF expRect(expPos.x() * GameConstants::UNIT_SIZE,
                              expPos.y() * GameConstants::UNIT_SIZE,
                              Block::SIZE_PIXELS, Block::SIZE_PIXELS);
                if (botRect.intersects(expRect)) {
                    // 机器人被炸到，标记为移除
                    botsToRemove.append(bot);
                    break; // 一旦发现机器人在爆炸范围内，跳出内循环
                }
            }
        }
        
        // 从场景中移除被炸死的机器人
        for (Player *bot : botsToRemove) {
            if (m_scene) {
                m_scene->removeItem(bot);
            }
            // 从bot管理器中移除机器人
            m_botManager->removeBot(bot);
            bot->deleteLater();
        }
    }
    
    // 销毁范围内的方块
    destroyBlocksInRange(explosionPositions);
    
    // 移除炸弹
    m_bombs.removeAll(bomb);
    if (m_scene) {
        m_scene->removeItem(bomb);
    }
    bomb->deleteLater();
}

void GameEngine::destroyBlocksInRange(const QList<QPoint> &explosionPositions)
{
    QList<Block*> toRemove;
    
    for (Block *block : m_blocks) {
        if (!block->isDestructible()) {
            continue;  // 跳过不可破坏的墙
        }
        
        // 使用像素矩形做交集判断，避免精度和对齐误差
        QRectF blockRect(block->x(), block->y(), Block::SIZE_PIXELS, Block::SIZE_PIXELS);
        for (const QPoint &expPos : explosionPositions) {
            QRectF expRect(expPos.x() * GameConstants::UNIT_SIZE,
                          expPos.y() * GameConstants::UNIT_SIZE,
                          Block::SIZE_PIXELS, Block::SIZE_PIXELS);
            if (blockRect.intersects(expRect)) {
                toRemove.append(block);
                break;  // 找到匹配就跳出内层循环
            }
        }
    }
    
    for (Block *block : toRemove) {
        m_blocks.removeAll(block);
        if (m_scene) {
            m_scene->removeItem(block);
        }
        delete block;
    }
}

bool GameEngine::isPositionInList(const QPoint &pos, const QList<QPoint> &list) const
{
    for (const QPoint &p : list) {
        if (p.x() == pos.x() && p.y() == pos.y()) {
            return true;
        }
    }
    return false;
}

void GameEngine::stopGame()
{
    m_dirX = 0;
    m_dirY = 0;
    if (m_moveTimer) {
        m_moveTimer->stop();
    }
    if (m_botTimer) {
        m_botTimer->stop();
    }
    if (m_player) {
        m_player->interruptMove();
        m_player->setCanMove(false);
    }
    if (m_botManager) {
        m_botManager->clearBots();
    }
}

bool GameEngine::createBombAtCell(int x, int y)
{
    if (!canPlaceBomb(x, y)) return false;
    Bomb *bomb = new Bomb(x, y, 1);
    m_scene->addItem(bomb);
    m_bombs.append(bomb);
    connect(bomb, &Bomb::explosionFinished, this, &GameEngine::onBombExploded);
    QTimer::singleShot(2000, bomb, &Bomb::explode);
    return true;
}
