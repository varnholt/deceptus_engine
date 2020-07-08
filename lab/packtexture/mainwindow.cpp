#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "packtexture.h"
#include <QActionGroup>
#include <QFileDialog>
#include <QSettings>


const auto defaultSize = 512;


MainWindow::MainWindow(QWidget *parent)
 : QMainWindow(parent),
   ui_(new Ui::MainWindow)
{
   ui_->setupUi(this);

   auto menu = ui_->menuBar_->addMenu(tr("file"));
   menu->addAction(tr("load"), this, &MainWindow::load);
   menu->addAction(tr("pack"), this, &MainWindow::pack);

   auto options = ui_->menuBar_->addMenu(tr("options"));

   sizes_ = {1024, 512, 256, 128, 64, 32, 24, 16};

  std::for_each(std::begin(sizes_), std::end(sizes_), [&](auto val) {
     auto option = options->addAction(tr("%1x%1").arg(val), this, &MainWindow::setSize);
     option->setCheckable(true);

     if (val == defaultSize)
     {
        option->setChecked(true);
     }

     sizeActions_.push_back(option);
  });

   packTexture_ = std::make_unique<PackTexture>();
   packTexture_->size_ = defaultSize;
   packTexture_->updateProgress_ = [&](int value){ui_->progressBar_->setValue(value);};
}


MainWindow::~MainWindow()
{
   delete ui_;
}


void MainWindow::load()
{
   QSettings fileSettings("texpack.ini", QSettings::IniFormat);
   auto path = fileSettings.value("path").toString();

   auto filename = QFileDialog::getOpenFileName(this, "load image", path, "*.png");

   if (!filename.isEmpty())
   {
      packTexture_->load(filename);
      QPixmap pm = QPixmap::fromImage(packTexture_->image_);
      ui_->textureLabel_->pixmap_ = pm.scaled(512, 512, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
      ui_->textureLabel_->scaleX_ = 512.0f / pm.width();
      ui_->textureLabel_->scaleY_ = 512.0f / pm.height();
      fileSettings.setValue("path", QFileInfo(filename).absolutePath());
      fileSettings.sync();
   }
}


void MainWindow::pack()
{
   ui_->info_->clear();
   ui_->info_->setText(tr("detecting empty areas..."));
   packTexture_->pack();
   ui_->textureLabel_->quads_ = &packTexture_->quads_;
   ui_->textureLabel_->repaint();
   ui_->info_->setText(tr("creating texture..."));
   packTexture_->dump();
   ui_->info_->setText(tr("created %1x%1px texture").arg(packTexture_->textureSize_));
}


void MainWindow::setSize()
{
   auto it = std::find_if(std::begin(sizeActions_), std::end(sizeActions_), [&](auto action){return action == sender();});
   std::for_each(std::begin(sizeActions_), std::end(sizeActions_), [&](auto action){action->setChecked(false);});
   (*it)->setChecked(true);
   auto size = sizes_[it - sizeActions_.begin()];
   packTexture_->size_ = size;
}
