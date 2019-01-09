#include "bulletgame.h"


// std
#include <iostream>
#include <stdio.h>
#include <string.h>

// game
#include "game/bullethitanimation.h"
#include "game/bumpmap.h"
#include "game/constants.h"
#include "game/demolevel.h"
#include "game/debugdraw.h"
#include "game/parallax.h"
#include "game/bulletplayer.h"
#include "game/weapon.h"

// framework
#include "image/psd.h"

// spine
#include <spine/spine-sfml.h>

// sfml
#include <SFML/System/Thread.hpp>

// opengl
#include <SFML/OpenGL.hpp>
#include <gl/GLU.h>


BulletGame* BulletGame::sInstance = 0;

// big images:
// http://en.sfml-dev.org/forums/index.php?topic=2508.0
// http://www.flipcode.com/archives/Efficient_Polygon_Triangulation.shtml
// http://www.iforce2d.net/b2dtut/jumping
// http://www.iforce2d.net/b2dtut/constant-speed
// http://www.iforce2d.net/b2dtut/jumpability
// http://www.sfml-dev.org/tutorials/2.0/graphics-view->php

// shaders
// http://www.sfml-dev.org/tutorials/2.0/graphics-shader.php
// render to texture
// http://www.sfml-dev.org/tutorials/2.0/graphics-draw.php
// http://www.sfml-dev.org/documentation/2.0/classsf_1_1RenderTexture.php

// parallax
// http://blog.notnowlewis.com/blog/2013/12/18/parallax-background-glsl-shaders/



BulletGame::BulletGame()
 : mWindow(0),
   mView(0),
   mPlayer(0),
   mParallax(0),
   mWidth(0),
   mHeight(0)
{
   sInstance = this;

   /*
   SkeletonData_dispose(skeletonData);
   Atlas_dispose(atlas);
   */
}


BulletGame *BulletGame::getInstance()
{
   return sInstance;
}



void BulletGame::updateView()
{
   int levelWidth = mLevel->getSize().x;
   int levelHeight = mLevel->getSize().y;

   int viewX = mPlayer->getPosition().x - mWidth / 2;
   int viewY = mPlayer->getPosition().y - mHeight / 2;

   // printf("view: %d, %d\n", viewX, viewY);

   if (viewX < 0)
      viewX = 0;
   if (viewY < 0)
      viewY = 0;

   if (viewX > levelWidth - mWidth)
      viewX = levelWidth - mWidth;
   if (viewY > levelHeight - mHeight)
      viewY = levelHeight - mHeight;


   mView->reset(
      sf::FloatRect(
         viewX,
         viewY,
         mWidth,
         mHeight
      )
   );

   // glMatrixMode(GL_PROJECTION);
   // glLoadMatrixf(mView->getTransform().getMatrix());
   mWindow->setView(*mView);
}


void renderingThread(sf::RenderWindow* window)
{
    // the rendering loop
    while (window->isOpen())
    {
        BulletGame::getInstance()->draw();

        // end the current frame
        window->display();
    }
}


void BulletGame::draw()
{
   updateView();

   mWindow->clear();

   mWindow->pushGLStates();

   mParallax->setPosition(
      mPlayer->getPosition().x / (float)mLevel->getSize().x,
      mPlayer->getPosition().y / (float)mLevel->getSize().y
   );

   mParallax->render(*mWindow, mWidth, mHeight);

   mLevel->draw(*mWindow);

   mWindow->draw(*mPlayer->getSpineDrawable());

   std::list<BulletHitAnimation *>* bulletHits = BulletHitAnimation::getAnimations();
   std::list<BulletHitAnimation *>::iterator it;
   for (it = bulletHits->begin(); it != bulletHits->end(); ++it)
   {
      mWindow->draw(*(*it));
   }

   debugBodies();

   mWindow->popGLStates();

   mParallax->drawGrass();

//   glBegin(GL_TRIANGLES);

//   glColor3f(1.0f, 1.0f, 1.0f);
//   glVertex2f( 0.0f,  0.0f);
//   glVertex2f( 0.0f, 20.0f);
//   glVertex2f(20.0f,  0.0f);

//   glColor3f(0.0f, 0.0f, 0.2f);
//   glVertex2f( 0.0f, 20.0f);
//   glVertex2f(20.0f, 20.0f);
//   glVertex2f(20.0f,  0.0f);

//   glEnd();

   mWindow->display();
}


void BulletGame::initialize()
{
   // init level
   mLevel = new DemoLevel();
   sf::Vector3f startPosition = mLevel->getStartPosition();
   mLevel->initialize();

   sf::Image levelColors;
   sf::Image levelNormals;
   levelColors.loadFromFile("data/level_big_paths_2.psd");
   levelNormals.loadFromFile("data/level_NRM.png");
   mLevel->generateSprites(levelColors, BulletLevel::TextureTypeColor);
   mLevel->generateSprites(levelNormals, BulletLevel::TextureTypeNormal);

   // extract static boundaries from psd
   mLevel->parsePsdPaths(
      mPointMap,
      mPointSizeMap
   );

   // init player
   mPlayer = new BulletPlayer();
   mPlayer->setWorld(mLevel->getWorld());
   mPlayer->setStartPosition(startPosition.x, startPosition.y);
   mPlayer->initialize();

   // init window
   mHeight = 900;
   mWidth = 1600;

   mWindow = new sf::RenderWindow(sf::VideoMode(mWidth, mHeight), "sfml test");

   mWindow->setFramerateLimit(60);
   mWindow->setKeyRepeatEnabled(false);

   // init opengl
   glEnable(GL_TEXTURE_2D);
   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho (0, mWidth, mHeight, 0, 0, 1);
   glMatrixMode(GL_MODELVIEW);

   // init camera ('view')
   mView = new sf::View();
   mView->reset(sf::FloatRect(0, 0, mWidth, mHeight));
   mView->setViewport(sf::FloatRect(0, 0, 1.0f, 1.0f));

   // helpers
   mDebugDraw = new DebugDraw(mWindow);

//   mWindow->setActive(false);
//   sf::Thread thread(&renderingThread, mWindow);
//   thread.launch();

   mParallax = new Parallax();
   mParallax->initialize();
}


void BulletGame::gameLoop()
{
   sf::Event event;

   while (mWindow->isOpen())
   {
      eventLoop(&event);
      keyboardFuckup();

      mLevel->getWorld()->Step(1 / 60.f, 8, 3);
      Weapon::cleanupBullets();

      std::list<b2Vec2> bulletDetonations = Weapon::getDetonationPositions();

      std::list<b2Vec2>::iterator it;
      for (it = bulletDetonations.begin(); it != bulletDetonations.end(); ++it)
      {
         b2Vec2 vec = *it;
         float gx = vec.x * PPM;
         float gy = vec.y * PPM;

         BulletHitAnimation::add(gx, gy);
      }

      BulletHitAnimation::updateAnimations(mDeltaClock.getElapsedTime());

      float delta = mDeltaClock.getElapsedTime().asSeconds();

      mDeltaClock.restart();

      mPlayer->act();
      mPlayer->updateAnimation();
      mPlayer->syncWithBox2d();
      mPlayer->getSpineDrawable()->update(delta);

      draw();
   }
}


void BulletGame::keyboardFuckup()
{
   int keys = mPlayer->getKeysPressed();

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
   {
      keys |= BulletPlayer::KeyPressedUp;
   }
   else
   {
      keys &= ~BulletPlayer::KeyPressedUp;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
   {
      keys |= BulletPlayer::KeyPressedDown;
   }
   else
   {
      keys &= ~BulletPlayer::KeyPressedDown;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
   {
      keys |= BulletPlayer::KeyPressedLeft;
   }
   else
   {
      keys &= ~BulletPlayer::KeyPressedLeft;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
   {
      keys |= BulletPlayer::KeyPressedRight;
   }
   else
   {
      keys &= ~BulletPlayer::KeyPressedRight;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
   {
      keys |= BulletPlayer::KeyPressedFire;
   }
   else
   {
      keys &= ~BulletPlayer::KeyPressedFire;
   }

   mPlayer->setKeysPressed(keys);
}


void BulletGame::eventLoop(sf::Event* event)
{
   while (mWindow->pollEvent(*event))
   {
      if (event->type == sf::Event::Closed)
      {
         mWindow->close();
      }

      else if (event->type == sf::Event::MouseWheelMoved)
      {
         // float z = 0.0f;
         sf::Vector3f pos = mLevel->getBumpMap()->getLightPosition();
         pos.z += event->mouseWheel.delta * 0.1f;
         // mLevel->getBumpMap()->setLightPosition(pos);
         printf("mouse wheel => light z=%f\n", pos.z);
         flushall();
      }

      else if (event->type == sf::Event::MouseMoved)
      {
         float x = event->mouseMove.x / (float)mWidth;
         float y = event->mouseMove.y / (float)mHeight;
         sf::Vector3f pos = mLevel->getBumpMap()->getLightPosition();
         pos.x = x;
         pos.y = y;
         // mLevel->getBumpMap()->setLightPosition(pos);
         printf("mouse move => light x,y: %f, %f\n", pos.x, pos.y);
         flushall();
      }

      else if (event->type == sf::Event::KeyPressed)
      {
         int keys = mPlayer->getKeysPressed();

         if (event->key.code == sf::Keyboard::Up)
         {
            keys |= BulletPlayer::KeyPressedUp;
         }

         if (event->key.code == sf::Keyboard::Down)
         {
            keys |= BulletPlayer::KeyPressedDown;
         }

         if (event->key.code == sf::Keyboard::Left)
         {
            keys |= BulletPlayer::KeyPressedLeft;
         }

         if (event->key.code == sf::Keyboard::Right)
         {
            keys |= BulletPlayer::KeyPressedRight;
         }

         mPlayer->setKeysPressed(keys);
      }

      if (event->type == sf::Event::KeyReleased)
      {
         int keys = mPlayer->getKeysPressed();

         if (event->key.code == sf::Keyboard::Up)
         {
            keys &= ~BulletPlayer::KeyPressedUp;
         }

         if (event->key.code == sf::Keyboard::Down)
         {
            keys &= ~BulletPlayer::KeyPressedDown;
         }

         if (event->key.code == sf::Keyboard::Left)
         {
            keys &= ~BulletPlayer::KeyPressedLeft;
         }

         if (event->key.code == sf::Keyboard::Right)
         {
            keys &= ~BulletPlayer::KeyPressedRight;
         }

         mPlayer->setKeysPressed(keys);
      }
   }
}


void BulletGame::debugBodies()
{
   for (
      b2Body* bodyIterator = mLevel->getWorld()->GetBodyList();
      bodyIterator != 0;
      bodyIterator = bodyIterator->GetNext()
   )
   {
      // dynamic stuff
      if (bodyIterator->GetType() == b2_dynamicBody)
      {
         b2Fixture* f = bodyIterator->GetFixtureList();
         while (f)
         {
            b2Fixture* next = f->GetNext();
            b2Shape *shape = f->GetShape();

            if (shape->GetType() == b2Shape::e_polygon)
            {
               /*
               b2PolygonShape* poly = (b2PolygonShape*)shape;

               int vertexCount = poly->GetVertexCount();
               b2Vec2* vertices = new b2Vec2[vertexCount];

               for( int i = 0; i < vertexCount; i++ )
               {
                  b2Vec2 vec2 = poly->GetVertex(i);
                  vertices[i] = vec2;
                  vertices[i].x += bodyIterator->GetPosition().x;
                  vertices[i].y += bodyIterator->GetPosition().y;
               }

               mDebugDraw->DrawPolygon(
                  vertices,
                  vertexCount,
                  b2Color(1,0,0,1)
               );

               delete[] vertices;
               */
            }
            else if (shape->GetType() == b2Shape::e_circle)
            {
               mDebugDraw->DrawSolidCircle(
                  bodyIterator->GetPosition(),
                  shape->m_radius,
                  b2Vec2(0, 0),
                  b2Color(1.0f, 0.0f, 0.0f, 1.0f)
               );

               /*
               mDebugDraw->DrawCircle(
                  bodyIterator->GetPosition(),
                  shape->m_radius,
                  b2Color(0.4f, 0.4f, 0.4f, 1.0f)
               );
               */
            }

            f = next;
         }
      }
      else
      {
         /*
         mDebugDraw->DrawPolygon(
            mPointMap[bodyIterator],
            mPointSizeMap[bodyIterator],
            b2Color(1,0,0)
         );
         */
      }
   }
}

