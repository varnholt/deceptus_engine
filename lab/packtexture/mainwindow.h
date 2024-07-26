#pragma once

#include <QMainWindow>
#include <memory>
#include <vector>

struct PackTexture;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
   explicit MainWindow(QWidget *parent = 0);
   ~MainWindow();

public slots:
   void load();
   void pack();
   void setSize();
   void log(const QString& line);

protected:
   void resizeEvent(QResizeEvent* event) override;

private:
   void updateTextureLabel();
   std::vector<int> _sizes;
   std::vector<QAction*> _size_actions;

   Ui::MainWindow* _ui;
   QPixmap _texture;
   std::unique_ptr<PackTexture> _pack_texture;
};
