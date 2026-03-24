#pragma once

#include "image.h"

#include <stdint.h>
#include <array>
#include <memory>
#include <vector>

///
/// \brief Loads layered Adobe PSD files and exposes decoded layer image data.
///
class PSD
{
public:
   ///
   /// \brief Selects byte packing for output layer pixels.
   ///
   enum class ColorFormat
   {
      ABGR,
      ARGB
   };

   ///
   /// \brief Represents the PSD file header section.
   ///
   class Header
   {
   public:
      ///
      /// \brief Enumerates PSD color mode identifiers from the file header.
      ///
      enum class ColorMode
      {
         Bitmap = 0,
         Grayscale = 1,
         Indexed = 2,
         RGB = 3,
         CMYK = 4,
         Multichannel = 7,
         Duotone = 8,
         Lab = 9,
      };

      ///
      /// \brief Reads the PSD file header fields from the stream.
      /// \param stream Binary PSD input stream.
      ///
      void load(std::istream& stream);

      ///
      /// \brief Returns image width parsed from the PSD header.
      /// \return Width in pixels.
      ///
      int32_t getWidth() const;

      ///
      /// \brief Returns image height parsed from the PSD header.
      /// \return Height in pixels.
      ///
      int32_t getHeight() const;

   private:
      std::array<uint8_t, 4> _signature;
      uint16_t _version = 0u;
      std::array<uint8_t, 6> _reserved;
      uint16_t _channels = 0u;
      int32_t _height = 0;
      int32_t _width = 0;
      uint16_t _depth = 0u;
      uint16_t _mode = 0u;
   };

   ///
   /// \brief Represents one PSD layer record and its decoded channels.
   ///
   class Layer
   {
   public:
      ///
      /// \brief Marks special section-divider layer entries used for folder boundaries.
      ///
      enum class SectionDivider
      {
         None = -1,
         Any = 0,
         OpenFolder = 1,
         ClosedFolder = 2,
         BoundingSectionDivider = 3,
      };

      ///
      /// \brief Stores one layer channel and its decoded sample bytes.
      ///
      class Channel
      {
      public:
         Channel() = default;

         ///
         /// \brief Reads channel id and encoded data size from layer records.
         /// \param stream Binary PSD input stream.
         ///
         void load(std::istream& stream);

         ///
         /// \brief Initializes channel storage for a layer-sized pixel plane.
         /// \param id PSD channel id (for example rgb or alpha ids).
         /// \param width Layer width in pixels.
         /// \param height Layer height in pixels.
         ///
         void init(int16_t id, int32_t width, int32_t height);

         ///
         /// \brief Decodes RLE-compressed channel data into `_data`.
         /// \param width Layer width in pixels.
         /// \param height Layer height in pixels.
         /// \param stream Binary PSD input stream.
         ///
         void loadRLE(int32_t width, int32_t height, std::istream& stream);

         ///
         /// \brief Reads raw (uncompressed) channel bytes into `_data`.
         /// \param width Layer width in pixels.
         /// \param height Layer height in pixels.
         /// \param stream Binary PSD input stream.
         ///
         void loadRaw(int32_t width, int32_t height, std::istream& stream);

         ///
         /// \brief Expands one PackBits scanline into destination channel storage.
         /// \param dest Destination byte buffer.
         /// \param offset Start offset in destination.
         /// \param scanlineBytesLeft Encoded bytes remaining for this scanline.
         /// \param stream Binary PSD input stream.
         ///
         void unpackBits(std::vector<uint8_t>& dest, int32_t offset, size_t scanlineBytesLeft, std::istream& stream);

         ///
         /// \brief Returns pointer to scanline `y` inside decoded channel data.
         /// \param y Scanline index.
         /// \return Pointer to the first byte of scanline `y`.
         ///
         uint8_t* getScanline(int32_t y) const;

         ///
         /// \brief Returns PSD channel id.
         /// \return Channel id.
         ///
         short getID() const;

         ///
         /// \brief Returns full decoded channel byte storage.
         /// \return Channel byte vector.
         ///
         const std::vector<uint8_t>& data() const;

      private:
         int16_t _id = 0;
         int32_t _size = 0;
         int32_t _width = 0;
         std::vector<uint8_t> _data;
      };

      Layer() = default;

      ///
      /// \brief Reads one layer record including geometry, metadata blocks, and channels.
      /// \param stream Binary PSD input stream.
      ///
      void loadLayerRecords(std::istream& stream);

      ///
      /// \brief Reads channel image payload and builds merged layer image pixels.
      /// \param stream Binary PSD input stream.
      ///
      void loadChannelImageData(std::istream& stream);

      ///
      /// \brief Returns layer bottom coordinate.
      /// \return Bottom coordinate.
      ///
      int32_t getBottom() const;

      ///
      /// \brief Returns layer top coordinate.
      /// \return Top coordinate.
      ///
      int32_t getTop() const;

      ///
      /// \brief Returns layer left coordinate.
      /// \return Left coordinate.
      ///
      int32_t getLeft() const;

      ///
      /// \brief Returns layer width derived from right-left bounds.
      /// \return Layer width in pixels.
      ///
      int32_t getWidth() const;

      ///
      /// \brief Returns layer height derived from bottom-top bounds.
      /// \return Layer height in pixels.
      ///
      int32_t getHeight() const;

      ///
      /// \brief Sets layer opacity byte from PSD metadata.
      /// \param opacity Opacity in range 0..255.
      ///
      void setOpacity(uint8_t opacity);

      ///
      /// \brief Returns layer opacity byte.
      /// \return Opacity in range 0..255.
      ///
      int32_t getOpacity() const;

      ///
      /// \brief Returns visibility state derived from PSD layer flags.
      /// \return `true` if layer is visible.
      ///
      bool isVisible() const;

      ///
      /// \brief Sets visibility flag inside PSD layer flags.
      /// \param visible New visibility state.
      ///
      void setVisible(bool visible);

      ///
      /// \brief Returns layer name.
      /// \return Layer name string.
      ///
      const std::string& getName() const;

      ///
      /// \brief Offsets layer rectangle by `(x, y)`.
      /// \param x Horizontal delta.
      /// \param y Vertical delta.
      ///
      void move(int32_t x, int32_t y);

      ///
      /// \brief Moves layer horizontally while preserving width.
      /// \param x New left coordinate.
      ///
      void setX(int32_t x);

      ///
      /// \brief Moves layer vertically while preserving height.
      /// \param y New top coordinate.
      ///
      void setY(int32_t y);

      ///
      /// \brief Sets bottom coordinate directly.
      /// \param v New bottom coordinate.
      ///
      void setBottom(int32_t v);

      ///
      /// \brief Sets top coordinate directly.
      /// \param v New top coordinate.
      ///
      void setTop(int32_t v);

      ///
      /// \brief Returns decoded merged layer image.
      /// \return Layer image.
      ///
      const Image& getImage() const;

      ///
      /// \brief Returns channel by PSD channel id.
      /// \param id Channel id to find.
      /// \return Matching channel.
      ///
      const Channel& getChannel(int32_t id) const;

      ///
      /// \brief Returns parsed section-divider marker.
      /// \return Section-divider type.
      ///
      SectionDivider getSectionDivider() const;

      ///
      /// \brief Returns whether this entry is a section-divider layer.
      /// \return `true` if the layer is a divider/folder marker.
      ///
      bool isSectionDivider() const;

      ///
      /// \brief Returns whether this layer has drawable image content.
      /// \return `true` for non-divider layers with positive dimensions.
      ///
      bool isImageLayer() const;

      ///
      /// \brief Returns pixel packing used when composing layer image.
      /// \return Current color format.
      ///
      ColorFormat getColorFormat() const;

      ///
      /// \brief Sets pixel packing used when composing layer image.
      /// \param colorFormat Target output color format.
      ///
      void setColorFormat(const ColorFormat& colorFormat);

   private:
      int32_t _top = 0;
      int32_t _left = 0;
      int32_t _bottom = 0;
      int32_t _right = 0;
      uint16_t _channel_count = 0;
      Image _image;
      std::vector<Channel> _channels;
      std::array<uint8_t, 4> _blend_mode_signature{};
      std::array<uint8_t, 4> _blend_mode_key{};
      uint8_t _opacity = 0;
      uint8_t _clipping = 0;
      uint8_t _flags = 0;
      std::string _name;
      SectionDivider _section_divider = SectionDivider::None;
      ColorFormat _color_format = ColorFormat::ABGR;
   };

   ///
   /// \brief Represents one PSD vector path resource.
   ///
   class Path
   {
   public:
      ///
      /// \brief One path control-point position decoded to normalized pixel space.
      ///
      struct Position
      {
         float _x;
         float _y;

         ///
         /// \brief Reads fixed-point coordinates and scales them to normalized image space.
         /// \param stream Binary PSD input stream.
         /// \param inv_width Reciprocal image width.
         /// \param inv_height Reciprocal image height.
         ///
         void load(std::istream& stream, float inv_width, float inv_height);
      };

      ///
      /// \brief One bezier knot containing in-point, anchor, and out-point.
      ///
      struct BezierKey
      {
         Position in;
         Position pos;
         Position out;
      };

      ///
      /// \brief Identifies PSD path-record subtypes.
      ///
      enum PathSelector
      {
         ClosedSubpathLengthRecord = 0,
         ClosedSubpathBezierKnotLinked = 1,
         ClosedSubpathBezierKnotUnlinked = 2,
         OpenSubpathLengthRecord = 3,
         OpenSubpathBezierKnotLinked = 4,
         OpenSubpathBezierKnotUnlinked = 5,
         PathFillRuleRecord = 6,
         ClipboardRecord = 7,
         InitialFillRuleRecord = 8
      };

      ///
      /// \brief Creates a path parser for a resource block of `blockSize` bytes.
      /// \param blockSize Resource payload size in bytes.
      ///
      Path(int blockSize);

      ///
      /// \brief Loads path records from a PSD image-resource block.
      /// \param stream Binary PSD input stream.
      /// \param width Image width in pixels.
      /// \param height Image height in pixels.
      ///
      void load(std::istream& stream, int width, int height);

      ///
      /// \brief Reads one path-record header and updates internal counters.
      /// \param stream Binary PSD input stream.
      ///
      void readPathRecord(std::istream& stream);

      ///
      /// \brief Reads one bezier-knot record and appends it to path positions.
      /// \param stream Binary PSD input stream.
      /// \param width Reciprocal width scale.
      /// \param height Reciprocal height scale.
      ///
      void readBezierKnot(std::istream& stream, float width, float height);

      ///
      /// \brief Reads and skips clipboard path record payload.
      /// \param stream Binary PSD input stream.
      ///
      void readClipboardRecord(std::istream& stream);

      ///
      /// \brief Reads initial-fill flag record.
      /// \param stream Binary PSD input stream.
      ///
      void readInitialFill(std::istream& stream);

      ///
      /// \brief Reads fill-rule record and updates fill state.
      /// \param stream Binary PSD input stream.
      ///
      void readFillRuleRecord(std::istream& stream);

      ///
      /// \brief Returns number of parsed bezier positions.
      /// \return Position count.
      ///
      int getPositionCount() const;

      ///
      /// \brief Returns bezier anchor position at `index`.
      /// \param index Position index.
      /// \return Anchor position.
      ///
      const Position& getPosition(int index) const;

      ///
      /// \brief Returns incoming tangent position at `index`.
      /// \param index Position index.
      /// \return Incoming tangent position.
      ///
      const Position& getTangentIn(int index) const;

      ///
      /// \brief Returns outgoing tangent position at `index`.
      /// \param index Position index.
      /// \return Outgoing tangent position.
      ///
      const Position& getTangentOut(int index) const;

      ///
      /// \brief Returns whether this path stores bezier knot data.
      /// \return `true` if path positions were parsed.
      ///
      bool isBesizer() const;

      ///
      /// \brief Returns path name assigned from PSD resource metadata.
      /// \return Path name.
      ///
      const std::string& getName() const;

      ///
      /// \brief Sets path name.
      /// \param name New path name.
      ///
      void setName(const std::string& name);

   protected:
      int32_t _record_type = 0;
      int32_t _path_record_count = 0;
      int32_t _initial_fill = 0;
      bool _fill = false;
      std::string _name;
      int32_t _position_count = 0;
      std::vector<BezierKey> _positions;
   };

   PSD() = default;

   ///
   /// \brief Returns PSD document width.
   /// \return Width in pixels.
   ///
   int32_t getWidth() const;

   ///
   /// \brief Returns PSD document height.
   /// \return Height in pixels.
   ///
   int32_t getHeight() const;

   ///
   /// \brief Returns number of parsed layer entries.
   /// \return Layer count.
   ///
   size_t getLayerCount() const;

   ///
   /// \brief Returns current output color format used for layer composition.
   /// \return Color format.
   ///
   ColorFormat getColorFormat() const;

   ///
   /// \brief Sets output color format for subsequently decoded layers.
   /// \param colorFormat New output color format.
   ///
   void setColorFormat(const ColorFormat& colorFormat);

   ///
   /// \brief Returns all parsed layers.
   /// \return Layer list.
   ///
   const std::vector<Layer>& getLayers() const;

   ///
   /// \brief Returns layer at index.
   /// \param index Layer index.
   /// \return Layer reference.
   ///
   const Layer& getLayer(int32_t index) const;

   ///
   /// \brief Finds first layer by exact name.
   /// \param name Layer name to search.
   /// \return Iterator to matching layer or `end()` if not found.
   ///
   std::vector<Layer>::const_iterator getLayer(const std::string& name) const;

   ///
   /// \brief Opens and loads a PSD file.
   /// \param filename PSD file path.
   /// \return `true` on successful load.
   ///
   bool load(const std::string& filename);

   ///
   /// \brief Loads PSD content from an already opened stream.
   /// \param stream Binary PSD input stream.
   ///
   void load(std::istream& stream);

   ///
   /// \brief Reads a Pascal-style padded PSD string.
   /// \param stream Binary PSD input stream.
   /// \return Decoded string.
   ///
   static std::string loadString(std::istream&);

private:
   ///
   /// \brief Reads and skips color-mode section payload.
   /// \param stream Binary PSD input stream.
   ///
   void loadColorModeData(std::istream& stream);

   ///
   /// \brief Reads image-resource section and extracts known path/name blocks.
   /// \param stream Binary PSD input stream.
   ///
   void loadImageResourceSection(std::istream& stream);

   ///
   /// \brief Reads layer/mask section and decodes all layer records.
   /// \param stream Binary PSD input stream.
   ///
   void loadLayerAndMaskInformation(std::istream& stream);

   ColorFormat _color_format = ColorFormat::ARGB;
   Header _header;
   std::vector<Layer> _layers;
   std::vector<Path> _paths;
};
