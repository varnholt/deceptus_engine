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
   static const auto black = qRgba(0, 0, 0, 128);
   static const auto transparent = qRgba(0, 0, 0, 0);
}


QImage blurImage(const QImage& inputImage)
{
   // create black/transparent texture which is blurred
   std::cout << "[x] creating black/transparent texture" << std::endl;

   QImage blackWhite(inputImage);
   for (auto x = 0; x < blackWhite.width(); x++)
   {
      for (auto y = 0; y < blackWhite.height(); y++)
      {
         const auto px = inputImage.pixel(x, y);
         if (qAlpha(px) != 0)
         {
            blackWhite.setPixel(x, y, black);
         }
      }
   }

   // blur the thing
   std::cout << "[x] blurring black/transparent texture" << std::endl;

   auto blur = new QGraphicsBlurEffect();
   blur->setBlurRadius(blurRadius);

   auto item = new QGraphicsPixmapItem();
   item->setPixmap(QPixmap::fromImage(blackWhite));
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

   // cut out original contents
   std::cout << "[x] cutting out original contents... " << std::endl;
   for (auto x = 0; x < outputImage.width(); x++)
   {
      for (auto y = 0; y < outputImage.height(); y++)
      {
         const auto px = inputImage.pixel(x, y);
         if (qAlpha(px) != 0)
         {
            outputImage.setPixel(x, y, transparent);
         }
      }
   }

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

   std::cout << "[x] loading original texture" << std::endl;
   const QImage source(fileInfo.filePath());
   const QImage result = blurImage(source);

   std::cout << "[x] writing texture to disk" << std::endl;
   result.save(outputFilename);
   std::cout << "[x] written ao texture to: " << outputFilename.toStdString() << std::endl;

   return 0;
}

