#include "stomper.h"

#include "tmxparser/tmximage.h"
#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxpolyline.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxproperties.h"
#include "tmxparser/tmxtileset.h"


//         0123456789ABC
//
//         <#         #>      0
//         <#         #>      1
//         <##### #####>      2
//         <#         #>      3
//         <#         #>      4
//         MMMMM     #        5
//         #####     #        6
//           #       #        7
//           #       #        8
//           #     #####      9
//           #     VVVVV      A
//         01234560123456     B
//         01234560123456     C
//         01234560123456     D
//         01234560123456     E

Stomper::Stomper()
{

}



std::vector<std::shared_ptr<GameMechanism>> Stomper::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath,
   const std::shared_ptr<b2World>&
)
{
   // std::cout << "load portal layer" << std::endl;

   if (!tileSet)
   {
      return {};
   }

   std::vector<std::shared_ptr<GameMechanism>> stompers;

   return stompers;
}
