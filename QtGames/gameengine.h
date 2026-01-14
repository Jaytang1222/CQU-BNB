#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <QObject>
#include <QTimer>
#include <QList>
#include <QPoint>
#include "player.h"
#include "bomb.h"
#include "block.h"
#include "gameconstants.h"

class QGraphicsScene;
class GameBotManager;

class GameEngine : public QObject
{
    Q_OBJECT

public:
    explicit GameEngine(QGraphicsScene *scene, QObject *parent = nullptr);
    ~GameEngine();
    
    void initializeGame();
    void handleKeyPress(QKeyEvent *event);
    void handleKeyRelease(QKeyEvent *event);
    void placeBomb();
    void stopGame();  // 停止移动并发出结束信号
    // 供 BotManager 调用
    bool createBombAtCell(int x, int y);
    bool isCellWalkable(int x, int y) const;
    
    Player* getPlayer() const { return m_player; }
    const QList<Block*>& blocks() const { return m_blocks; }
    const QList<Bomb*>& bombs() const { return m_bombs; }
    QGraphicsScene* scene() const { return m_scene; }
    int mapWidth() const { return m_mapWidth; }
    int mapHeight() const { return m_mapHeight; }

signals:
    void gameOver();

private slots:
    void onBombExploded(Bomb *bomb);

private:
    QGraphicsScene *m_scene;
    Player *m_player;
    QList<Bomb*> m_bombs;
    QList<Block*> m_blocks;
    class GameBotManager *m_botManager;
    
    int m_mapWidth;
    int m_mapHeight;
    int m_dirX;
    int m_dirY;
    int m_lastMoveX;
    int m_lastMoveY;
    QTimer *m_moveTimer;
    QTimer *m_botTimer;
    
    void createMap();
    bool isValidPosition(int x, int y) const;
    bool canPlaceBomb(int x, int y) const;
    void checkCollisions();
    void destroyBlocksInRange(const QList<QPoint> &explosionPositions);
    bool isPositionInList(const QPoint &pos, const QList<QPoint> &list) const;
    void tryMoveStep();
    void interruptForTurn();
};

#endif // GAMEENGINE_H
