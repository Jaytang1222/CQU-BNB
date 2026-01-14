#ifndef PLAYER_H
#define PLAYER_H

#include <QGraphicsEllipseItem>
#include <QObject>
#include <QKeyEvent>
#include <QVariantAnimation>
#include <QTimer>
#include "gameconstants.h"

class Player : public QObject, public QGraphicsEllipseItem
{
    Q_OBJECT

public:
    Player(int x, int y, QGraphicsItem *parent = nullptr);
    
    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    
    QPointF getPosition() const;
    void setPosition(int x, int y);  // 参数是逻辑单位
    
    bool canMove() const;
    void setCanMove(bool canMove);
    bool isMoving() const;
    void interruptMove();  // 中断当前移动动画，立即可拐弯
    void alignToGrid();    // 对齐到网格中心，消除残留偏差
    
    // 玩家大小为4个单位x4个单位，显示时转换为像素
    enum : int {
        SIZE_UNITS = GameConstants::BLOCK_SIZE,              // 4个单位
        SIZE_PIXELS = SIZE_UNITS * GameConstants::UNIT_SIZE  // 32像素
    };

private:
    QVariantAnimation *m_moveAnim;
    bool m_canMove;
    QPointF m_targetPos;
    void snapToGrid();
};

#endif // PLAYER_H
