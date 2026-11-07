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
   /// \brief creates the editor window and initializes its independent ImGui context.
   MechanismInspector();
   /// \brief releases the editor context and off-screen picking targets.
   ~MechanismInspector();
   /// \brief forwards window events to ImGui and handles close/F6 requests.
   void processEvents();
   /// \brief refreshes source metadata when the level changes and draws the editor.
   /// \param level active, fully loaded level, or null while no level can be edited.
   void draw(const std::shared_ptr<Level>& level);
   /// \brief checks whether the editor initialized successfully and its window remains open.
   /// \return true when the editor can process input and draw.
   [[nodiscard]] bool isOpen() const;
   /// \brief checks whether a game-image click is explicitly requested.
   /// \return true when the open editor is waiting for a picking click.
   [[nodiscard]] bool isPicking() const;
   /// \brief filters to all sources covering the clicked pixel without automatically selecting one.
   /// \param position world position of the click, or null when it falls outside the game image.
   /// \param image_size game-image dimensions before final window placement.
   /// \param image_pixel exact clicked pixel within that image, using a top-left origin.
   /// \note visual sampling renders only for this explicit request and does not advance simulation.
   void selectAt(const std::optional<sf::Vector2f>& position, sf::Vector2u image_size, sf::Vector2i image_pixel);
   /// \brief restores unsaved property values only when the edited level is still active.
   /// \param active_level currently loaded level, or null to avoid touching an unloaded instance.
   /// \note gameplay consequences of previous edits are not rewound.
   void discardPreviews(const std::shared_ptr<Level>& active_level);

private:
   /// \brief pairs a displayed requirement with its parsed condition when parsing succeeds.
   struct Condition
   {
      std::string label;                            //!< display label containing the source property and requirement.
      std::optional<MechanismCondition> condition;  //!< parsed requirement, absent when invalid.
   };

   /// \brief owns a text draft and per-part baselines for one schema property.
   struct PropertyEdit
   {
      const PropertyInfo* schema{nullptr};      //!< non-owning definition from the registered schema.
      std::vector<char> buffer;                 //!< null-terminated editor buffer owned by this draft.
      std::vector<std::string> original_parts;  //!< individual live values restored when the draft is discarded.
      std::string initial_text;                 //!< text baseline used to detect changes.
      bool mixed{false};                        //!< whether runtime parts started with different values.
      bool changed{false};                      //!< whether this property has a pending authored override.
      bool valid{true};                         //!< whether the last attempted live edit was accepted.
   };

   /// \brief associates one authored source with its runtime parts, schema, links, and drafts.
   struct Entry
   {
      std::string group;                       //!< runtime registry group, or schema layer for source-only entries.
      std::string name;                        //!< logical source name used by scripts and state lookups.
      std::string type;                        //!< resolved authoring schema type or runtime type label.
      std::weak_ptr<GameMechanism> mechanism;  //!< representative runtime part for state inspection.
      std::shared_ptr<TmxObject> source;       //!< retained source snapshot, including a synthetic object for tile layers.
      bool layer{false};                       //!< whether the source id belongs to the layer namespace.
      std::vector<std::weak_ptr<GameMechanism>>
         parts;  //!< runtime instances sharing the authoring source, without extending their lifetimes.
      /// \brief reads a representative value when every runtime part exposes the property.
      /// \param name authored property name to look up.
      /// \return the first part's value, or null if any part is missing or unbound.
      [[nodiscard]] std::optional<std::string> liveValue(const std::string& name) const;
      /// \brief reads all runtime values while preserving the order of parts.
      /// \param name authored property name to look up.
      /// \return per-part values, or an empty vector if any part cannot provide a value.
      [[nodiscard]] std::vector<std::string> liveValues(const std::string& name) const;
      /// \brief validates every part before applying an edit, restoring earlier values if application fails.
      /// \param name authored property name to update.
      /// \param value candidate text in the property's Tiled representation.
      /// \return true when every runtime part accepts the edit.
      bool setLiveValue(const std::string& name, const std::string& value);
      std::vector<std::string> targets;        //!< explicit named targets from source properties.
      std::vector<Condition> conditions;       //!< parsed source condition requirements.
      const MechanismSchema* schema{nullptr};  //!< non-owning registered schema defining the editable fields.
      std::vector<PropertyEdit> properties;    //!< drafts retained across selection changes.
      bool properties_initialized{false};      //!< whether schema defaults and live baselines have been loaded.
      std::string property_message;            //!< feedback about a rejected property edit.
   };

   /// \brief discards drafts and rebuilds entries from the loaded authoring sources and runtime registry.
   /// \param level new active level, or null to clear the editor.
   void refresh(const std::shared_ptr<Level>& level);
   /// \brief draws runtime state, source details, references, and editable property controls.
   /// \param entry selected authoring source and its runtime parts.
   void drawDetails(Entry& entry);
   /// \brief draws navigation buttons for matching named sources, or a missing-target message.
   /// \param name logical target id stored in a Tiled reference property.
   void drawReference(const std::string& name);
   /// \brief selects a referenced entry, clears filters, and requests scrolling it into view.
   /// \param index valid index in the current entry collection.
   void followReference(size_t index);
   /// \brief initializes drafts as needed and applies explicit live edits through shared bindings.
   /// \param entry source whose schema controls are being displayed.
   void drawProperties(Entry& entry);
   /// \brief restores each runtime part's unsaved baseline and removes the source's drafts.
   /// \param entry source whose property edits should be discarded.
   void discard(Entry& entry);
   /// \brief saves all changed sources and advances draft baselines only after a successful map write.
   void saveProperties();
   /// \brief samples each runtime part independently so overlapping graphics remain pickable.
   /// \param entry source whose runtime graphics should be tested.
   /// \param pixel game-image pixel with a top-left origin.
   /// \param view game camera with a scissor region covering the sampled pixel.
   /// \return true if any runtime part contributes nonzero alpha at the pixel.
   bool renderedAt(const Entry& entry, sf::Vector2i pixel, const sf::View& view);
   /// \brief combines runtime interaction bounds, falling back to authoring geometry when unavailable.
   /// \param entry source whose bounds are requested.
   /// \return world-space bounds, or null when neither runtime nor source geometry is available.
   [[nodiscard]] std::optional<sf::FloatRect> bounds(const Entry& entry) const;

   std::unique_ptr<sf::RenderWindow> _window;        //!< independent editor window.
   std::unique_ptr<sf::RenderTexture> _pick_color;   //!< color sampling target allocated only for explicit picks.
   std::unique_ptr<sf::RenderTexture> _pick_normal;  //!< companion normal target used by the existing draw interface.
   sf::Clock _clock;                                 //!< elapsed time between editor UI frames.
   std::weak_ptr<Level> _level;                      //!< edited level without extending its lifetime.
   std::string _tmx_filename;                        //!< local map path used for source writes.
   std::string _source_text;                         //!< last loaded or successfully written map bytes for conflict detection.
   std::vector<Entry> _entries;                      //!< source-oriented mechanism catalogue.
   std::optional<size_t> _selected;                  //!< selected entry index, if any.
   std::vector<size_t> _picked;                      //!< entry indices passing the current position filter.
   std::optional<sf::Vector2f> _position_filter;     //!< last valid picking position in world coordinates.
   std::array<char, 256> _filter{};                  //!< null-terminated text search buffer.
   bool _picking{false};                             //!< whether the next valid game click should filter sources.
   bool _initialized{false};                         //!< whether ImGui initialization succeeded.
   bool _scroll_to_selected{false};                  //!< whether reference navigation should scroll to the selected entry.
   std::string _selection_status;                    //!< picking and save feedback shown above the source list.
};

#endif
