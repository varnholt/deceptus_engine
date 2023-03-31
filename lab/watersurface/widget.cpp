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

   // draw background
   painter.fillRect(QRect{0, 0, width(), height()}, QColor{30, 30, 30});

   // create poly from segments
   const auto height_px = height() / 2.0f;
   const auto& segments = _surface.getSegments();
   const auto segment_width = width() / segments.size();
   const auto y_offset = height() / 2;
   QList<QPointF> polygon;

   auto index = 0;
   for (const auto& segment : segments)
   {
      const auto x = static_cast<float>(index * segment_width);
      const auto y = y_offset + segment._height;
      polygon.push_back({x, y});
      index++;
   }

   // draw previous waves
   auto draw_background = [&](const QColor& color, const QList<QPointF>& poly)
   {
      QPolygonF background_poly(poly);
      background_poly.push_back({poly.back().x(), y_offset + height_px});
      background_poly.push_back({poly.front().x(), y_offset + height_px});
      painter.setPen(color);
      painter.setBrush(color);
      painter.drawPolygon(background_poly);
   };

   auto blend = [](const QColor& color_1, const QColor& color_2, float ratio) -> QColor
   {
      return QColor(
         qRound(qreal(color_1.red()) * (1.0 - ratio) + qreal(color_2.red()) * ratio),
         qRound(qreal(color_1.green()) * (1.0 - ratio) + qreal(color_2.green()) * ratio),
         qRound(qreal(color_1.blue()) * (1.0 - ratio) + qreal(color_2.blue()) * ratio),
         qRound(qreal(color_1.alpha()) * (1.0 - ratio) + qreal(color_2.alpha()) * ratio)
      );
   };

   float ratio = 0.0f;
   const auto white = QColor{255, 255, 255, 255};
   const auto grey = QColor{100, 100, 100, 255};
   const auto blue = QColor{0, 40, 73, 255};
   for (const auto& poly : _polygons)
   {
      draw_background(blue, poly);

      painter.setPen(blend(white, grey, ratio));
      painter.drawPolyline(poly);

      ratio += 1.0f / _polygons.size();
   }

   // draw latest wave
   draw_background(blue, polygon);
   painter.setPen({255, 255, 255, 255});
   painter.drawPolyline(polygon);

   // store previous polygons
   _polygons.push_front(polygon);
   while (_polygons.size() > 10)
   {
      _polygons.pop_back();
   }
}

void Widget::mousePressEvent(QMouseEvent* event)
{
   const auto pos_normalized = event->pos().x() / static_cast<float>(width());
   const auto segment_index = pos_normalized * _surface.getSegments().size();
   _surface.splash(segment_index, 48.0f);
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
