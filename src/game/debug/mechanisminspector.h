#pragma once

#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)

#include <SFML/Graphics.hpp>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "game/mechanisms/gamemechanismschema.h"
#include "game/mechanisms/mechanismcondition.h"

class GameMechanism;
class Level;
struct TmxObject;

/// \brief mechanism property editing in an explicitly opened development window.
class MechanismInspector
{
public:
   MechanismInspector();
   ~MechanismInspector();
   void processEvents();
   void draw(const std::shared_ptr<Level>& level);
   [[nodiscard]] bool isOpen() const;
   [[nodiscard]] bool isPicking() const;
   void selectAt(const std::optional<sf::Vector2f>& position, sf::Vector2u image_size, sf::Vector2i image_pixel);
   void discardPreviews(const std::shared_ptr<Level>& active_level);

private:
   struct Condition
   {
      std::string label;
      std::optional<MechanismCondition> condition;
   };

   struct PropertyEdit
   {
      const PropertyInfo* schema{nullptr};
      std::vector<char> buffer;
      std::optional<std::string> original_live;
      std::vector<std::string> original_parts;
      std::string initial_text;
      bool mixed{false};
      bool changed{false};
      bool valid{true};
   };

   struct Entry
   {
      std::string group;
      std::string name;
      std::string type;
      std::weak_ptr<GameMechanism> mechanism;
      std::shared_ptr<TmxObject> source;
      bool layer{false};
      std::vector<std::weak_ptr<GameMechanism>> parts;
      std::optional<std::string> liveValue(const std::string& name) const;
      std::vector<std::string> liveValues(const std::string& name) const;
      bool setLiveValue(const std::string& name, const std::string& value);
      std::vector<std::string> targets;
      std::vector<Condition> conditions;
      const MechanismSchema* schema{nullptr};
      std::vector<PropertyEdit> properties;
      bool properties_initialized{false};
      std::string property_message;
   };

   void refresh(const std::shared_ptr<Level>& level);
   void drawDetails(Entry& entry);
   void drawReference(const std::string& name);
   void followReference(size_t index);
   void drawProperties(Entry& entry);
   void discard(Entry& entry);
   void saveProperties();
   bool renderedAt(const Entry& entry, sf::Vector2i pixel, const sf::View& view);
   [[nodiscard]] std::optional<sf::FloatRect> bounds(const Entry& entry) const;

   std::unique_ptr<sf::RenderWindow> _window;
   std::unique_ptr<sf::RenderTexture> _pick_color;
   std::unique_ptr<sf::RenderTexture> _pick_normal;
   sf::Clock _clock;
   std::weak_ptr<Level> _level;
   std::string _tmx_filename;
   std::string _source_text;
   std::vector<Entry> _entries;
   std::optional<size_t> _selected;
   std::vector<size_t> _picked;
   std::optional<sf::Vector2f> _position_filter;
   std::array<char, 256> _filter{};
   bool _picking{false};
   bool _initialized{false};
   bool _scroll_to_selected{false};
   std::string _selection_status;
};

#endif
