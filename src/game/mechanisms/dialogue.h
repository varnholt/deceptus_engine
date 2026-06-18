#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "game/constants.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxObject;

/// \brief triggers a sequence of message boxes when the player enters a dialogue area.
class Dialogue : public GameMechanism, public GameNode
{
public:
   /// \brief stores one dialogue page with text, colors, placement, and typing settings.
   struct DialogueItem
   {
      std::string _title;
      std::string _message;
      sf::Color _text_color = sf::Color{232, 219, 243};
      sf::Color _background_color = sf::Color{47, 12, 75};
      MessageBoxLocation _location = MessageBoxLocation::MiddleCenter;
      std::optional<sf::Vector2f> _pos;
      bool _animate_text{true};
      float _animate_text_speed{30.0f};
   };

   /// \brief creates a dialogue mechanism with no configured pages.
   /// \param parent parent node in the scene graph.
   Dialogue(GameNode* parent = nullptr);

   /// \brief returns the mechanism registry name.
   /// \return string view containing `Dialogue`.
   std::string_view objectName() const override;

   /// \brief builds a dialogue from TMX properties and numbered message entries.
   /// \param parent parent node that owns the created mechanism.
   /// \param data deserialize context with TMX object, rectangle, and properties.
   /// \return created dialogue instance, or `nullptr` when the TMX object has no properties.
   static std::shared_ptr<Dialogue> deserialize(GameNode* parent, const GameDeserializeData& data);

   /// \brief checks activation conditions and opens the first page when triggered.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the trigger rectangle used for player intersection tests.
   /// \return dialogue trigger rectangle in pixels.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief reports whether this dialogue is currently marked as active.
   /// \return true when the dialogue is active and waiting for completion or reset.
   bool isActive() const;

   /// \brief marks the dialogue as active or inactive.
   /// \param active true to mark as active.
   void setActive(bool active);

   /// \brief shows the next page and finalizes the sequence when the last page was shown.
   /// \param text_color optional text color override; nullopt uses the per-page TMX color.
   /// \param background_color optional background color override; nullopt uses the per-page TMX color.
   /// \brief replaces the full set of dialogue pages with the supplied items.
   /// \param items pages to show in order; replaces any previously deserialized items.
   void setItems(std::vector<DialogueItem> items);

   /// \brief shows the next page and finalizes the sequence when the last page was shown.
   void showNext();

private:
   /// \brief replaces runtime tags like `<player>` and `<br>` inside dialogue text.
   /// \param str message text to modify in place.
   void replaceTags(std::string& str);

   /// \brief replaces every occurrence of one token inside a string.
   /// \param str text to modify in place.
   /// \param what token to search for.
   /// \param with replacement text.
   void replace(std::string& str, const std::string& what, const std::string& with);

   std::vector<DialogueItem> _dialogue_items;
   uint32_t _index = 0;

   sf::FloatRect _pixel_rect;
   bool _active{false};
   bool _button_required{true};
   bool _pause_game{true};
   bool _open_on_intersect{true};
   std::optional<std::chrono::milliseconds> _show_delay_ms;
   std::optional<int32_t> _consumed_counter;
   sf::Time _elapsed;
};
