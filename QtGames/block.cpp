#include "block.h"
#include <QBrush>
#include <QPen>

Block::Block(int x, int y, BlockType type, QGraphicsItem *parent)
    : QGraphicsRectItem(0, 0, Block::SIZE_PIXELS, Block::SIZE_PIXELS, parent)
    , m_type(type)
{
    // x, y是逻辑单位，转换为像素
    setPos(x * GameConstants::UNIT_SIZE, y * GameConstants::UNIT_SIZE);
    
    if (type == WALL) {
        // 不可破坏的墙 - 深灰色
        setBrush(QBrush(Qt::darkGray));
        setPen(QPen(Qt::black, 1));
    } else {
        // 可破坏的砖块 - 棕色
        setBrush(QBrush(QColor(139, 69, 19)));
        setPen(QPen(Qt::darkRed, 1));
    }
    
    // 设置方块不可移动
    setFlag(QGraphicsItem::ItemIsMovable, false);
}
