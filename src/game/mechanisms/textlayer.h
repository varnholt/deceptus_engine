#ifndef TEXTLAYER_H
#define TEXTLAYER_H

#include "game/io/gamedeserializedata.h"
#include "game/layers/bitmapfont.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <SFML/Graphics.hpp>
#ifdef __EMSCRIPTEN__
#include <optional>
#endif

struct TmxObject;

/// \brief renders static map text using bitmap fonts or truetype fonts.
struct TextLayer : public GameMechanism, public GameNode
{
public:
   /// \brief creates a text layer mechanism.
   /// \param parent owning game node in the scene graph.
   TextLayer(GameNode* parent = nullptr);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "TextLayer".
   std::string_view objectName() const override;

   /// \brief draws text using the configured bitmap or truetype mode.
   /// \param target render target.
   /// \param normal normal-map render target, unused by this mechanism.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

#ifdef __EMSCRIPTEN__
   /// \brief draws text with explicit render states (used in WASM to carry the level view).
   /// \param target render target.
   /// \param normal normal-map render target, unused by this mechanism.
   /// \param states render states to apply.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;
#endif

   /// \brief updates this mechanism.
   /// \param dt elapsed frame time, unused because text is static.
   void update(const sf::Time& dt) override;

   /// \brief returns the text layer bounds in pixel space.
   /// \return rectangle used for chunk activation and culling.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief creates a text layer and configures fonts, text content, and color.
   /// \param parent owning game node in the scene graph.
   /// \param data deserialization data with text and font properties.
   /// \return configured text layer instance.
   static std::shared_ptr<TextLayer> deserialize(GameNode* parent, const GameDeserializeData& data);

private:
   enum class Mode
   {
      Invalid,
      Bitmap,
      TrueType
   };

   Mode _mode = Mode::Invalid;

   sf::FloatRect _rect;
   std::shared_ptr<sf::Texture> _texture;

   BitmapFont _bitmap_font;
   std::vector<std::shared_ptr<sf::IntRect>> _bitmap_coords;

   std::string _text;
#ifdef __EMSCRIPTEN__
   std::optional<sf::Font> _truetype_font;
#else
   sf::Font _truetype_font;
#endif
   std::unique_ptr<sf::Text> _truetype_text;
};

#endif  // TEXTLAYER_H
