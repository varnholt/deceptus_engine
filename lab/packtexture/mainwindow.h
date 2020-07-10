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

private:
   std::vector<int> mSizes;
   std::vector<QAction*> mSizeActions;

   Ui::MainWindow* mUi;
   std::unique_ptr<PackTexture> mPackTexture;
};

