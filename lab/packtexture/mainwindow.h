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


private:

  std::vector<int> sizes_;
  std::vector<QAction*> sizeActions_;

  Ui::MainWindow* ui_;
  std::unique_ptr<PackTexture> packTexture_;


public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

public slots:

  void load();
  void pack();
  void setSize();
};

