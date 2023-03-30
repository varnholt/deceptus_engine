#include "widget.h"
#include "ui_widget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
      , ui(new Ui::Widget)
{
   ui->setupUi(this);

   auto timer = new QTimer(this);
   timer->setInterval(16);
   connect(
      timer,
      &QTimer::timeout,
      this,
      [this]()
      {
         const auto dt = _elapsed.elapsed() * 0.001f;
         _surface.update(dt);
         _elapsed.restart();
         repaint();
      }
   );
   timer->start();
}

Widget::~Widget()
{
   delete ui;
}

void Widget::paintEvent(QPaintEvent* /*event*/)
{
   QPainter painter(this);

   const auto& segments = _surface.getSegments();
   const auto segment_width = width() / segments.size();

   const auto y_offset = height() / 2;

   for (auto i = 0; i < segments.size() - 1; i++)
   {
      painter.drawLine(i * segment_width, segments[i]._height + y_offset, (i + 1) * segment_width, segments[i + 1]._height + y_offset);
   }
}

void Widget::mousePressEvent(QMouseEvent* event)
{
   const auto pos_normalized = event->pos().x() / static_cast<float>(width());
   const auto segment_index = pos_normalized * _surface.getSegments().size();
   _surface.splash(segment_index, 200.0f);
}

// Hooks law
//
// F = -kx
//
// F: force
// k: spring constant
// x: displacement from the spring's original length
//
//
// Newton's Second Law of Motion
// F = ma
// F: force
// m: mass
// a: acceleration
//
//
// The two combined gives the acceleration for a water particle:
// ma = -kx
// a = -kx/m
//
// As we're just processing water particles, k/m is considered a constant
