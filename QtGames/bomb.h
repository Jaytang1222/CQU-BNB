#ifndef BOMB_H
#define BOMB_H

#include <QGraphicsEllipseItem>
#include <QObject>
#include <QTimer>
#include <QList>
#include "gameconstants.h"

class Bomb : public QObject, public QGraphicsEllipseItem
{
    Q_OBJECT

public:
    Bomb(int x, int y, int range, QGraphicsItem *parent = nullptr);  // x, y是逻辑单位
    
    void explode();
    bool isExploding() const { return m_isExploding; }
    int getRange() const { return m_range; }
    QPointF getBombPosition() const;
    
    // 获取爆炸范围的所有位置（返回逻辑单位）
    QList<QPoint> getExplosionPositions() const;
    
    // 炸弹大小为4个单位x4个单位，显示时转换为像素
    enum : int {
        SIZE_UNITS = GameConstants::BLOCK_SIZE,              // 4个单位
        SIZE_PIXELS = SIZE_UNITS * GameConstants::UNIT_SIZE  // 32像素
    };

signals:
    void explosionFinished(Bomb *bomb);

private slots:
    void onExplosionTimeout();

private:
    int m_range;
    bool m_isExploding;
    QTimer *m_explosionTimer;
    QList<QGraphicsRectItem*> m_explosionEffects;
    
    void createExplosionEffects();
    void removeExplosionEffects();
    
    static const int EXPLOSION_DURATION = 400;  // 爆炸持续400ms
};

#endif // BOMB_H
