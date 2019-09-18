#include "psd.h"

#include "image.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory.h>
#include <stdlib.h>


// https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/#50577409_pgfId-1036097


namespace
{
   void check(std::istream& stream)
   {
       if (!stream)
       {
           std::cerr << "stream is consumed :(" << std::endl;
           // exit(-1);
       }

       // std::cout << "read " << stream.gcount() << " bytes" << std::endl;

       if (stream.gcount() == 0)
       {
           std::cerr << "no data read :(" << std::endl;
           // exit(-1);
       }
   }

   // void read(int8_t& val, std::istream& stream)
   // {
   //    stream.read(reinterpret_cast<char*>(&val), 1);
   //    check(stream);
   // }

   void read(uint8_t& val, std::istream& stream)
   {
      stream.read(reinterpret_cast<char*>(&val), 1);
      check(stream);
   }

   void read(int16_t& val, std::istream& stream)
   {
      // std::cout << "reading " << sizeof(val) << " bytes" << std::endl;
      auto bytes = reinterpret_cast<char*>(&val);
      stream.read(bytes, sizeof(val));
      std::reverse(bytes, bytes + sizeof(val));
      check(stream);
   }

   void read(uint16_t& val, std::istream& stream)
   {
      // std::cout << "reading " << sizeof(val) << " bytes" << std::endl;
      auto bytes = reinterpret_cast<char*>(&val);
      stream.read(bytes, sizeof(val));
      std::reverse(bytes, bytes + sizeof(val));
      check(stream);
   }

   void read(uint32_t& val, std::istream& stream)
   {
      // std::cout << "reading " << sizeof(val) << " bytes" << std::endl;
      auto bytes = reinterpret_cast<char*>(&val);
      stream.read(bytes, sizeof(val));
      std::reverse(bytes, bytes + sizeof(val));
      check(stream);
   }

   void read(int32_t& val, std::istream& stream)
   {
      // std::cout << "reading " << sizeof(val) << " bytes" << std::endl;
      auto bytes = reinterpret_cast<char*>(&val);
      stream.read(bytes, sizeof(val));
      std::reverse(bytes, bytes + sizeof(val));
      check(stream);
   }

   void read(std::vector<uint8_t>& val, std::istream& stream)
   {
      // std::cout << "reading " << val.size() * sizeof(uint8_t) << " bytes" << std::endl;
      stream.read(reinterpret_cast<char*>(val.data()), val.size());
      check(stream);
   }

   void read(unsigned char* val, std::size_t size, std::istream& stream)
   {
      // std::cout << "reading " << size << " bytes" << std::endl;
      stream.read(reinterpret_cast<char*>(val), size);
      check(stream);
   }

   template<std::size_t arraySize>
   void read(std::array<uint8_t, arraySize>& val, std::istream& stream)
   {
      // std::cout << "reading " << arraySize * sizeof(uint8_t) << " bytes" << std::endl;
      stream.read(reinterpret_cast<char*>(val.data()), arraySize);
      check(stream);
   }
}

// PSD Header -----------------------------------------------------------------

#define VISIBILITY_FLAG 0x02

int32_t PSD::Header::getWidth() const
{
   return mWidth;
}


int32_t PSD::Header::getHeight() const
{
   return mHeight;
}


void PSD::Header::load(std::istream& stream)
{
   // File Header Section
   //
   // 4   Signature: always equal to '8BPS' . Do not try to read the file if the
   //     signature does not match this value.
   //
   // 2   Version: always equal to 1.
   //     Do not try to read the file if the version does not match this value.
   //
   // 6   Reserved: must be zero.
   //
   // 2   The number of channels in the image, including any alpha channels.
   //     Supported range is 1 to 56.
   //
   // 4   The height of the image in pixels. Supported range is 1 to 30,000.
   //
   // 4   The width of the image in pixels. Supported range is 1 to 30,000.
   //
   // 2   Depth: the number of bits per channel. Supported values are 1, 8, 16 and 32.
   //
   // 2   The color mode of the file. Supported values are:
   //     Bitmap = 0; Grayscale = 1; Indexed = 2; RGB = 3; CMYK = 4; Multichannel = 7;
   //     Duotone = 8; Lab = 9.

   read<sizeof(mSignature)>(mSignature, stream);
   read(mVersion, stream);
   read<sizeof(mReserved)>(mReserved, stream);
   read(mChannels, stream);
   read(mHeight, stream);
   read(mWidth, stream);
   read(mDepth, stream);
   read(mMode, stream);
}


// Layer ----------------------------------------------------------------------

const std::string& PSD::Layer::getName() const
{
   return mName;
}

int32_t PSD::Layer::getBottom() const
{
   return mBottom;
}

int32_t PSD::Layer::getTop() const
{
   return mTop;
}

int32_t PSD::Layer::getLeft() const
{
   return mLeft;
}

int32_t PSD::Layer::getWidth() const
{
   return mRight - mLeft;
}

int32_t PSD::Layer::getHeight() const
{
   return mBottom - mTop;
}

void PSD::Layer::move(int32_t x, int32_t y)
{
   mRight += x;
   mLeft += x;

   mTop += y;
   mBottom +=y;
}

void PSD::Layer::setX(int32_t x)
{
   int32_t width = mRight - mLeft;

   mLeft = x;
   mRight = x + width;
}

void PSD::Layer::setY(int32_t y)
{
   int32_t height = mBottom - mTop;

   mTop = y;
   mBottom = y + height;
}

void PSD::Layer::setBottom(int32_t v)
{
   mBottom = v;
}


void PSD::Layer::setTop(int32_t v)
{
   mTop = v;
}


const Image& PSD::Layer::getImage() const
{
    return mImage;
}


int32_t PSD::Layer::getOpacity() const
{
   return mOpacity;
}


bool PSD::Layer::isVisible() const
{
   return ((mFlags & VISIBILITY_FLAG) != 2);
}


void PSD::Layer::setVisible(bool visible)
{
   if (visible)
      mFlags &= ~VISIBILITY_FLAG;
   else
      mFlags |= VISIBILITY_FLAG;
}


const PSD::Layer::Channel& PSD::Layer::getChannel(int32_t id) const
{
   auto it = std::find_if(mChannels.begin(), mChannels.end(), [id](const Channel& c){return c.getID() == id;});
   return *it;
}


PSD::Layer::SectionDivider PSD::Layer::getSectionDivider() const
{
   return mSectionDivider;
}


PSD::ColorFormat PSD::Layer::getColorFormat() const
{
   return mColorFormat;
}


void PSD::Layer::setColorFormat(const PSD::ColorFormat& colorFormat)
{
   mColorFormat = colorFormat;
}


void PSD::Layer::setOpacity(int32_t opacity)
{
   mOpacity = opacity;
}


void PSD::Layer::loadLayerRecords(std::istream& stream)
{
   // Layer records
   //
   // 4 * 4              Rectangle containing the contents of the layer.
   //                    Specified as top, left, bottom, right coordinates
   // 2                  Number of channels in the layer
   // 6 * # of channels  Channel information. Six bytes per channel, consisting of:
   //                    2 bytes for Channel ID:
   //                        0 = red, 1 = green, etc.; -1 = transparency mask;
   //                       -2 = user supplied layer mask, -3 real user supplied layer mask
   //                    4 bytes for length of corresponding channel data
   //                    -> Channel image data
   //                    -> https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/#50577409_26431
   // 4                 Blend mode signature: '8BIM'
   // 4                 Blend mode key
   // 1                 Opacity. 0 = transparent ... 255 = opaque
   // 1                 Clipping: 0 = base, 1 = non-base
   // 1                 Flags
   // 1                 Filler (zero)
   // 4                 Length of the extra data field ( = the total length of the next five fields).
    // Variable         Layer mask data: See See Layer mask / adjustment layer data for structure.
    //                  Can be 40 bytes, 24 bytes, or 4 bytes if no layer mask.
    // Variable         Layer blending ranges: See See Layer blending ranges data.
    // Variable         Layer name: Pascal string, padded to a multiple of 4 bytes.

   read(mTop, stream);
   read(mLeft, stream);
   read(mBottom, stream);
   read(mRight, stream);

   read(mChannelCount, stream);

   for (auto i = 0; i < mChannelCount; i++)
   {
      Channel channel;
      channel.load(stream);
      mChannels.push_back(channel);
   }

   read(mBlendModeSignature, stream);
   read(mBlendModeKey, stream);
   read(mOpacity, stream);
   read(mClipping, stream);
   read(mFlags, stream);

   stream.ignore(1); // filler

   int32_t extraDataLength = 0;
   read(extraDataLength, stream);

   auto layerStart = stream.tellg();

   // Layer mask / adjustment layer data (ignored)
   {
      int32_t size;
      read(size, stream);
      stream.ignore(size);
   }

   // Layer blending ranges data (ignored)
   {
      int32_t length;
      read(length, stream);
      stream.ignore(length);
   }

   // Layer name: Pascal string, padded to a multiple of 4 bytes.
   mName = loadString(stream);

   // std::cout << "layer name: " << mName << std::endl;

   int32_t blockHeader= 0;
   while (extraDataLength - (stream.tellg() - layerStart) > 4)
   {
      if (blockHeader == 0)
      {
         read(blockHeader, stream);
      }
      else
      {
         uint8_t byte = 0;
         read(byte, stream);
         blockHeader = (blockHeader << 8) | byte;
      }

      if (blockHeader == '8BIM')
      {
         int32_t blockId = 0;
         int32_t blockSize = 0;

         read(blockId, stream);
         read(blockSize, stream);
         auto blockPos = stream.tellg();

         if (blockId == 'lsct')
         {
            int32_t sectionDivider;
            read(sectionDivider, stream);
            mSectionDivider = static_cast<SectionDivider>(sectionDivider);
         }

         // Unicode layer name
         if (blockId == 'luni')
         {
            uint32_t length = 0;
            read(length, stream);

            auto name = new char[length + 1];
            name[length] = 0;

            for (auto i = 0u; i < length; i++)
            {
               uint16_t word = 0;
               read(word, stream);
               name[i] = word & 255;
            }

            mName = std::string(name);
            delete[] name;
         }

         // skip rest of block
         stream.ignore(blockSize - (stream.tellg() - blockPos));
         blockHeader= 0;
      }
   }

   // skip rest of data
   stream.ignore(extraDataLength - (stream.tellg() - layerStart));
}


void PSD::Layer::loadChannelImageData(std::istream& stream)
{
   // 2        Compression. 0 = Raw Data, 1 = RLE compressed, 2 = ZIP without prediction, 3 = ZIP with prediction.
   //
   // Variable Image data.
   //
   //          If the compression code is 0, the image data is just the raw image data, whose size is
   //          calculated as (LayerBottom - LayerTop) * (LayerRight - LayerLeft) (from the first field in
   //          See Layer records).
   //
   //          If the compression code is 1, the image data starts with the byte counts for all the scan
   //          lines in the channel (LayerBottom - LayerTop) , with each count stored as a two-byte value.
   //
   //          If the layer's size, and therefore the data, is odd, a pad byte will be inserted at the end of the row.
   //          If the layer is an adjustment layer, the channel data is undefined (probably all white.)

   int32_t height = mBottom - mTop;
   int32_t width = mRight - mLeft;

   for (auto& channel : mChannels)
   {
      uint16_t compression = 0;
      read(compression, stream);

      switch (compression)
      {
         case 0: // raw
            channel.loadRaw(width, height, stream);
            break;

         case 1: // rle compressed
            channel.loadRLE(width, height, stream);
            break;

         case 2: // zip without prediction
            std::cerr << "unsupported compression" << std::endl;
            // exit(-1);
            break;

         case 3: // zip with prediction
            std::cerr << "unsupported compression" << std::endl;
            // exit(-1);
            break;

         default:
            std::cerr << "unsupported compression" << std::endl;
            // exit(-1);
            break;
      }
   }

   if (mChannelCount == 3)
   {
      mChannels[mChannelCount].init(-1, width, height);
      mChannelCount++;
   }

   mImage.init(width, height);

   for (auto y = 0; y < height; y++)
   {
      uint32_t* dst = mImage.getScanline(y);

      uint8_t* red   = getChannel(0).getScanline(y);
      uint8_t* green = getChannel(1).getScanline(y);
      uint8_t* blue  = getChannel(2).getScanline(y);
      uint8_t* alpha = getChannel(-1).getScanline(y);

      for (auto x = 0; x < width; x++)
      {
         uint8_t a, r, g, b;

         a = alpha[x];

         if (a > 0)
         {
            r = red[x];
            g = green[x];
            b = blue[x];
         }
         else
         {
            r = 0;
            g = 0;
            b = 0;
         }

         switch (mColorFormat)
         {
            case PSD::ColorFormat::ARGB:
               dst[x] = (a << 24) | (r << 16) | (g << 8) | b;
               break;
            case PSD::ColorFormat::ABGR:
               dst[x] = (a << 24) | (b << 16) | (g << 8) | r;
               break;
         }
      }
   }
}


// Channel --------------------------------------------------------------------

short PSD::Layer::Channel::getID() const
{
   return mID;
}

const std::vector<uint8_t>& PSD::Layer::Channel::data() const
{
   return mData;
}


void PSD::Layer::Channel::load(std::istream& stream)
{
   read(mID, stream);
   read(mSize, stream);
}

// https://web.archive.org/web/20080705155158/http://developer.apple.com/technotes/tn/tn1023.html
//
// The first byte is a flag-counter byte that specifies whether or not the following data is packed,
// and the number of bytes involved.
// If this first byte is a negative number, the following data is packed and the number is a zero-based
// count of the number of times the data byte repeats when expanded. There is one data byte following the
// flag-counter byte in packed data; the byte after the data byte is the next flag-counter byte.
//
// If the flag-counter byte is a positive number, then the following data is unpacked and the number is a zero-based
// count of the number of incompressible data bytes that follow. There are (flag-counter+1) data bytes following
// the flag-counter byte. The byte after the last data byte is the next flag-counter byte.
//
// Given a pointer to the start of packed data, there is no way to know when you have reached the end of the packed
// data. Because UnPackBits requires the length of the unpacked data, you need to know either the length of the
// packed or unpacked data before you start unpacking.
//
//
// Consider the following example:
//
// Unpacked data:
// AA AA AA 80 00 2A AA AA AA AA 80 00 2A 22 AA AA AA AA AA AA AA AA AA AA
//
// After being packed by PackBits:
//
//     FE AA                    ; (-(-2)+1) = 3 bytes of the pattern $AA
//     02 80 00 2A              ; (2)+1 = 3 bytes of discrete data
//     FD AA                    ; (-(-3)+1) = 4 bytes of the pattern $AA
//     03 80 00 2A 22           ; (3)+1 = 4 bytes of discrete data
//     F7 AA                    ; (-(-9)+1) = 10 bytes of the pattern $AA
//
// or
//
//     FE AA 02 80 00 2A FD AA 03 80 00 2A 22 F7 AA
//     *     *           *     *              *
void PSD::Layer::Channel::unpackBits(
   std::vector<uint8_t>& dest,
   int32_t offset,
   size_t bytesPerScanline,
   std::istream& stream
)
{
   auto bytesRead = 0u;
   while (bytesRead < bytesPerScanline)
   {
      uint8_t controlByte = 0;
      read(controlByte, stream);
      bytesRead++;

      if (controlByte <= 0x80)
      {
         for (auto j = 0; j < controlByte + 1; j++)
         {
            uint8_t singleValue = 0;
            read(singleValue, stream);
            bytesRead++;

            // printf("%x ", singleValue);
            dest[offset] = singleValue;
            offset++;
         }
      }
      else
      {
         uint8_t spanValue = 0;
         read(spanValue, stream);
         bytesRead++;

         auto count = (256 - controlByte) + 1;
         for (auto j = 0; j < count; j++)
         {
            // printf("%x ", spanValue);
            dest[offset] = spanValue;
            offset++;
         }
      }
   }
}


void PSD::Layer::Channel::loadRLE(int32_t width, int32_t height, std::istream& stream)
{
   std::vector<uint16_t> scanlineByteCounts;
   for (auto y = 0; y < height; y++)
   {
      uint16_t val = 0;
      read(val, stream);
      scanlineByteCounts.push_back(val);
   }

   mWidth = width;
   mData.resize(width * height);

   for (auto y = 0; y < height; y++)
   {
      const auto bytesPerScanline = scanlineByteCounts[y];
      const auto offset = y * width;
      unpackBits(mData, offset, bytesPerScanline, stream);
   }
}


void PSD::Layer::Channel::loadRaw(int32_t width, int32_t height, std::istream& stream)
{
   mWidth = width;
   mData.resize(width * height);
   read(mData, stream);
}


void PSD::Layer::Channel::init(int32_t id, int32_t width, int32_t height)
{
   mID = id;
   mData.resize(width * height, 0xff);
}


uint8_t* PSD::Layer::Channel::getScanline(int32_t y) const
{
   return const_cast<uint8_t*>(mData.data() + y * mWidth);
}


// PSD Interface --------------------------------------------------------------


int32_t PSD::getWidth() const
{
   return mHeader.getWidth();
}


int32_t PSD::getHeight() const
{
   return mHeader.getHeight();
}


PSD::ColorFormat PSD::getColorFormat() const
{
    return mColorFormat;
}

void PSD::setColorFormat(const PSD::ColorFormat& colorFormat)
{
    mColorFormat = colorFormat;
}


size_t PSD::getLayerCount() const
{
   return mLayers.size();
}


const std::vector<PSD::Layer>& PSD::getLayers() const
{
   return mLayers;
}


const PSD::Layer& PSD::getLayer(int32_t index) const
{
   return mLayers[index];
}


std::vector<PSD::Layer>::const_iterator PSD::getLayer(const std::string& name) const
{
   auto it = std::find_if(
      mLayers.begin(),
      mLayers.end(),
      [name](const PSD::Layer& layer){return (layer.getName() == name);}
   );

   return it;
}


std::string PSD::loadString(std::istream& stream)
{
   uint8_t size = 0;
   read (size, stream);

   char* name= new char[size + 1];
   name[size]= 0;
   read(reinterpret_cast<unsigned char*>(name), size, stream);

   std::string str(name);
   delete[] name;

   return str;
}


void PSD::loadImageResourceSection(std::istream& stream)
{
   // Image Resources Section
   //
   // 4          Length of image resource section. The length may be zero.
   //
   // Variable   Image resources (Image Resource Blocks).

   // to ignore the whole resource block
   //
    int32_t length;
    read(length, stream);
    stream.ignore(length);
    return;

   // Image resource block
   //
   // 4           Signature: '8BIM'
   //
   // 2           Unique identifier for the resource. Image resource IDs contains a list
   //             of resource IDs used by Photoshop.
   //
   // Variable    Name: Pascal string, padded to make the size even (a null name consists of two bytes of 0)
   //
   // 4           Actual size of resource data that follows
   //
   // Variable    The resource data, described in the sections on the individual resource types.
   //             <!!!> It is padded to make the size even <!!!>

   int32_t totalSize = 0;
   read(totalSize, stream);
   auto sectionStart = stream.tellg();

   while (stream.tellg() < sectionStart + static_cast<std::streampos>(totalSize))
   {
      int32_t blockSignature = 0;
      read(blockSignature, stream);

      if (blockSignature == '8BIM')
      {
         uint16_t resourceIdentifier = 0;
         read(resourceIdentifier, stream);

         auto name = loadString(stream);
         // std::cout << "image resource: " << name << std::endl;

         int32_t blockSize = 0;
         read(blockSize, stream);

         auto blockStart = stream.tellg();

         // path
         if (resourceIdentifier >= 2000 && resourceIdentifier < 2999)
         {
             Path path(blockSize);
             path.load(stream, mHeader.getWidth(), mHeader.getHeight());
             path.setName(name);
             mPaths.push_back(path);
         }

         // handle odd block size
         if (blockSize & 1)
         {
            blockSize++;
         }

         // ignore the whole block, meh
         stream.ignore(blockSize);

         // ignore unprocessed block bytes
         const auto blockBytesRead = stream.tellg() - blockStart;
         const auto blockIgnoredBytes = blockSize - blockBytesRead;
         stream.ignore(blockIgnoredBytes);
      }
      else
      {
         std::cerr << "invalid block signature" << std::endl;
         exit(-1);
      }
   }

   // ignore unprocessed section bytes
   const auto sectionBytesRead = stream.tellg() - sectionStart;
   const auto sectionIgnoredBytes = totalSize - sectionBytesRead;
   stream.ignore(sectionIgnoredBytes);
}


void PSD::loadLayerAndMaskInformation(std::istream& stream)
{
   // https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/#50577409_pgfId-1031423

   // 4          Length of the layer and mask information section.
   //
   // Variable   Layer info (see See Layer info for details).
   //            -> see code below
   //
   // Variable   Global layer mask info
   //
   // Variable   Series of tagged blocks containing various types of data.

   [[maybe_unused]] int32_t total = 0;
   read(total, stream);

   // 4          Length of the layers info section, rounded up to a multiple of 2.
   //
   // 2          Layer count. If it is a negative number, its absolute value is the number of layers
   //            and the first alpha channel contains the transparency data for the merged result.
   //
   // Variable   Information about each layer. See Layer records describes the structure of this
   //            information for each layer.
   //            -> Layer records
   //
   // Variable   Channel image data. Contains one or more image data records for each layer.
   //            The layers are in the same order as in the layer information
   //            -> Channel image data

   [[maybe_unused]] int32_t length = 0;
   read(length, stream);

   int16_t layerCount = 0;
   read(layerCount, stream);
   layerCount = abs(layerCount);

   // load 'layer records'
   for (auto i = 0; i < layerCount; i++)
   {
      Layer layer;
      layer.setColorFormat(getColorFormat());
      layer.loadLayerRecords(stream);
      mLayers.push_back(layer);
   }

   // load 'channel image data'
   for (auto& layer : mLayers)
   {
      layer.loadChannelImageData(stream);
   }
}


void PSD::loadColorModeData(std::istream& stream)
{
    // Color Mode Data Section
    //
    // 4          The length of the following color data.
    // Variable   The color data.
    //
    // Only indexed color and duotone (see the mode field in the File header section) have color mode data.
    // For all other modes, this section is just the 4-byte length field, which is set to zero.

    int32_t length = 0;
    read(length, stream);
    stream.ignore(length);
}


void PSD::load(std::istream& stream)
{
   // assume big endian
   mHeader.load(stream);
   loadColorModeData(stream);
   loadImageResourceSection(stream);
   loadLayerAndMaskInformation(stream);
}


bool PSD::load(const std::string& filename)
{
   std::ifstream stream;

   stream.open(filename, std::ios::binary);
   load(stream);

   return stream.good();
}


// PSD Path -------------------------------------------------------------------

PSD::Path::Path(int32_t blockSize)
{
   mPathRecordCount = blockSize / 26;
}

void PSD::Path::Position::load(std::istream& stream, float invWidth, float invHeight)
{
   int32_t x, y;
   read(y, stream);
   read(x, stream);
   mY = y * invHeight;
   mX = x * invWidth;
}

const std::string& PSD::Path::getName() const
{
   return mName;
}

void PSD::Path::setName(const std::string& name)
{
   mName = name;
}

void PSD::Path::load(std::istream& stream, int32_t width, int32_t height)
{
   const auto invScale= 1.0f / (1 << 24);

   auto invWidth = width * invScale;
   auto invHeight = height * invScale;

   while (mPathRecordCount > 0)
   {
      auto recordStart = stream.tellg();

      // read first two bytes of record
      read(mRecordType, stream);

      // all subsequent records have 24 bytes left
      switch (mRecordType)
      {
         case Path::ClosedSubpathLengthRecord:
         case Path::OpenSubpathLengthRecord:
         {
            // these are followed by path data
            readPathRecord(stream);
            break;
         }

         case Path::ClosedSubpathBezierKnotLinked:
         case Path::ClosedSubpathBezierKnotUnlinked:
         case Path::OpenSubpathBezierKnotLinked:
         case Path::OpenSubpathBezierKnotUnlinked:
         {
            readBezierKnot(stream, invWidth, invHeight);
            break;
         }

         case Path::PathFillRuleRecord:
         {
            readFillRuleRecord(stream);
            break;
         }

         case Path::ClipboardRecord:
         {
            readClipboardRecord(stream);
            break;
         }

         case Path::InitialFillRuleRecord:
         {
            readInitialFill(stream);
            break;
         }
      }

      // default: skip rest of 26-byte block
      stream.ignore( (static_cast<int64_t>(recordStart) + 26) - static_cast<int64_t>(stream.tellg()) );

      mPathRecordCount--;
   }
}


void PSD::Path::readPathRecord(std::istream& stream)
{
   [[maybe_unused]] bool closedPath = (mRecordType == ClosedSubpathLengthRecord);

   // read number of records and then kthxbye
   uint16_t recordCount;
   read(recordCount, stream);

   for (auto i = 0; i < recordCount; i++)
   {
      mPositions.resize(recordCount);
   }

   mPositionCount= 0;
}


void PSD::Path::readBezierKnot(std::istream& stream, float invWidth, float invHeight)
{
   mPositions[mPositionCount].in.load(stream, invWidth, invHeight);
   mPositions[mPositionCount].pos.load(stream, invWidth, invHeight);
   mPositions[mPositionCount].out.load(stream, invWidth, invHeight);
   mPositionCount++;
}


void PSD::Path::readInitialFill(std::istream& stream)
{
   read(mInitialFill, stream);
   stream.ignore(22);
}


void PSD::Path::readFillRuleRecord(std::istream& stream)
{
   int32_t fill;
   read(fill, stream);
   mFill = (fill != 0);
   stream.ignore(22);
}

int PSD::Path::getPositionCount() const
{
   return mPositionCount;
}

const PSD::Path::Position &PSD::Path::getPosition(int index) const
{
   return mPositions[index].pos;
}

const PSD::Path::Position &PSD::Path::getTangentIn(int index) const
{
   return mPositions[index].in;
}

const PSD::Path::Position &PSD::Path::getTangentOut(int index) const
{
   return mPositions[index].out;
}


void PSD::Path::readClipboardRecord(std::istream& stream)
{
   // unsupported
   stream.ignore(24);
}

bool PSD::Path::isBesizer() const
{
   bool bezier = false;

   switch (mRecordType)
   {
      case ClosedSubpathBezierKnotLinked:
      case ClosedSubpathBezierKnotUnlinked:
      case OpenSubpathBezierKnotLinked:
      case OpenSubpathBezierKnotUnlinked:
         bezier = true;
         break;
      default:
         break;
   }

   return bezier;
}


