#include "mainwindow.h"
#include <QActionGroup>
#include <QFileDialog>
#include <QSettings>
#include "packtexture.h"
#include "ui_mainwindow.h"

const auto defaultSize = 512;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), _ui(new Ui::MainWindow)
{
   _ui->setupUi(this);

   auto menu = _ui->menuBar_->addMenu(tr("file"));
   menu->addAction(tr("load"), this, &MainWindow::load);
   menu->addAction(tr("pack"), this, &MainWindow::pack);

   auto options = _ui->menuBar_->addMenu(tr("options"));

   _sizes = {1024, 512, 256, 128, 64, 32, 24, 16};

   std::for_each(
      std::begin(_sizes),
      std::end(_sizes),
      [&](auto val)
      {
         auto option = options->addAction(tr("%1x%1").arg(val), this, &MainWindow::setSize);
         option->setCheckable(true);

         if (val == defaultSize)
         {
            option->setChecked(true);
         }

         _size_actions.push_back(option);
      }
   );

   _pack_texture = std::make_unique<PackTexture>();
   _pack_texture->_size = defaultSize;
   _pack_texture->_update_progress = [&](int value) { _ui->progressBar_->setValue(value); };
}

MainWindow::~MainWindow()
{
   delete _ui;
}

void MainWindow::updateTextureLabel()
{
   _texture = QPixmap::fromImage(_pack_texture->_image);
   const auto w = static_cast<float>(width());
   const auto h = static_cast<float>(height());
   _ui->textureLabel_->_pixmap = _texture.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
   _ui->textureLabel_->_scale_x = _texture.width() / w;
   _ui->textureLabel_->_scale_y = _texture.height() / h;
}

void MainWindow::load()
{
   QSettings fileSettings("texpack.ini", QSettings::IniFormat);
   const auto path = fileSettings.value("path").toString();
   const auto filename = QFileDialog::getOpenFileName(this, "load image", path, "*.png");
   if (filename.isEmpty())
   {
      return;
   }

   _pack_texture->load(filename);

   updateTextureLabel();

   fileSettings.setValue("path", QFileInfo(filename).absolutePath());
   fileSettings.sync();
}

void MainWindow::pack()
{
   _ui->info_->clear();
   _ui->info_->setText(tr("detecting empty areas..."));
   _pack_texture->pack();
   _ui->textureLabel_->_quads = &_pack_texture->_quads;
   _ui->textureLabel_->repaint();
   _ui->info_->setText(tr("creating texture..."));

   _pack_texture->dump();

   if (_pack_texture->_texture_size != 0)
   {
      _ui->info_->setText(tr("created %1x%1px texture (%2)").arg(_pack_texture->_texture_size).arg(_pack_texture->_filename));
   }
   else
   {
      _ui->info_->setText(tr("failed to create suitable texture texture; please try smaller chunk sizes"));
   }
}

void MainWindow::setSize()
{
   auto it = std::find_if(std::begin(_size_actions), std::end(_size_actions), [&](auto action) { return action == sender(); });
   std::for_each(std::begin(_size_actions), std::end(_size_actions), [&](auto action) { action->setChecked(false); });
   (*it)->setChecked(true);
   auto size = _sizes[it - _size_actions.begin()];
   _pack_texture->_size = size;
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
   updateTextureLabel();
   _ui->textureLabel_->repaint();
}
