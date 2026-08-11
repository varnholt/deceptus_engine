#include "grabrope.h"

#include "framework/tools/sfmlcompat.h"
#include "game/constants.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
// how much a metre of rope weighs. the player is carried by the anchor limit rather than by the chain,
// so this only has to be heavy enough that he cannot fling the rope around.
constexpr auto rope_linear_density = 0.5f;

// how far off the chain the player may be and still grab it
constexpr auto grab_tolerance_px = 8.0f;

// same values the harpoon gives its rope segments
constexpr auto chain_link_linear_damping = 0.1f;
constexpr auto chain_link_angular_damping = 0.1f;

// the chain must not collide with the player: his solid fixtures collide with CategoryBoundary,
// CategoryEnemyCollideWith and CategoryMoveableBox, his sensors additionally with
// CategoryEnemyWalkThrough. CategoryNoCastShadow appears in none of those masks, so the chain only ever
// touches the level geometry - the same filter the harpoon rope uses.
constexpr uint16_t chain_category_bits = CategoryNoCastShadow;
constexpr uint16_t chain_mask_bits = CategoryBoundary;

static constexpr std::array grab_rope_properties{
   PropertyInfo{.name = "z", .type = "int", .default_value = int32_t{20}},
};
static constexpr MechanismSchema grab_rope_schema{
   .type_name = "GrabRope",
   .layer_name = "grab_ropes",
   .default_width = 24,
   .default_height = 96,
   .properties = grab_rope_properties,
};
const auto registered_grabrope = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.registerSchema(grab_rope_schema);

   registry.mapGroupToLayer("GrabRope", "grab_ropes");

   // grab ropes are stored in the shared rope group, the same way ropes with light are
   registry.registerLayerName(
      "grab_ropes",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<GrabRope>(parent);
         mechanism->setup(data);
         mechanisms["ropes"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "GrabRope",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<GrabRope>(parent);
         mechanism->setup(data);
         mechanisms["ropes"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

GrabRope::GrabRope(GameNode* parent) : Rope(parent)
{
   setClassName(typeid(GrabRope).name());
}

std::string_view GrabRope::objectName() const
{
   return "GrabRope";
}

void GrabRope::setup(const GameDeserializeData& data)
{
   Rope::setup(data);

   // Rope leaves its chain as sensors, which is fine for something that is only looked at: it hangs
   // through the level geometry and nobody notices. a rope the player hangs on is looked at closely and
   // has to lie against the rock it is next to instead of sweeping through it. the filter is the one the
   // harpoon rope uses: CategoryNoCastShadow appears in no player or enemy mask, so the chain only ever
   // touches level geometry - which is also what lets it wrap around a corner.
   makeChainCarryable();

   for (auto element_index = 0u; element_index < _chain_elements.size(); element_index++)
   {
      for (auto* fixture = _chain_elements[element_index]->GetFixtureList(); fixture != nullptr; fixture = fixture->GetNext())
      {
         b2Filter filter;
         filter.categoryBits = chain_category_bits;

         // the topmost link sits in whatever the rope is bolted to; letting it collide would push the
         // whole chain back out of that surface
         filter.maskBits = (element_index > 0) ? chain_mask_bits : 0;

         fixture->SetSensor(false);
         fixture->SetFilterData(filter);
      }
   }
}

const std::vector<b2Body*>& GrabRope::getChainElements() const
{
   return _chain_elements;
}

float GrabRope::getSegmentLength() const
{
   return _segment_length_m;
}

b2Vec2 GrabRope::getAnchorLocalPosition() const
{
   const auto position_px = getPixelPosition();
   return b2Vec2{static_cast<float>(position_px.x) * MPP, static_cast<float>(position_px.y) * MPP};
}

bool GrabRope::isWithinGrabRange(const sf::FloatRect& rect_px) const
{
   // one segment at a time: the box around a whole swinging chain covers a lot of empty air
   for (auto element_index = 0u; element_index + 1 < _chain_elements.size(); element_index++)
   {
      const auto from_m = _chain_elements[element_index]->GetPosition();
      const auto to_m = _chain_elements[element_index + 1]->GetPosition();

      // the tolerance is applied to the segment rather than to the player: a segment of a rope hanging
      // straight down has no width at all, and a rect without area intersects nothing
      const auto segment_rect_px = sf::FloatRect{
         {(std::min(from_m.x, to_m.x) * PPM) - grab_tolerance_px, (std::min(from_m.y, to_m.y) * PPM) - grab_tolerance_px},
         {(std::fabs(to_m.x - from_m.x) * PPM) + 2.0f * grab_tolerance_px, (std::fabs(to_m.y - from_m.y) * PPM) + 2.0f * grab_tolerance_px}
      };

      // a segment is short enough that its own bounding box is a fair stand-in for the segment itself
      if (sfcompat::findIntersection(rect_px, segment_rect_px).has_value())
      {
         return true;
      }
   }

   return false;
}

void GrabRope::makeChainCarryable()
{
   // Rope spaces its links a segment apart but gives every one of them the same fixed 0.025 m collision
   // box, so a link's rotational inertia has nothing to do with the joint arm it swings on - roughly
   // 1e-6 against 0.19 m here. that is stable enough for a short decorative rope and comes apart over
   // seconds for a long one: measured at rest, a 20 link chain drifted to twice its own length. it also
   // puts the centre of mass exactly on the link's own pivot:
   //
   //     joint  o---- link body origin, and the default centre of mass, sit here
   //            |     -> gravity acts on the pivot, produces no torque about it, so a slack link's
   //            |        rotation is unconstrained and points wherever momentum left it. that rotation
   //     next   o        decides where the next joint goes, so a loose chain folds into a knot.
   //     joint
   //
   // both are fixed by describing each link as what it actually represents: a rod one segment long,
   // hanging from its own pivot. b2MassData::I is measured about the local origin, which is the pivot,
   // and for a rod about its end that is mass * length^2 / 3.
   const auto link_mass = rope_linear_density * _segment_length_m;

   for (auto* chain_element : _chain_elements)
   {
      b2MassData mass_data;
      mass_data.mass = link_mass;
      mass_data.center = b2Vec2{0.0f, _segment_length_m * 0.5f};
      mass_data.I = link_mass * _segment_length_m * _segment_length_m / 3.0f;
      chain_element->SetMassData(&mass_data);

      // without damping the slack tail keeps swinging on its own long after the player stopped moving
      chain_element->SetLinearDamping(chain_link_linear_damping);
      chain_element->SetAngularDamping(chain_link_angular_damping);
   }
}
