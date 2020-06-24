#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QObject>
#include <QPainter>

#include <cstdint>
#include <iostream>

namespace
{
   constexpr auto blurRadius = 8;
}


QImage blurImage(const QImage& inputImage)
{
   auto blur = new QGraphicsBlurEffect();
   blur->setBlurRadius(blurRadius);

   auto item = new QGraphicsPixmapItem();
   item->setPixmap(QPixmap::fromImage(inputImage));
   item->setGraphicsEffect(blur);

   QGraphicsScene scene;
   scene.addItem(item);

   QImage outputImage(inputImage.size(), QImage::Format_ARGB32);
   outputImage.fill(Qt::transparent);

   QPainter painter(&outputImage);

   scene.render(
      &painter,
      {},
      QRectF{
         0.0,
         0.0,
         static_cast<double>(inputImage.width()),
         static_cast<double>(inputImage.height())
      }
   );

   return outputImage;
}


int32_t main(int32_t argc, char* argv[])
{
   QApplication a(argc, argv);

   static constexpr auto inputParam = "input";

   QCommandLineParser parser;
   parser.addHelpOption();
   parser.addPositionalArgument(inputParam, QCoreApplication::translate("main", "input texture"));
   parser.parse(QCoreApplication::arguments());

   const auto& args = parser.positionalArguments();

   if (args.isEmpty())
   {
      parser.showHelp(1);
   }

   std::cout << "[x] processing texture: " << args.first().toStdString() << std::endl;

   QFileInfo fileInfo{args.first()};

   if (!fileInfo.exists())
   {
      std::cerr<< "[!] unable to read file: " << fileInfo.filePath().toStdString() << std::endl;
   }

   auto outputFilename = QString("%1_ao.%2").arg(fileInfo.baseName()).arg(fileInfo.suffix());
   const QImage source(fileInfo.filePath());
   const QImage result = blurImage(source);
   result.save(outputFilename);
   std::cout << "[x] written ao texture to: " << outputFilename.toStdString() << std::endl;

   return 0;
}

