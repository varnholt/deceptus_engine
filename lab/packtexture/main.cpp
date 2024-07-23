#include "mainwindow.h"
#include "packtexture.h"

#include <QApplication>
#include <QCommandLineParser>
#include <iostream>

// support qt6
#if QT_VERSION > 0x060000
#include <QImageReader>
#endif

int main(int argc, char* argv[])
{
   QApplication a(argc, argv);

   // support qt6
#if QT_VERSION > 0x060000
   QImageReader::setAllocationLimit(4096);
#endif

   QCoreApplication::setApplicationName("packtexture");
   QCoreApplication::setApplicationVersion("1.0");

   QCommandLineParser parser;
   parser.addHelpOption();
   parser.addVersionOption();

   QCommandLineOption input_option(
      QStringList() << "i"
                    << "input",
      QCoreApplication::translate("main", "specify input file"),
      QCoreApplication::translate("main", "file path")
   );

   QCommandLineOption size_option(
      QStringList() << "s"
                    << "size",
      QCoreApplication::translate("main", "specify quad size, available values: 16, 24, 32, 64, 128, 256, 512, 1024"),
      QCoreApplication::translate("main", "quad size")
   );

   parser.addOption(input_option);
   parser.addOption(size_option);
   parser.process(a);

   if (parser.isSet(input_option))
   {
      PackTexture pt;
      int32_t size = pt._size;

      if (parser.isSet(size_option))
      {
         bool ok = false;
         size = parser.value(size_option).toInt(&ok);

         if (!ok)
         {
            std::cerr << "[!] bad value for size parameter" << std::endl;
            parser.showHelp(1);
         }

         auto supportedValues = {16, 24, 32, 64, 128, 256, 512, 1024};

         auto it = std::find(supportedValues.begin(), supportedValues.end(), size);
         if (it == supportedValues.end())
         {
            std::cerr << "[!] bad value for size parameter" << std::endl;
            parser.showHelp(2);
         }
      }

      pt._size = size;
      if (!pt.load(parser.value(input_option)))
      {
         std::cerr << "[!] unable to load file: " << parser.value(input_option).toStdString() << std::endl;
         exit(3);
      }
      pt.pack();
      pt.dump();

      return 0;
   }

   MainWindow w;
   w.show();
   // w.setFixedSize(550, 550);

   return a.exec();
}
