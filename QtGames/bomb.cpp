#include "bomb.h"
#include <QBrush>
#include <QPen>
#include <QGraphicsScene>
#include <QObject>

Bomb::Bomb(int x, int y, int range, QGraphicsItem *parent)
    : QObject()
    , QGraphicsEllipseItem(0, 0, Bomb::SIZE_PIXELS, Bomb::SIZE_PIXELS, parent)
    , m_range(range)
    , m_isExploding(false)
{
    // x, y是逻辑单位，转换为像素
    setPos(x * GameConstants::UNIT_SIZE, y * GameConstants::UNIT_SIZE);
    
    // 设置炸弹外观 - 黑色圆形
    setBrush(QBrush(Qt::black));
    setPen(QPen(Qt::darkGray, 1));
    
    // 创建爆炸定时器
    m_explosionTimer = new QTimer(this);
    m_explosionTimer->setSingleShot(true);
    m_explosionTimer->setInterval(EXPLOSION_DURATION);
    
    connect(m_explosionTimer, &QTimer::timeout, this, &Bomb::onExplosionTimeout);
}

void Bomb::explode()
{
    if (m_isExploding) return;
    
    m_isExploding = true;
    createExplosionEffects();
    m_explosionTimer->start();
}

QPointF Bomb::getBombPosition() const
{
    return pos();
}

QList<QPoint> Bomb::getExplosionPositions() const
{
    QList<QPoint> positions;
    // 将像素位置转换回逻辑单位
    int bombX = static_cast<int>(x()) / GameConstants::UNIT_SIZE;
    int bombY = static_cast<int>(y()) / GameConstants::UNIT_SIZE;
    
    // 中心位置（逻辑单位）
    positions.append(QPoint(bombX, bombY));
    
    // 上下左右各m_range个方块（每个方块大小为SIZE_UNITS个单位）
    for (int i = 1; i <= m_range; ++i) {
        positions.append(QPoint(bombX, bombY - i * SIZE_UNITS));  // 上
        positions.append(QPoint(bombX, bombY + i * SIZE_UNITS));  // 下
        positions.append(QPoint(bombX - i * SIZE_UNITS, bombY));  // 左
        positions.append(QPoint(bombX + i * SIZE_UNITS, bombY));  // 右
    }
    
    return positions;
}

void Bomb::createExplosionEffects()
{
    QList<QPoint> positions = getExplosionPositions();
    
    for (const QPoint &pos : positions) {
        // pos是逻辑单位，转换为像素显示
        QGraphicsRectItem *effect = new QGraphicsRectItem(0, 0, SIZE_PIXELS, SIZE_PIXELS);
        effect->setPos(pos.x() * GameConstants::UNIT_SIZE, pos.y() * GameConstants::UNIT_SIZE);
        
        // 爆炸特效 - 红色半透明
        QBrush brush(QColor(255, 0, 0, 150));
        effect->setBrush(brush);
        effect->setPen(QPen(Qt::red, 2));
        
        m_explosionEffects.append(effect);
        
        if (scene()) {
            scene()->addItem(effect);
        }
    }
}

void Bomb::removeExplosionEffects()
{
    for (QGraphicsRectItem *effect : m_explosionEffects) {
        if (scene()) {
            scene()->removeItem(effect);
        }
        delete effect;
    }
    m_explosionEffects.clear();
}

void Bomb::onExplosionTimeout()
{
    removeExplosionEffects();
    emit explosionFinished(this);
}
