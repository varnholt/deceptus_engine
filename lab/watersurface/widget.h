#ifndef WIDGET_H
#define WIDGET_H

#include <QElapsedTimer>
#include <QWidget>

#include "watersurface.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
   Q_OBJECT

public:
   Widget(QWidget *parent = nullptr);
   ~Widget();

protected:
   void paintEvent(QPaintEvent* event) override;
   void mousePressEvent(QMouseEvent* event) override;

private:
   Ui::Widget *ui;
   WaterSurface _surface;
   QElapsedTimer _elapsed;
};
#endif // WIDGET_H
