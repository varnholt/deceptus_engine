#pragma once

#include "image.h"

#include <array>
#include <memory>
#include <stdint.h>
#include <vector>

class PSD
{
   public:

      enum class ColorFormat {
         ABGR,
         ARGB
      };

      class Header
      {
         public:

            enum class ColorMode
            {
               Bitmap       = 0,
               Grayscale    = 1,
               Indexed      = 2,
               RGB          = 3,
               CMYK         = 4,
               Multichannel = 7,
               Duotone      = 8,
               Lab          = 9,
            };

            void load(std::istream& stream);
            int32_t getWidth() const;
            int32_t getHeight() const;

         private:
            std::array<uint8_t, 4> mSignature;
            uint16_t mVersion;
            std::array<uint8_t, 6> mReserved;
            uint16_t mChannels;
            int32_t mHeight;
            int32_t mWidth;
            uint16_t mDepth;
            uint16_t mMode;
      };

      class Layer
      {
         public:

            enum class SectionDivider{
               None                   = -1,
               Any                    = 0,
               OpenFolder             = 1,
               ClosedFolder           = 2,
               BoundingSectionDivider = 3,
            };

            class Channel
            {
               public:
                  Channel() = default;
                  void load(std::istream& stream);
                  void init(int32_t id, int32_t width, int32_t height);
                  void loadRLE(int32_t width, int32_t height, std::istream& stream);
                  void loadRaw(int32_t width, int32_t height, std::istream &stream);
                  void unpackBits(std::vector<uint8_t>& dest, int32_t offset, size_t scanlineBytesLeft, std::istream& stream);
                  uint8_t* getScanline(int32_t y) const;

                  short getID() const;
                  const std::vector<uint8_t>& data() const;


            private:
                  int16_t mID = 0;
                  int32_t mSize = 0;
                  int32_t mWidth = 0;
                  std::vector<uint8_t> mData;
            };

            Layer() = default;

            void loadLayerRecords(std::istream&);
            void loadChannelImageData(std::istream &stream);

            int32_t getBottom() const;
            int32_t getTop() const;
            int32_t getLeft() const;
            int32_t getWidth() const;
            int32_t getHeight() const;
            void setOpacity(int32_t opacity);
            int32_t getOpacity() const;
            bool isVisible() const;
            void setVisible(bool visible);
            const std::string& getName() const;
            void move(int32_t x, int32_t y);
            void setX(int32_t x);
            void setY(int32_t y);
            void setBottom(int32_t v);
            void setTop(int32_t v);
            const Image& getImage() const;
            const Channel& getChannel(int32_t id) const;
            SectionDivider getSectionDivider() const;
            ColorFormat getColorFormat() const;
            void setColorFormat(const ColorFormat& colorFormat);

         private:

            int32_t mTop = 0;
            int32_t mLeft = 0;
            int32_t mBottom = 0;
            int32_t mRight = 0;
            uint16_t mChannelCount = 0;
            Image mImage;
            std::vector<Channel> mChannels;
            std::array<uint8_t, 4> mBlendModeSignature;
            std::array<uint8_t, 4> mBlendModeKey;
            uint8_t mOpacity = 0;
            uint8_t mClipping = 0;
            uint8_t mFlags = 0;
            std::string mName;
            SectionDivider mSectionDivider = SectionDivider::None;
            ColorFormat mColorFormat = ColorFormat::ABGR;
      };

      class Path
      {
         public:
            struct Position
            {
               float mX;
               float mY;

               void load(std::istream& stream, float invWidth, float invHeight);
            };

            struct BezierKey
            {
               Position in;
               Position pos;
               Position out;
            };

            enum PathSelector
            {
               ClosedSubpathLengthRecord       = 0,
               ClosedSubpathBezierKnotLinked   = 1,
               ClosedSubpathBezierKnotUnlinked = 2,
               OpenSubpathLengthRecord         = 3,
               OpenSubpathBezierKnotLinked     = 4,
               OpenSubpathBezierKnotUnlinked   = 5,
               PathFillRuleRecord              = 6,
               ClipboardRecord                 = 7,
               InitialFillRuleRecord           = 8
            };

            Path(int blockSize);

            void load(std::istream& stream, int width, int height);
            void readPathRecord(std::istream& stream);
            void readBezierKnot(std::istream& stream, float width, float height);
            void readClipboardRecord(std::istream& stream);
            void readInitialFill(std::istream &stream);
            void readFillRuleRecord(std::istream& stream);

            int getPositionCount() const;
            const Position& getPosition(int index) const;
            const Position& getTangentIn(int index) const;
            const Position& getTangentOut(int index) const;
            bool isBesizer() const;
            const std::string& getName() const;
            void setName(const std::string& name);

         protected:
            int32_t mRecordType = 0;
            int32_t mPathRecordCount = 0;
            int32_t mInitialFill = 0;
            bool mFill = false;
            std::string mName;
            int32_t mPositionCount = 0;
            std::vector<BezierKey> mPositions;
      };

      PSD() = default;

      int32_t getWidth() const;
      int32_t getHeight() const;
      size_t getLayerCount() const;

      ColorFormat getColorFormat() const;
      void setColorFormat(const ColorFormat& colorFormat);

      const std::vector<Layer>& getLayers() const;
      const Layer& getLayer(int32_t index) const;
      std::vector<Layer>::const_iterator getLayer(const std::string& name) const;

      bool load(const std::string& filename);
      void load(std::istream &stream);

      static std::string loadString(std::istream&);

   private:
      void loadColorModeData(std::istream& stream);
      void loadImageResourceSection(std::istream& stream);
      void loadLayerAndMaskInformation(std::istream& stream);

      ColorFormat mColorFormat = ColorFormat::ARGB;
      Header mHeader;
      std::vector<Layer> mLayers;
      std::vector<Path> mPaths;
};

