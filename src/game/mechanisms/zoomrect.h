#ifndef ZOOMRECT_H
#define ZOOMRECT_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

/// \brief interpolates camera zoom based on player distance from a zone center.
class ZoomRect : public GameMechanism, public GameNode
{
public:
   /// \brief zoom normalization mode metadata for this mechanism.
   enum class Normalization
   {
      None,
      Linear
   };

   /// \brief mapping entry from normalized radius to camera zoom factor.
   struct ZoomFactor
   {
      float _radius{1.0f};
      float _factor{1.0f};
   };

   /// \brief creates a zoom rectangle mechanism.
   /// \param parent owning game node in the scene graph.
   ZoomRect(GameNode* parent);
   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "ZoomRect".
   std::string_view objectName() const override;

   /// \brief updates camera zoom from player position while inside the rectangle.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;
   /// \brief initializes area bounds and radius-to-zoom mapping from tmx properties.
   /// \param data deserialization data with zoom curve values.
   void setup(const GameDeserializeData& data);

   /// \brief returns the zoom activation rectangle in pixel space.
   /// \return area that controls when this mechanism affects camera zoom.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

private:
   Normalization _normalization{Normalization::Linear};
   sf::FloatRect _rect_px;
   sf::Vector2f _center_px;
   float _radius_px{0.0};
   std::vector<ZoomFactor> _zoom_factors;
   bool _within_rect_in_previous_frame{false};
   int32_t _instance_id{0};
};

#endif  // ZOOMRECT_H
