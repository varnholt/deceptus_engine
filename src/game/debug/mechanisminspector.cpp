#include "mechanisminspector.h"

#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)

#include <SFML/OpenGL.hpp>
#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"

#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "framework/tmxparser/tmxpolygon.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "framework/tools/stringutils.h"
#include "game/debug/debugdraw.h"
#include "game/debug/mechanismsourceeditor.h"
#include "game/level/gamenode.h"
#include "game/level/level.h"
#include "game/mechanisms/gamemechanism.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/mechanisms/interactioninterface.h"
#include "game/state/savestate.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <format>
#include <fstream>
#include <iterator>
#include <ranges>
#include <tuple>

namespace
{
/// \brief returns a lowercase copy for case-insensitive source searches.
/// \param text owned search text to transform.
/// \return transformed text.
std::string lower(std::string text)
{
   std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
   return text;
}

/// \brief reads explicit named target links from the authoring properties.
/// \param object source whose outgoing links should be listed.
/// \return nonempty target ids in source-property traversal order.
std::vector<std::string> targets(const TmxObject& object)
{
   std::vector<std::string> result;
   if (!object._properties)
   {
      return result;
   }
   for (const auto& [key, value] : object._properties->_map)
   {
      if (key == "target_id" || key == "target_ids" || key == "reference_id" || key == "reference")
      {
         for (const auto& id : StringUtils::split(value->toString(), ';'))
         {
            const auto name = StringUtils::trim(id);
            if (!name.empty())
            {
               result.push_back(name);
            }
         }
      }
   }
   return result;
}

/// \brief renders one instance independently and samples its alpha at an exact game-image pixel.
/// \param mechanism runtime instance to draw without advancing its simulation.
/// \param color off-screen color target, cleared before the draw.
/// \param normal companion normal target required by the mechanism draw interface.
/// \param pixel top-left-origin image coordinate to sample.
/// \param view camera and scissor region matching the clicked game image.
/// \return true when the instance contributes nonzero alpha at the pixel.
bool pixelHit(GameMechanism& mechanism, sf::RenderTexture& color, sf::RenderTexture& normal, sf::Vector2i pixel, const sf::View& view)
{
   color.setView(view);
   normal.setView(view);
   color.clear(sf::Color::Transparent);
   normal.clear(sf::Color::Transparent);
   mechanism.draw(color, normal);
   color.display();
   if (!color.setActive(true))
   {
      return false;
   }
   std::array<GLubyte, 4> sample{};
   glReadPixels(pixel.x, static_cast<GLint>(color.getSize().y) - pixel.y - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, sample.data());
   return sample[3] != 0;
}

/// \brief draws an expandable serialized-state panel or an unavailable-state explanation.
/// \param title label identifying the panel.
/// \param value serialized state to display; null represents unavailable state.
void jsonPanel(const char* title, const nlohmann::json& value)
{
   if (ImGui::TreeNode(title))
   {
      const auto text = value.is_null() ? std::string{"No serialized state available."} : value.dump(2);
      ImGui::TextUnformatted(text.c_str());
      ImGui::TreePop();
   }
}
}  // namespace

MechanismInspector::MechanismInspector()
    : _window(std::make_unique<sf::RenderWindow>(sf::VideoMode({1000, 760}), "deceptus mechanism editor"))
{
   _initialized = ImGui::SFML::Init(*_window);
}

MechanismInspector::~MechanismInspector()
{
   if (_initialized)
   {
      ImGui::SFML::Shutdown(*_window);
   }
}

bool MechanismInspector::isOpen() const
{
   return _initialized && _window->isOpen();
}

bool MechanismInspector::isPicking() const
{
   return isOpen() && _picking;
}

void MechanismInspector::processEvents()
{
   while (const auto event = _window->pollEvent())
   {
      if (_initialized)
      {
         ImGui::SFML::ProcessEvent(*_window, *event);
      }
      if (event->is<sf::Event::Closed>())
      {
         _window->close();
      }
      else if (const auto key = event->getIf<sf::Event::KeyPressed>(); key && key->code == sf::Keyboard::Key::F6)
      {
         _window->close();
      }
   }
}

void MechanismInspector::refresh(const std::shared_ptr<Level>& level)
{
   discardPreviews(level);
   _level = level;
   _entries.clear();
   _hovered.clear();
   _hover_clock.restart();
   _picked.clear();
   _position_filter.reset();
   _selected.reset();
   _picking = false;
   _selection_status.clear();
   _tmx_filename.clear();
   if (!level)
   {
      return;
   }

   _tmx_filename = level->getTmxFilename();
   std::ifstream source_file(_tmx_filename, std::ios::binary);
   _source_text.assign(std::istreambuf_iterator<char>{source_file}, {});
   const auto& registry = level->getMechanismRegistry();
   const auto& schemas = GameMechanismDeserializerRegistry::instance().schemas();
   const auto add_source = [&](const std::shared_ptr<TmxObject>& object, const std::string& group, bool layer)
   {
      // resolve authoring types, including customizations sharing a layer, independently of runtime class names
      auto schema_group = GameMechanismDeserializerRegistry::instance().getLayerName(group).value_or(group);
      if (group == "lasers_1" || group == "lasers_2")
      {
         schema_group = "lasers";
      }
      if (group == "interval_spikes" || group == "toggle_spikes" || group == "trap_spikes")
      {
         schema_group = "spikes";
      }
      if (group.starts_with("shader_quads"))
      {
         schema_group = "shader_quads";
      }
      const auto has_layer_schema =
         std::ranges::any_of(schemas, [&](const auto& candidate) { return candidate.layer_name == schema_group; });
      const auto template_type = object->_template_type ? std::string_view{*object->_template_type} : std::string_view{};
      const MechanismSchema* schema = nullptr;
      for (const auto& candidate : schemas)
      {
         if (has_layer_schema ? candidate.layer_name != schema_group : candidate.type_name != template_type)
         {
            continue;
         }
         if (!candidate.selector_property.empty())
         {
            const auto key = std::string{candidate.selector_property};
            if (object->_properties && object->_properties->_map.contains(key) &&
                object->_properties->_map.at(key)->toString() == candidate.selector_value)
            {
               schema = &candidate;
               break;
            }
         }
         else if (!schema || candidate.type_name == template_type)
         {
            schema = &candidate;
         }
      }
      if (!schema)
      {
         return;
      }
      auto snapshot = std::make_shared<TmxObject>(*object);
      if (object->_properties)
      {
         snapshot->_properties = std::make_shared<TmxProperties>(*object->_properties);
      }
      Entry entry{
         .group = std::string{schema->layer_name},
         .name = object->_name,
         .type = std::string{schema->type_name},
         .source = snapshot,
         .layer = layer
      };
      entry.schema = schema;
      entry.targets = targets(*snapshot);
      if (snapshot->_properties)
      {
         for (const auto& [key, property] : snapshot->_properties->_map)
         {
            if (key.starts_with("condition_") && !property->toString().empty())
            {
               for (const auto& requirement : StringUtils::split(property->toString(), ','))
               {
                  auto condition = MechanismCondition::parse(requirement);
                  if (condition)
                  {
                     condition->resolveReferences(registry.getMap());
                  }
                  entry.conditions.push_back({key + ": " + requirement, std::move(condition)});
               }
            }
         }
      }
      _entries.push_back(std::move(entry));
   };
   for (const auto& element : level->getInspectorElements())
   {
      if (const auto group = std::dynamic_pointer_cast<TmxObjectGroup>(element))
      {
         for (const auto& [id, object] : group->_objects)
         {
            add_source(object, group->_name, false);
         }
      }
      else if (const auto layer = std::dynamic_pointer_cast<TmxLayer>(element); layer && layer->_inspector_mechanism_layer)
      {
         auto object = std::make_shared<TmxObject>();
         object->_id = layer->_inspector_id;
         object->_name = layer->_name;
         object->_properties = layer->_properties;
         object->_x_px = static_cast<float>(layer->_position_x_px);
         object->_y_px = static_cast<float>(layer->_position_y_px);
         object->_width_px = static_cast<float>(layer->_width_tl * PIXELS_PER_TILE);
         object->_height_px = static_cast<float>(layer->_height_tl * PIXELS_PER_TILE);
         add_source(object, layer->_name, true);
      }
   }
   for (const auto& [group, mechanisms] : registry.getMap())
   {
      for (const auto& mechanism : *mechanisms)
      {
         const auto& source = mechanism->getInspectorSource();
         auto entry =
            source
               ? std::ranges::find_if(
                    _entries,
                    [&](const auto& candidate)
                    { return candidate.source && candidate.layer == mechanism->isInspectorLayer() && candidate.source->_id == source->_id; }
                 )
               : _entries.end();
         if (entry == _entries.end())
         {
            const auto node = std::dynamic_pointer_cast<GameNode>(mechanism);
            _entries.push_back(Entry{.group = group, .name = node ? node->getObjectId() : "", .type = std::string{mechanism->objectName()}}
            );
            entry = std::prev(_entries.end());
         }
         if (entry->parts.empty())
         {
            entry->mechanism = mechanism;
            entry->group = group;
            if (const auto node = std::dynamic_pointer_cast<GameNode>(mechanism))
            {
               entry->name = node->getObjectId();
            }
         }
         entry->parts.push_back(mechanism);
      }
   }
   std::ranges::sort(
      _entries, [](const auto& a, const auto& b) { return std::tie(a.group, a.name, a.type) < std::tie(b.group, b.name, b.type); }
   );
}

std::vector<std::string> MechanismInspector::Entry::liveValues(const std::string& name) const
{
   std::vector<std::string> values;
   for (const auto& part : parts)
   {
      const auto mechanism = part.lock();
      if (!mechanism || !mechanism->supportsInspectorProperty(name))
      {
         return {};
      }
      const auto value = mechanism->getInspectorProperty(name);
      if (!value)
      {
         return {};
      }
      values.push_back(*value);
   }
   return values;
}

std::optional<std::string> MechanismInspector::Entry::liveValue(const std::string& name) const
{
   const auto values = liveValues(name);
   return values.empty() ? std::nullopt : std::optional{values.front()};
}

bool MechanismInspector::Entry::setLiveValue(const std::string& name, const std::string& value)
{
   if (schema)
   {
      const auto property = std::ranges::find(schema->properties, name, &PropertyInfo::name);
      if (property == schema->properties.end() || !MechanismPropertyBindings::validateValue(*property, value))
      {
         return false;
      }
   }
   const auto previous = liveValues(name);
   if (previous.empty())
   {
      return false;
   }
   for (const auto& part : parts)
   {
      const auto mechanism = part.lock();
      if (!mechanism || !mechanism->validateInspectorProperty(name, value))
      {
         return false;
      }
   }
   for (size_t index = 0; index < parts.size(); ++index)
   {
      if (const auto mechanism = parts[index].lock(); !mechanism || !mechanism->setInspectorProperty(name, value))
      {
         for (size_t rollback = 0; rollback < index; ++rollback)
         {
            if (const auto part = parts[rollback].lock())
            {
               part->setInspectorProperty(name, previous[rollback]);
            }
         }
         return false;
      }
   }
   return true;
}

std::optional<sf::FloatRect> MechanismInspector::bounds(const Entry& entry) const
{
   std::optional<sf::FloatRect> combined;
   for (const auto& part : entry.parts)
   {
      if (const auto mechanism = part.lock())
      {
         if (const auto rect = mechanism->get_editor_bounds())
         {
            if (!combined)
            {
               combined = rect;
            }
            else
            {
               const auto left = std::min(combined->position.x, rect->position.x);
               const auto top = std::min(combined->position.y, rect->position.y);
               const auto right = std::max(combined->position.x + combined->size.x, rect->position.x + rect->size.x);
               const auto bottom = std::max(combined->position.y + combined->size.y, rect->position.y + rect->size.y);
               combined = sf::FloatRect{{left, top}, {right - left, bottom - top}};
            }
         }
      }
   }
   if (combined)
   {
      return combined;
   }
   if (entry.source)
   {
      const auto& source = *entry.source;
      if (source._polygon || source._polyline)
      {
         const auto& points = source._polygon ? source._polygon->_polyline : source._polyline->_path;
         if (!points.empty())
         {
            const auto [left, right] = std::ranges::minmax(points, {}, &sf::Vector2f::x);
            const auto [top, bottom] = std::ranges::minmax(points, {}, &sf::Vector2f::y);
            return sf::FloatRect{
               {source._x_px + left.x, source._y_px + top.y}, {std::max(1.0f, right.x - left.x), std::max(1.0f, bottom.y - top.y)}
            };
         }
      }
      return sf::FloatRect{{source._x_px, source._y_px}, {source._width_px, source._height_px}};
   }
   return std::nullopt;
}

bool MechanismInspector::renderedAt(const Entry& entry, sf::Vector2i pixel, const sf::View& view)
{
   if (!_pick_color || !_pick_normal)
   {
      return false;
   }
   // draw each instance separately so another object cannot occlude its picking result
   return std::ranges::any_of(
      entry.parts,
      [&](const auto& part)
      {
         const auto mechanism = part.lock();
         return mechanism && pixelHit(*mechanism, *_pick_color, *_pick_normal, pixel, view);
      }
   );
}

std::vector<size_t> MechanismInspector::hits_at(sf::Vector2f position, sf::Vector2u image_size, sf::Vector2i pixel)
{
   const auto level = _level.lock();
   if (!level || image_size.x == 0 || image_size.y == 0)
   {
      return {};
   }
   // share the same visual hit detection between explicit picking and editor-only hover checks
   if (!_pick_color || _pick_color->getSize() != image_size)
   {
      auto color = std::make_unique<sf::RenderTexture>();
      auto normal = std::make_unique<sf::RenderTexture>();
      if (!color->resize(image_size) || !normal->resize(image_size))
      {
         return {};
      }
      _pick_color = std::move(color);
      _pick_normal = std::move(normal);
   }
   auto view = *level->getLevelView();
   // retain the real camera and image resolution; scissoring limits ordinary draws to the sampled pixel region
   const auto left = std::max(0, pixel.x - 1);
   const auto top = std::max(0, pixel.y - 1);
   const auto right = std::min(static_cast<int>(image_size.x), pixel.x + 2);
   const auto bottom = std::min(static_cast<int>(image_size.y), pixel.y + 2);
   if (left >= right || top >= bottom)
   {
      return {};
   }
   view.setScissor(sf::FloatRect{
      {static_cast<float>(left) / image_size.x, static_cast<float>(top) / image_size.y},
      {static_cast<float>(right - left) / image_size.x, static_cast<float>(bottom - top) / image_size.y}
   });
   std::vector<size_t> hits;
   for (size_t index = 0; index < _entries.size(); ++index)
   {
      if (const auto rect = bounds(_entries[index]); (rect && rect->contains(position)) || renderedAt(_entries[index], pixel, view))
      {
         hits.push_back(index);
      }
   }
   return hits;
}

void MechanismInspector::selectAt(const std::optional<sf::Vector2f>& position, sf::Vector2u image_size, sf::Vector2i pixel)
{
   _picked.clear();
   _selected.reset();
   const auto level = _level.lock();
   if (!level)
   {
      _selection_status = "No loaded level to inspect.";
      return;
   }
   if (!position)
   {
      _selection_status = "That click was outside the game image. Click an object inside the scene.";
      _window->requestFocus();
      return;
   }
   _position_filter = position;
   _picked = hits_at(*position, image_size, pixel);
   _filter[0] = '\0';
   _picking = false;
   _selection_status =
      std::format("Position {:.0f}, {:.0f}: {} mechanisms. Choose one from the filtered list.", position->x, position->y, _picked.size());
   Log::Info() << "mechanism editor: " << _selection_status;
   _window->requestFocus();
}

void MechanismInspector::draw_hover(
   sf::RenderWindow& window,
   const std::shared_ptr<Level>& level,
   const sf::RenderTexture& image,
   sf::FloatRect placement
)
{
   if (!isOpen())
   {
      return;
   }
   if (_level.lock() != level)
   {
      refresh(level);
   }
   if (!isPicking())
   {
      _hovered.clear();
      _hover_clock.restart();
      return;
   }
   if (!level || placement.size.x <= 0.0f || placement.size.y <= 0.0f)
   {
      _hovered.clear();
      _hover_clock.restart();
      return;
   }
   const auto cursor = sf::Vector2f(sf::Mouse::getPosition(window));
   const auto image_size = image.getSize();
   const sf::Vector2f scale{placement.size.x / image_size.x, placement.size.y / image_size.y};
   const auto& camera = *level->getLevelView();
   if (placement.contains(cursor))
   {
      const sf::Vector2i pixel{
         static_cast<int>((cursor.x - placement.position.x) / scale.x), static_cast<int>((cursor.y - placement.position.y) / scale.y)
      };
      if (_hover_clock.getElapsedTime() >= sf::milliseconds(100))
      {
         _hovered = hits_at(image.mapPixelToCoords(pixel, camera), image_size, pixel);
         _hover_clock.restart();
      }
   }
   else
   {
      _hovered.clear();
      _hover_clock.restart();
   }
   if (_hovered.empty() && !_show_all_bounds)
   {
      return;
   }
   const auto previous_view = window.getView();
   window.setView(window.getDefaultView());
   const auto draw_outline = [&](const Entry& entry, const sf::Color& color, const sf::Color& fill, float thickness)
   {
      if (const auto rect = bounds(entry))
      {
         const auto first = sf::Vector2f(image.mapCoordsToPixel(rect->position, camera));
         const auto last = sf::Vector2f(image.mapCoordsToPixel(rect->position + rect->size, camera));
         // clip source bounds to the game image and skip sources outside the camera
         const auto left = std::clamp(std::min(first.x, last.x), 0.0f, static_cast<float>(image_size.x));
         const auto top = std::clamp(std::min(first.y, last.y), 0.0f, static_cast<float>(image_size.y));
         const auto right = std::clamp(std::max(first.x, last.x), 0.0f, static_cast<float>(image_size.x));
         const auto bottom = std::clamp(std::max(first.y, last.y), 0.0f, static_cast<float>(image_size.y));
         if (left >= right || top >= bottom)
         {
            return;
         }
         DebugDraw::drawRect(
            window,
            sf::FloatRect{
               placement.position + sf::Vector2f{left * scale.x, top * scale.y}, {(right - left) * scale.x, (bottom - top) * scale.y}
            },
            color,
            fill,
            thickness
         );
      }
   };
   if (_show_all_bounds)
   {
      for (const auto& entry : _entries)
      {
         draw_outline(entry, {255, 215, 70, 90}, sf::Color::Transparent, -1.0f);
      }
   }
   const sf::Color accent{255, 215, 70};
   std::string labels;
   for (const auto index : _hovered)
   {
      const auto& entry = _entries[index];
      draw_outline(entry, accent, {255, 215, 70, 30}, -2.0f);
      if (!labels.empty())
      {
         labels += '\n';
      }
      labels += std::format(
         "{}{}{} [#{}]", entry.type, entry.name.empty() ? "" : ": ", entry.name, entry.source ? entry.source->_id : std::string{}
      );
   }
   DebugDraw::draw_label(window, labels, cursor + sf::Vector2f{18.0f, 18.0f}, placement, accent);
   window.setView(previous_view);
}

void MechanismInspector::followReference(size_t index)
{
   _selected = index;
   _position_filter.reset();
   _picked.clear();
   _filter[0] = '\0';
   _scroll_to_selected = true;
   _selection_status.clear();
}

void MechanismInspector::drawReference(const std::string& name)
{
   bool found = false;
   for (size_t index = 0; index < _entries.size(); ++index)
   {
      if (_entries[index].name == name)
      {
         found = true;
         ImGui::PushID(static_cast<int>(index));
         if (ImGui::SmallButton(std::format("{} / {}", _entries[index].group, name).c_str()))
         {
            followReference(index);
         }
         ImGui::PopID();
      }
   }
   if (!found)
   {
      ImGui::Text("Missing mechanism: %s", name.c_str());
   }
}

void MechanismInspector::drawDetails(Entry& entry)
{
   const auto mechanism = entry.mechanism.lock();
   ImGui::Text("Runtime parts: %zu", entry.parts.size());
   ImGui::Text("%s / %s", entry.group.c_str(), entry.name.empty() ? "(unnamed)" : entry.name.c_str());
   ImGui::Text("Type: %s", entry.type.c_str());
   if (entry.source)
   {
      ImGui::Text("TMX source ID: %s", entry.source->_id.c_str());
   }
   if (mechanism)
   {
      ImGui::Text("Z: %d", mechanism->getZ());
      ImGui::Text("Enabled: %s   Visible: %s", mechanism->isEnabled() ? "yes" : "no", mechanism->isVisible() ? "yes" : "no");
      if (const auto interaction = std::dynamic_pointer_cast<InteractionInterface>(mechanism))
      {
         ImGui::Text("Interaction available: %s", interaction->isInteractionAvailable() ? "yes" : "no");
      }
   }
   else
   {
      ImGui::TextWrapped("Authored source without a standalone runtime instance. Properties can be saved and applied by reloading.");
   }
   if (const auto rect = bounds(entry))
   {
      ImGui::Text("Bounds: %.1f, %.1f   Size: %.1f x %.1f", rect->position.x, rect->position.y, rect->size.x, rect->size.y);
   }
   if (!entry.name.empty() &&
       std::ranges::count_if(_entries, [&entry](const auto& other) { return other.group == entry.group && other.name == entry.name; }) > 1)
   {
      ImGui::TextWrapped("Duplicate ID in this group: saved state and script lookups may be ambiguous.");
   }

   nlohmann::json current;
   if (mechanism)
   {
      mechanism->serializeState(current);
   }
   nlohmann::json saved;
   const auto& states = SaveState::getCurrent()._level_state;
   if (const auto level = states.find(_tmx_filename); level != states.end() && level->is_object())
   {
      if (const auto group = level->find(entry.group); group != level->end() && group->is_object())
      {
         if (const auto object = group->find(entry.name); object != group->end())
         {
            saved[entry.name] = *object;
         }
      }
   }
   ImGui::Separator();
   if (!current.is_null() && !saved.is_null())
   {
      ImGui::Text("State differs from last level snapshot: %s", current == saved ? "no" : "yes");
   }
   ImGui::TextWrapped("Live previews stay in memory until Save. Checkpoints may capture serialized runtime changes.");
   jsonPanel("Live serialized state", current);
   jsonPanel("Last level snapshot", saved);

   ImGui::Separator();
   ImGui::TextUnformatted("Condition requirements");
   if (entry.conditions.empty())
   {
      ImGui::TextDisabled("No Tiled condition requirements exposed.");
   }
   for (const auto& condition : entry.conditions)
   {
      ImGui::TextWrapped(
         "[%s] %s",
         !condition.condition                 ? "invalid"
         : condition.condition->isSatisfied() ? "met"
                                              : "not met",
         condition.label.c_str()
      );
   }

   ImGui::Separator();
   ImGui::TextUnformatted("Tiled target/reference links");
   for (const auto& target : entry.targets)
   {
      ImGui::PushID(target.c_str());
      drawReference(target);
      ImGui::PopID();
   }
   ImGui::TextDisabled("Script-created connections are not inferred.");
   if (ImGui::TreeNode("Referenced by"))
   {
      for (size_t index = 0; index < _entries.size(); ++index)
      {
         if (std::ranges::find(_entries[index].targets, entry.name) != _entries[index].targets.end())
         {
            ImGui::PushID(static_cast<int>(index));
            if (ImGui::SmallButton(std::format("{} / {}", _entries[index].group, _entries[index].name).c_str()))
            {
               followReference(index);
            }
            ImGui::PopID();
         }
      }
      ImGui::TreePop();
   }
   if (ImGui::TreeNode("Tiled source properties"))
   {
      if (entry.source)
      {
         ImGui::Text("TMX source ID: %s", entry.source->_id.c_str());
         if (entry.source->_properties)
         {
            for (const auto& [key, property] : entry.source->_properties->_map)
            {
               ImGui::TextWrapped("%s = %s", key.c_str(), property->toString().c_str());
            }
         }
      }
      else
      {
         ImGui::TextWrapped("No unique matching source object. Runtime-generated objects may have no Tiled source.");
      }
      ImGui::TreePop();
   }
   drawProperties(entry);
}

void MechanismInspector::discard(Entry& entry)
{
   for (const auto& edit : entry.properties)
   {
      if (!edit.changed)
      {
         continue;
      }
      for (size_t index = 0; index < edit.original_parts.size(); ++index)
      {
         if (const auto mechanism = entry.parts[index].lock())
         {
            mechanism->setInspectorProperty(std::string{edit.schema->name}, edit.original_parts[index]);
         }
      }
   }
   entry.properties.clear();
   entry.properties_initialized = false;
   entry.property_message.clear();
}

void MechanismInspector::discardPreviews(const std::shared_ptr<Level>& active_level)
{
   if (active_level && active_level == _level.lock())
   {
      for (auto& entry : _entries)
      {
         discard(entry);
      }
   }
}

void MechanismInspector::saveProperties()
{
   std::vector<MechanismSourceEditor::ObjectOverride> objects;
   for (const auto& entry : _entries)
   {
      if (!entry.source || !entry.schema)
      {
         continue;
      }
      MechanismSourceEditor::ObjectOverride object{entry.source->_id, entry.schema, {}, entry.layer ? "layer" : "object"};
      for (const auto& edit : entry.properties)
      {
         if (edit.changed)
         {
            if (!edit.valid)
            {
               _selection_status = "Correct rejected live values or discard them before saving.";
               return;
            }
            object.properties.push_back({std::string{edit.schema->name}, edit.buffer.data()});
         }
      }
      if (!object.properties.empty())
      {
         objects.push_back(std::move(object));
      }
   }
   auto result = MechanismSourceEditor::writeObjects(_tmx_filename, _source_text, objects);
   if (!result)
   {
      _selection_status = result.error().message;
      return;
   }
   if (const auto level = _level.lock())
   {
      level->acceptInspectorContents(result->contents);
   }
   _source_text = std::move(result->contents);
   _selection_status = std::format(
      "Saved to TMX without reloading. Backup: {}. Properties marked 'reload' need a manual level reload.", result->backup_path.string()
   );
   for (auto& entry : _entries)
   {
      for (auto& edit : entry.properties)
      {
         if (!edit.changed)
         {
            continue;
         }
         if (!entry.source->_properties)
         {
            entry.source->_properties = std::make_shared<TmxProperties>();
         }
         auto property = std::make_shared<TmxProperty>();
         tinyxml2::XMLDocument property_document;
         auto* property_element = property_document.NewElement("property");
         property_element->SetAttribute("type", std::string{edit.schema->type}.c_str());
         property_element->SetAttribute("value", edit.buffer.data());
         property->deserialize(property_element, nullptr);
         entry.source->_properties->_map[std::string{edit.schema->name}] = property;
         edit.changed = false;
         edit.mixed = false;
         edit.initial_text = edit.buffer.data();
         edit.original_parts = entry.liveValues(std::string{edit.schema->name});
      }
   }
}

void MechanismInspector::drawProperties(Entry& entry)
{
   if (!entry.source || !entry.schema)
   {
      ImGui::TextDisabled("No schema-backed editable source object for this mechanism.");
      return;
   }
   if (!entry.properties_initialized)
   {
      entry.properties_initialized = true;
      entry.properties.clear();
      entry.property_message.clear();
      for (const auto& schema : entry.schema->properties)
      {
         auto value = std::visit([](const auto& item) { return std::format("{}", item); }, schema.default_value);
         if (entry.source->_properties)
         {
            const auto found = entry.source->_properties->_map.find(std::string{schema.name});
            if (found != entry.source->_properties->_map.end())
            {
               value = found->second->toString();
            }
         }
         PropertyEdit edit{.schema = &schema};
         const auto original_live = entry.liveValue(std::string{schema.name});
         edit.original_parts = entry.liveValues(std::string{schema.name});
         edit.mixed = !edit.original_parts.empty() &&
                      std::ranges::any_of(edit.original_parts, [&](const auto& value) { return value != edit.original_parts.front(); });
         if (original_live)
         {
            value = *original_live;
         }
         edit.initial_text = value;
         edit.buffer.assign(value.begin(), value.end());
         edit.buffer.resize(std::max(size_t{256}, value.size() + 1), '\0');
         entry.properties.push_back(std::move(edit));
      }
   }
   ImGui::Separator();
   ImGui::TextUnformatted("Mechanism schema properties");
   ImGui::TextWrapped(
      "Edit to preview live-supported fields. Save writes all pending object overrides without reloading; fields marked "
      "'reload' need a manual level reload. Shared templates are unchanged."
   );
   ImGui::PushID(entry.layer ? "layer" : "object");
   ImGui::PushID(entry.source->_id.c_str());
   for (auto& edit : entry.properties)
   {
      const auto& schema = *edit.schema;
      const auto live_value = entry.liveValue(std::string{schema.name});
      const auto live = live_value.has_value();
      if (live_value && !edit.changed && !ImGui::IsAnyItemActive() && entry.liveValues(std::string{schema.name}) != edit.original_parts)
      {
         edit.original_parts = entry.liveValues(std::string{schema.name});
         edit.mixed = !edit.original_parts.empty() &&
                      std::ranges::any_of(edit.original_parts, [&](const auto& value) { return value != edit.original_parts.front(); });
         edit.initial_text = *live_value;
         edit.buffer.assign(live_value->begin(), live_value->end());
         edit.buffer.resize(std::max(size_t{256}, live_value->size() + 1), '\0');
      }
      const auto label = std::string{schema.name} + (live ? " [live]" : " [reload]") + (edit.mixed && !edit.changed ? " [mixed]" : "") +
                         (edit.changed ? " *" : "") + "###value";
      const std::string previous = edit.buffer.data();
      ImGui::PushID(schema.name.data(), schema.name.data() + schema.name.size());
      const auto set_value = [&edit](const std::string& value)
      {
         edit.buffer.assign(value.begin(), value.end());
         edit.buffer.resize(std::max(size_t{256}, value.size() + 1), '\0');
         edit.changed = edit.mixed || value != edit.initial_text;
      };
      if (!schema.allowed_values.empty())
      {
         if (ImGui::BeginCombo(label.c_str(), edit.buffer.data()))
         {
            for (const auto& value : schema.allowed_values)
            {
               if (ImGui::Selectable(std::string{value}.c_str(), value == edit.buffer.data()))
               {
                  set_value(std::string{value});
               }
            }
            ImGui::EndCombo();
         }
      }
      else if (schema.type == "color")
      {
         const auto parsed = MechanismPropertyBindings::parseColor(edit.buffer.data());
         const auto color = parsed.value_or(sf::Color::Transparent);
         float rgba[]{color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
         if (ImGui::ColorEdit4(label.c_str(), rgba, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf))
         {
            const auto channel = [](float value) { return static_cast<uint8_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f)); };
            set_value(
               MechanismPropertyBindings::formatColor(sf::Color{channel(rgba[0]), channel(rgba[1]), channel(rgba[2]), channel(rgba[3])})
            );
         }
         if (!parsed)
         {
            ImGui::TextDisabled("Invalid color; choose a color or restore the default.");
         }
      }
      else if (schema.type == "bool")
      {
         bool value = std::string_view{edit.buffer.data()} == "true" || std::string_view{edit.buffer.data()} == "1";
         if (ImGui::Checkbox(label.c_str(), &value))
         {
            set_value(value ? "true" : "false");
         }
      }
      else if (schema.type == "int")
      {
         int value = 0;
         const std::string_view text{edit.buffer.data()};
         std::from_chars(text.data(), text.data() + text.size(), value);
         if (ImGui::InputInt(label.c_str(), &value))
         {
            set_value(std::format("{}", value));
         }
      }
      else if (schema.type == "float")
      {
         float value = 0.0f;
         const std::string_view text{edit.buffer.data()};
         std::from_chars(text.data(), text.data() + text.size(), value);
         if (ImGui::InputFloat(label.c_str(), &value))
         {
            set_value(std::format("{}", value));
         }
      }
      else
      {
         const auto resize = [](ImGuiInputTextCallbackData* data)
         {
            auto& buffer = *static_cast<std::vector<char>*>(data->UserData);
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
            {
               buffer.resize(static_cast<size_t>(data->BufSize));
               data->Buf = buffer.data();
            }
            return 0;
         };
         if (ImGui::InputText(
                label.c_str(), edit.buffer.data(), edit.buffer.size(), ImGuiInputTextFlags_CallbackResize, resize, &edit.buffer
             ))
         {
            edit.changed = edit.mixed || std::string_view{edit.buffer.data()} != edit.initial_text;
         }
      }
      ImGui::SameLine();
      if (ImGui::SmallButton("Default"))
      {
         set_value(std::visit([](const auto& item) { return std::format("{}", item); }, schema.default_value));
      }
      if (previous != edit.buffer.data())
      {
         _selection_status.clear();
      }
      if (live && previous != edit.buffer.data())
      {
         edit.valid = entry.setLiveValue(std::string{schema.name}, edit.buffer.data());
         if (!edit.valid)
         {
            entry.property_message = "Live value rejected for '" + std::string{schema.name} + "'. Correct it or discard before saving.";
         }
         else
         {
            entry.property_message.clear();
         }
      }
      ImGui::PopID();
   }
   if (ImGui::Button("Discard this object's property edits"))
   {
      discard(entry);
   }
   ImGui::PopID();
   ImGui::PopID();
   if (!entry.property_message.empty())
   {
      ImGui::TextWrapped("%s", entry.property_message.c_str());
   }
}

void MechanismInspector::draw(const std::shared_ptr<Level>& level)
{
   if (!isOpen())
   {
      return;
   }
   if (_level.lock() != level)
   {
      refresh(level);
   }
   ImGui::SFML::Update(*_window, _clock.restart());
   const auto* viewport = ImGui::GetMainViewport();
   ImGui::SetNextWindowPos(viewport->WorkPos);
   ImGui::SetNextWindowSize(viewport->WorkSize);
   ImGui::Begin(
      "Mechanism editor",
      nullptr,
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar
   );
   ImGui::TextUnformatted("Mechanism editor");
   if (ImGui::Button(_picking ? "Cancel picking" : "Pick an object in the game"))
   {
      _picking = !_picking;
      _hovered.clear();
      _hover_clock.restart();
   }
   if (_picking)
   {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Picking active: left-click an object in the game window.");
      ImGui::Checkbox("Show all mechanism bounds", &_show_all_bounds);
   }
   if (!_selection_status.empty())
   {
      ImGui::TextWrapped("%s", _selection_status.c_str());
   }
   ImGui::BeginDisabled(!level);
   if (ImGui::Button("Save all property changes to TMX"))
   {
      saveProperties();
   }
   ImGui::SameLine();
   if (ImGui::Button("Discard all previews"))
   {
      discardPreviews(level);
   }
   ImGui::EndDisabled();
   ImGui::TextDisabled("Picking filters all overlaps. Live fields preview immediately; Save writes to TMX.");
   if (_position_filter)
   {
      if (ImGui::SmallButton("Clear position filter"))
      {
         _position_filter.reset();
         _picked.clear();
         _selection_status.clear();
      }
   }
   if (!level)
   {
      _picking = false;
      ImGui::TextUnformatted("Load a level to edit mechanisms.");
   }
   else
   {
      ImGui::SetNextItemWidth(300);
      ImGui::InputText("Search ID / group / type", _filter.data(), _filter.size());
      ImGui::SameLine();
      if (ImGui::Button("Refresh (discard previews)"))
      {
         refresh(level);
      }
      const auto filter = lower(_filter.data());
      ImGui::BeginChild("mechanisms", ImVec2(340, 0), true);
      for (size_t index = 0; index < _entries.size(); ++index)
      {
         const auto& entry = _entries[index];
         if (_position_filter && std::ranges::find(_picked, index) == _picked.end())
         {
            continue;
         }
         if (!filter.empty() &&
             !lower(entry.group + " " + entry.name + " " + entry.type + " " + (entry.source ? entry.source->_id : "")).contains(filter))
         {
            continue;
         }
         ImGui::PushID(static_cast<int>(index));
         const auto item_label = std::format(
            "{} / {}{}",
            entry.group,
            entry.name.empty() ? entry.type : entry.name,
            entry.source ? std::format(" #{}", entry.source->_id) : ""
         );
         if (ImGui::Selectable(item_label.c_str(), _selected == index))
         {
            _selected = index;
         }
         if (_scroll_to_selected && _selected == index)
         {
            ImGui::SetScrollHereY();
            _scroll_to_selected = false;
         }
         ImGui::PopID();
      }
      ImGui::EndChild();
      ImGui::SameLine();
      ImGui::BeginChild("details", ImVec2(0, 0), true);
      if (_selected && *_selected < _entries.size())
      {
         drawDetails(_entries[*_selected]);
      }
      else
      {
         ImGui::TextWrapped("Choose a mechanism from the list, or press 'Pick an object in the game' and left-click it in the scene.");
      }
      ImGui::EndChild();
   }
   ImGui::End();
   _window->clear();
   ImGui::SFML::Render(*_window);
   _window->display();
}

#endif
