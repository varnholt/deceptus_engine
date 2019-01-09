#include "qtsfmlcanvas.h"

#include "debugdraw.h"

#include "player.h"
#include "level.h"


#ifdef Q_WS_X11
    #include <Qt/qx11info_x11.h>
    #include <X11/Xlib.h>
#endif


QtSfmlCanvas::QtSfmlCanvas(
   QWidget* parent,
   unsigned int FrameTime
)
 : QGLWidget(parent),
   mInitialized(false)
{
   // Setup some states to allow direct rendering into the widget
   setAttribute(Qt::WA_PaintOnScreen);
   setAttribute(Qt::WA_OpaquePaintEvent);
   setAttribute(Qt::WA_NoSystemBackground);

   // Set strong focus to enable keyboard events to be received
   setFocusPolicy(Qt::StrongFocus);

   int mode = 3;

   if (mode == 1)
   {
      mWindowWidth = 1066;
      mWindowHeight = 600;
      mViewWidth = 533;
      mViewHeight = 300;
   }
   else if (mode == 2)
   {
      mWindowWidth = 1599;
      mWindowHeight = 900;
      mViewWidth = 533;
      mViewHeight = 300;
   }
   else if (mode == 3)
   {
      mWindowWidth = 533;
      mWindowHeight = 300;
      mViewWidth = 533;
      mViewHeight = 300;
   }

   for (int i = 0; i < 10; i++)
   {
      mVisibleLayers[i] = true;
   }

   // Setup the widget geometry
   //   move(Position);
   resize(mWindowWidth, mWindowHeight);

   // Setup the timer
   myTimer.setInterval(FrameTime);
}


QtSfmlCanvas::~QtSfmlCanvas()
{

}


void QtSfmlCanvas::showEvent(QShowEvent*)
{
   if (!mInitialized)
   {
      // Under X11, we need to flush the commands sent to the server to ensure that
      // SFML will get an updated view of the windows
      #ifdef Q_WS_X11
      XFlush(QX11Info::display());
      #endif

      // Create the SFML window with the widget handle
      sf::RenderWindow::create((sf::WindowHandle) winId());

      // Let the derived class do its specific stuff
      OnInit();

      // Setup the timer to trigger a refresh at specified framerate
      connect(&myTimer, SIGNAL(timeout()), this, SLOT(repaint()));
      myTimer.start();

      mInitialized = true;
   }
}


QPaintEngine* QtSfmlCanvas::paintEngine() const
{
   return 0;
}


void QtSfmlCanvas::paintEvent(QPaintEvent*)
{
   // Let the derived class do its specific stuff
   OnUpdate();

   // Display on screen
   sf::RenderWindow::display();
}


void QtSfmlCanvas::resizeEvent(QResizeEvent* event)
{
   setSize(sf::Vector2u(QWidget::width(), QWidget::height()));
}


void QtSfmlCanvas::OnInit()
{
   // init camera ('view')
   mView = new sf::View();
   mView->reset(sf::FloatRect(0, 0, mViewWidth, mViewHeight));
   mView->setViewport(sf::FloatRect(0, 0, 1.0f, 1.0f));

   setView(*mView);
   setVerticalSyncEnabled(true);
   setFramerateLimit(60);
   setKeyRepeatEnabled(false);

   // helpers
   mDebugDraw = new DebugDraw(this);
}


void QtSfmlCanvas::OnUpdate()
{
   updateView();
   clear(sf::Color::Black);
   pushGLStates();

   Level::getCurrentLevel()->setVisibleLayers(mVisibleLayers);

   if (mDebugDrawEnabled)
   {
      debugBodies();
   }

   for (int z = 0; z < 50; z++)
   {
      Level::getCurrentLevel()->draw(this, z);
      // mLevel->draw(mWindow, 1);
   }

   popGLStates();
   display();
}


void QtSfmlCanvas::updateView()
{
   int levelWidth  = Level::getCurrentLevel()->getSize().x;
   int levelHeight = Level::getCurrentLevel()->getSize().y;

   int viewX = Player::getPlayer(0)->getPosition().x - mViewWidth / 2;
   int viewY = Player::getPlayer(0)->getPosition().y - mViewHeight / 2;

   // printf("view: %d, %d\n", viewX, viewY);

   if (viewX < 0)
      viewX = 0;
   if (viewY < 0)
      viewY = 0;

   if (viewX > levelWidth - mViewWidth)
      viewX = levelWidth - mViewWidth;
   if (viewY > levelHeight - mViewHeight)
      viewY = levelHeight - mViewHeight;


   mView->reset(
      sf::FloatRect(
         viewX,
         viewY,
         mViewWidth,
         mViewHeight
      )
   );

   setView(*mView);
}



void QtSfmlCanvas::debugBodies()
{
   for (
      b2Body* body = Level::getCurrentLevel()->getWorld()->GetBodyList();
      body != 0;
      body = body->GetNext()
   )
   {
      // dynamic stuff
      if (body->GetType() == b2_dynamicBody || body->GetType() == b2_kinematicBody)
      {
         b2Fixture* f = body->GetFixtureList();
         while (f)
         {
            b2Fixture* next = f->GetNext();
            b2Shape *shape = f->GetShape();

            if (shape->GetType() == b2Shape::e_polygon)
            {
               b2PolygonShape* poly = (b2PolygonShape*)shape;

               int vertexCount = poly->GetVertexCount();
               b2Vec2* vertices = new b2Vec2[vertexCount];

               for( int i = 0; i < vertexCount; i++ )
               {
                  b2Vec2 vec2 = poly->GetVertex(i);
                  vertices[i] = vec2;
                  vertices[i].x += body->GetPosition().x;
                  vertices[i].y += body->GetPosition().y;
               }

               mDebugDraw->DrawPolygon(
                  vertices,
                  vertexCount,
                  b2Color(1,0,0,1)
               );

               delete[] vertices;
            }
            else if (shape->GetType() == b2Shape::e_circle)
            {
               b2Vec2 offset;
               b2CircleShape* circleShape = nullptr;
               circleShape = dynamic_cast<b2CircleShape*>(f->GetShape());
               if (circleShape != nullptr)
               {
                  offset = circleShape->m_p;
               }

               mDebugDraw->DrawCircle(
                  body->GetPosition() + offset,
                  shape->m_radius,
                  b2Color(0.4f, 0.4f, 0.4f, 1.0f)
               );
            }

            f = next;
         }
      }
      else
      {
         mDebugDraw->DrawPolygon(
            Level::getCurrentLevel()->getPointMap()->value(body),
            Level::getCurrentLevel()->getPointSizeMap()->value(body),
            b2Color(1,0,0)
         );
      }
   }
}
