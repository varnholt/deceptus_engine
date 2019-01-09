#pragma once

#include <QGLWidget>
#include <QTimer>

// sfml
#include <SFML/Graphics.hpp>

class DebugDraw;

class QtSfmlCanvas : public QGLWidget, public sf::RenderWindow
{
   Q_OBJECT

public :

    QtSfmlCanvas(QWidget* Parent, unsigned int FrameTime = 0);

    virtual ~QtSfmlCanvas();

protected:

    void resizeEvent(QResizeEvent *event);

    virtual void OnInit();

    virtual void OnUpdate();

    virtual QPaintEngine* paintEngine() const;

    virtual void showEvent(QShowEvent*);

    virtual void paintEvent(QPaintEvent*);

   void debugBodies();
   QTimer myTimer;
   bool   mInitialized;
   void updateView();

   sf::View* mView = nullptr;

   bool mVisibleLayers[10];

   int mWindowWidth = 0;
   int mWindowHeight = 0;
   int mViewWidth = 0;
   int mViewHeight = 0;

   bool mDebugDrawEnabled = false;
   DebugDraw* mDebugDraw;
};


