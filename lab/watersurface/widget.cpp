#include "widget.h"
#include "ui_widget.h"

#include <QPainter>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
      , ui(new Ui::Widget)
{
   ui->setupUi(this);
}

Widget::~Widget()
{
   delete ui;
}

void Widget::paintEvent(QPaintEvent* /*event*/)
{
   QPainter painter(this);
   painter.drawLine(0, 0, 200, 200);
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
