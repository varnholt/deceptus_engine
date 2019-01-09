#ifndef GAME_H
#define GAME_H

// sfml
#include <SFML/Graphics.hpp>



// box2d
#include <Box2D\Box2D.h>

class DebugDraw;
class BulletLevel;
class Parallax;
class BulletPlayer;


class BulletGame
{

   public:

      BulletGame();

      static BulletGame* getInstance();

      void initialize();

      void draw();

      void gameLoop();


protected:


      void eventLoop(sf::Event *event);



      void updateView();

      void debugBodies();

      void keyboardFuckup();

      // void renderingThread(sf::Window *window);


      // #ifdef USE_GL
      // sf::Window* mWindow;
      // #else
      sf::RenderWindow* mWindow;
      // #endif

      sf::View* mView;

      BulletLevel* mLevel;
      sf::Clock mDeltaClock;
      BulletPlayer* mPlayer;
      Parallax* mParallax;

      DebugDraw* mDebugDraw;
      std::map<b2Body*, b2Vec2*> mPointMap;
      std::map<b2Body*, int> mPointSizeMap;

      int mHeight;
      int mWidth;

      static BulletGame* sInstance;
};

#endif // GAME_H
