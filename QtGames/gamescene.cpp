#include "gamescene.h"
#include "gameengine.h"
#include "gameconstants.h"
#include <QKeyEvent>
#include <QMessageBox>

GameScene::GameScene(QObject *parent)
    : QGraphicsScene(parent)
    , m_gameEngine(nullptr)
{
    // 设置场景大小（使用像素）
    setSceneRect(0, 0, GameConstants::MAP_SIZE_PIXELS, GameConstants::MAP_SIZE_PIXELS);
    
    // 创建游戏引擎
    m_gameEngine = new GameEngine(this, this);
    
    // 连接游戏结束信号
    connect(m_gameEngine, &GameEngine::gameOver, this, [this]() {
        m_gameEngine->stopGame();
        QMessageBox::information(nullptr, tr("游戏结束"), tr("玩家被炸到了，游戏结束！"));
    });
    
    // 初始化游戏
    initializeGame();
}

GameScene::~GameScene()
{
    // GameEngine会在父对象销毁时自动清理
}

void GameScene::initializeGame()
{
    if (m_gameEngine) {
        m_gameEngine->initializeGame();
    }
}

void GameScene::keyPressEvent(QKeyEvent *event)
{
    if (m_gameEngine) {
        m_gameEngine->handleKeyPress(event);
    }
    // 不调用父类方法，因为我们自己处理了所有键盘事件
}

void GameScene::keyReleaseEvent(QKeyEvent *event)
{
    if (m_gameEngine) {
        m_gameEngine->handleKeyRelease(event);
    }
    // 不调用父类方法
}
