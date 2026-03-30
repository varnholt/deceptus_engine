#pragma once

#include "tmxelement.h"

struct TmxAnimation;
struct TmxObjectGroup;

///
/// \brief Represents one TMX tileset tile definition.
///
struct TmxTile : TmxElement
{
   ///
   /// \brief Constructs an empty tile descriptor.
   ///
   TmxTile() = default;

   ///
   /// \brief Parses tile attributes and optional animation/objectgroup children.
   /// \param e XML element for `<tile>`.
   /// \param parse_data Shared TMX parse context.
   ///
   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>& parse_data) override;

   int32_t _id = 0;

   std::shared_ptr<TmxAnimation> _animation = nullptr;
   std::shared_ptr<TmxObjectGroup> _object_group = nullptr;
};
