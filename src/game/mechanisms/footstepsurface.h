#ifndef FOOTSTEPSURFACE_H
#define FOOTSTEPSURFACE_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <string>

/// \brief marks a region of the level whose ground the player's footsteps sound different on.
/// \note deliberately does not call addChunks: the mechanism holds no state to update and nothing to draw,
///       it is only ever queried for the surface below the player.
class FootstepSurface : public GameMechanism, public GameNode
{
public:
   /// \brief creates a footstep surface region.
   /// \param parent owning game node in the scene graph.
   FootstepSurface(GameNode* parent);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "FootstepSurface".
   std::string_view objectName() const override;

   /// \brief creates a footstep surface region from tmx properties.
   /// \param parent owning game node in the scene graph.
   /// \param data deserialization data with the bounds and the surface property.
   /// \return configured footstep surface instance.
   static std::shared_ptr<FootstepSurface> deserialize(GameNode* parent, const GameDeserializeData& data);

   /// \brief returns the region bounds in pixel space.
   /// \return rectangle the surface applies inside of.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief returns the surface the footstep samples are looked up with.
   /// \return surface identifier as written in the tmx object, empty when the property was missing.
   const std::string& getSurface() const;

private:
   sf::FloatRect _rect_px;
   std::string _surface;  //!< key into the surface definitions in data/config/footsteps.json
};

#endif  // FOOTSTEPSURFACE_H
