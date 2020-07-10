#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "packtexture.h"
#include <QActionGroup>
#include <QFileDialog>
#include <QSettings>


const auto defaultSize = 512;


MainWindow::MainWindow(QWidget *parent)
 : QMainWindow(parent),
   mUi(new Ui::MainWindow)
{
   mUi->setupUi(this);

   auto menu = mUi->menuBar_->addMenu(tr("file"));
   menu->addAction(tr("load"), this, &MainWindow::load);
   menu->addAction(tr("pack"), this, &MainWindow::pack);

   auto options = mUi->menuBar_->addMenu(tr("options"));

   mSizes = {1024, 512, 256, 128, 64, 32, 24, 16};

  std::for_each(std::begin(mSizes), std::end(mSizes), [&](auto val) {
     auto option = options->addAction(tr("%1x%1").arg(val), this, &MainWindow::setSize);
     option->setCheckable(true);

     if (val == defaultSize)
     {
        option->setChecked(true);
     }

     mSizeActions.push_back(option);
  });

   mPackTexture = std::make_unique<PackTexture>();
   mPackTexture->mSize = defaultSize;
   mPackTexture->mUpdateProgress = [&](int value){mUi->progressBar_->setValue(value);};
}


MainWindow::~MainWindow()
{
   delete mUi;
}


void MainWindow::load()
{
   QSettings fileSettings("texpack.ini", QSettings::IniFormat);
   auto path = fileSettings.value("path").toString();

   auto filename = QFileDialog::getOpenFileName(this, "load image", path, "*.png");

   if (!filename.isEmpty())
   {
      mPackTexture->load(filename);
      QPixmap pm = QPixmap::fromImage(mPackTexture->mImage);
      mUi->textureLabel_->mPixmap = pm.scaled(512, 512, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
      mUi->textureLabel_->mScaleX = 512.0f / pm.width();
      mUi->textureLabel_->mScaleY = 512.0f / pm.height();
      fileSettings.setValue("path", QFileInfo(filename).absolutePath());
      fileSettings.sync();
   }
}


void MainWindow::pack()
{
   mUi->info_->clear();
   mUi->info_->setText(tr("detecting empty areas..."));
   mPackTexture->pack();
   mUi->textureLabel_->mQuads = &mPackTexture->mQuads;
   mUi->textureLabel_->repaint();
   mUi->info_->setText(tr("creating texture..."));
   mPackTexture->dump();
   mUi->info_->setText(tr("created %1x%1px texture").arg(mPackTexture->mTextureSize));
}


void MainWindow::setSize()
{
   auto it = std::find_if(std::begin(mSizeActions), std::end(mSizeActions), [&](auto action){return action == sender();});
   std::for_each(std::begin(mSizeActions), std::end(mSizeActions), [&](auto action){action->setChecked(false);});
   (*it)->setChecked(true);
   auto size = mSizes[it - mSizeActions.begin()];
   mPackTexture->mSize = size;
}
