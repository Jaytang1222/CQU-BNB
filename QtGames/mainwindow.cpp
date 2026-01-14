#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "gamescene.h"
#include "gameconstants.h"
#include <QVBoxLayout>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_gameScene(nullptr)
    , m_graphicsView(nullptr)
{
    ui->setupUi(this);
    
    // 设置窗口标题
    setWindowTitle("Bomberman Game");
    
    // 创建游戏场景
    m_gameScene = new GameScene(this);
    
    // 创建图形视图
    m_graphicsView = new QGraphicsView(m_gameScene, this);
    m_graphicsView->setRenderHint(QPainter::Antialiasing);
    m_graphicsView->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    m_graphicsView->setBackgroundBrush(QBrush(Qt::lightGray));
    m_graphicsView->setCacheMode(QGraphicsView::CacheBackground);
    
    // 设置视图大小（稍微大一点以容纳边框）
    m_graphicsView->setFixedSize(GameConstants::MAP_SIZE_PIXELS + 40, GameConstants::MAP_SIZE_PIXELS + 60);
    
    // 设置中央部件
    setCentralWidget(m_graphicsView);
    
    // 确保窗口可以接收键盘事件
    setFocusPolicy(Qt::StrongFocus);
    
    // 设置图形视图可以接收焦点
    m_graphicsView->setFocusPolicy(Qt::StrongFocus);
    m_graphicsView->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (m_gameScene) {
        m_gameScene->keyPressEvent(event);
    }
    QMainWindow::keyPressEvent(event);
}
