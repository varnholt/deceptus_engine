#pragma once

#include "tmxelement.h"

#include "SFML/Graphics.hpp"

///
/// \brief Represents one infinite-map TMX data chunk.
///
struct TmxChunk : TmxElement
{
   ///
   /// \brief Constructs an empty chunk.
   ///
   TmxChunk() = default;
   ///
   /// \brief Releases the heap-allocated tile data array.
   ///
   ~TmxChunk() override;

   ///
   /// \brief Parses chunk bounds and CSV tile data.
   /// \param element XML element for `<chunk>`.
   /// \param parse_data Shared TMX parse context.
   ///
   void deserialize(tinyxml2::XMLElement* element, const std::shared_ptr<TmxParseData>& parse_data) override;
   std::vector<sf::Vector2f> _polyline;
   int32_t _x_px = 0;
   int32_t _y_px = 0;
   int32_t _width_px = 0;
   int32_t _height_px = 0;
   int32_t* _data = nullptr;
};
