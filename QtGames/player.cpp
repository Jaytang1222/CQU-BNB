#include "player.h"
#include <QBrush>
#include <QPen>
#include <QAbstractAnimation>

Player::Player(int x, int y, QGraphicsItem *parent)
    : QObject()
    , QGraphicsEllipseItem(0, 0, Player::SIZE_PIXELS, Player::SIZE_PIXELS, parent)
    , m_moveAnim(new QVariantAnimation(this))
    , m_canMove(true)
    , m_targetPos(QPointF(x * GameConstants::UNIT_SIZE, y * GameConstants::UNIT_SIZE))
{
    // 设置玩家外观
    setBrush(QBrush(Qt::blue));
    setPen(QPen(Qt::darkBlue, 1));
    setZValue(5); // 确保玩家在炸弹之上
    
    // 设置初始位置（x, y是逻辑单位，转换为像素）
    setPos(m_targetPos);
    
    // 设置插值动画，75ms完成一次移动
    m_moveAnim->setDuration(75);
    m_moveAnim->setEasingCurve(QEasingCurve::Linear);
    connect(m_moveAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        setPos(value.toPointF());
    });
    connect(m_moveAnim, &QVariantAnimation::finished, this, [this]() {
        // 保持目标位置，避免回弹
        setPos(m_targetPos);
        m_canMove = true;
    });
}

void Player::moveUp()
{
    if (!m_canMove) return;
    
    // 每次移动1/4格（1个逻辑单位 = 8px），细腻操作
    m_canMove = false;
    QPointF start = pos();
    const qreal stepPx = GameConstants::UNIT_SIZE;  // 1个单位 = 8像素
    QPointF end = start + QPointF(0, -stepPx);
    m_targetPos = end;
    m_moveAnim->stop();
    m_moveAnim->setStartValue(start);
    m_moveAnim->setEndValue(end);
    m_moveAnim->start();
}

void Player::moveDown()
{
    if (!m_canMove) return;
    
    m_canMove = false;
    QPointF start = pos();
    const qreal stepPx = GameConstants::UNIT_SIZE;  // 1个单位 = 8像素
    QPointF end = start + QPointF(0, stepPx);
    m_targetPos = end;
    m_moveAnim->stop();
    m_moveAnim->setStartValue(start);
    m_moveAnim->setEndValue(end);
    m_moveAnim->start();
}

void Player::moveLeft()
{
    if (!m_canMove) return;
    
    m_canMove = false;
    QPointF start = pos();
    const qreal stepPx = GameConstants::UNIT_SIZE;  // 1个单位 = 8像素
    QPointF end = start + QPointF(-stepPx, 0);
    m_targetPos = end;
    m_moveAnim->stop();
    m_moveAnim->setStartValue(start);
    m_moveAnim->setEndValue(end);
    m_moveAnim->start();
}

void Player::moveRight()
{
    if (!m_canMove) return;
    
    m_canMove = false;
    QPointF start = pos();
    const qreal stepPx = GameConstants::UNIT_SIZE;  // 1个单位 = 8像素
    QPointF end = start + QPointF(stepPx, 0);
    m_targetPos = end;
    m_moveAnim->stop();
    m_moveAnim->setStartValue(start);
    m_moveAnim->setEndValue(end);
    m_moveAnim->start();
}

QPointF Player::getPosition() const
{
    return pos();
}

void Player::setPosition(int x, int y)
{
    // x, y是逻辑单位，转换为像素
    m_targetPos = QPointF(x * GameConstants::UNIT_SIZE, y * GameConstants::UNIT_SIZE);
    setPos(m_targetPos);
}

bool Player::isMoving() const
{
    return m_moveAnim->state() == QAbstractAnimation::Running;
}

void Player::interruptMove()
{
    if (m_moveAnim->state() == QAbstractAnimation::Running) {
        m_moveAnim->stop();
    }
    // 中断后对齐到1/4格网格，避免位置残留小数导致重叠
    snapToGrid();
    m_canMove = true;
}

void Player::alignToGrid()
{
    snapToGrid();
}

bool Player::canMove() const
{
    return m_canMove;
}

void Player::setCanMove(bool canMove)
{
    m_canMove = canMove;
}

void Player::snapToGrid()
{
    QPointF p = pos();
    // 对齐到1/4格网格（1个单位 = 8像素）
    const qreal unitPx = GameConstants::UNIT_SIZE;
    qreal gx = std::round(p.x() / unitPx) * unitPx;
    qreal gy = std::round(p.y() / unitPx) * unitPx;
    m_targetPos = QPointF(gx, gy);
    setPos(m_targetPos);
}
