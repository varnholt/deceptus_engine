#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QObject>
#include <QPainter>

// support qt6
#if QT_VERSION > 0x060000
#include <QImageReader>
#endif


#include <cstdint>
#include <optional>
#include <iostream>

namespace
{
   std::optional<float> alpha;
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
            blackWhite.setPixel(
               x,
               y,
               alpha.has_value()
                  ? qRgba(0, 0, 0, std::min(static_cast<int32_t>(alpha.value() * 255), 255))
                  : black
            );
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

   // support qt6
#if QT_VERSION > 0x060000
   QImageReader::setAllocationLimit(4096);
#endif

   static constexpr auto inputParam = "input";

   QCommandLineOption alphaOption(
      QStringList() << "a" << "alpha",
      QCoreApplication::translate("main", "specify the alpha value for the ambient occlusion (range: 0..1, default = 0.5)"),
      QCoreApplication::translate("main", "transparency")
   );

   QCommandLineParser parser;
   parser.addHelpOption();
   parser.addPositionalArgument(inputParam, QCoreApplication::translate("main", "input texture"));
   parser.addOption(alphaOption);

   parser.parse(QCoreApplication::arguments());

   const auto& args = parser.positionalArguments();

   if (args.isEmpty())
   {
      parser.showHelp(1);
   }

   if (parser.isSet(alphaOption))
   {
      bool ok = false;
      alpha = parser.value(alphaOption).toFloat(&ok);

      if (!ok)
      {
         std::cerr << "[!] bad value for alpha" << std::endl;
         parser.showHelp(1);
      }
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

