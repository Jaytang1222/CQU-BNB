#ifndef BLOCK_H
#define BLOCK_H

#include <QGraphicsRectItem>
#include "gameconstants.h"

class Block : public QGraphicsRectItem
{
public:
    enum BlockType {
        WALL,      // 不可破坏的墙
        BRICK      // 可破坏的砖块
    };
    
    Block(int x, int y, BlockType type, QGraphicsItem *parent = nullptr);
    
    BlockType getType() const { return m_type; }
    bool isDestructible() const { return m_type == BRICK; }
    
    // 方块大小为4个单位x4个单位，显示时转换为像素
    enum : int {
        SIZE_UNITS = GameConstants::BLOCK_SIZE,              // 4个单位
        SIZE_PIXELS = SIZE_UNITS * GameConstants::UNIT_SIZE  // 32像素
    };

private:
    BlockType m_type;
};

#endif // BLOCK_H
