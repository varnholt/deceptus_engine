#include "menulabel.h"

#include "framework/tools/localization.h"
#include "framework/tools/localizedtext.h"
#include "framework/tools/log.h"
#include "framework/tools/sfmlcompat.h"

#include <memory>

namespace
{

//! every label is centered on the height of this character rather than on its own height, so a word
//! with a descender and one without end up on the same baseline
constexpr char32_t reference_codepoint{U'A'};

/// \brief creates a text carrying the translation of source_text.
/// \param source_text english source text, looked up through tr().
/// \param character_size character size to render at.
/// \param color fill color of the text.
/// \return the text, ready to be measured or drawn.
sf::Text createText(const std::string& source_text, uint32_t character_size, const sf::Color& color)
{
#ifdef DECEPTUS_VRSFML
   sf::Text text(getFont(), sf::Text::Data{});
#else
   sf::Text text(getFont());
#endif

   text.setCharacterSize(character_size);
   text.setFillColor(color);
   text.setString(LocalizedText::toSfmlString(tr(source_text)));

   return text;
}

/// \brief returns the position that places text inside box according to alignment.
/// \param text text to place; the size it renders at is read from it.
/// \param box region the text is placed in.
/// \param alignment horizontal placement inside the box.
/// \return position in whole pixels.
sf::Vector2f placement(const sf::Text& text, const sf::IntRect& box, MenuLabel::Align alignment)
{
   const auto text_bounds = text.getLocalBounds();
   const auto box_position = sf::Vector2f{static_cast<float>(box.position.x), static_cast<float>(box.position.y)};
   const auto box_size = sf::Vector2f{static_cast<float>(box.size.x), static_cast<float>(box.size.y)};

   auto x_px = box_position.x;
   switch (alignment)
   {
      case MenuLabel::Align::Left:
      {
         break;
      }
      case MenuLabel::Align::Centered:
      {
         x_px = box_position.x + (box_size.x - text_bounds.size.x) / 2.0f;
         break;
      }
      case MenuLabel::Align::Right:
      {
         x_px = box_position.x + box_size.x - text_bounds.size.x;
         break;
      }
   }

   const auto reference_height = getFont().getGlyph(reference_codepoint, text.getCharacterSize(), false, 0.0f).bounds.size.y;
   const auto y_px = box_position.y + (box_size.y - reference_height) / 2.0f - text_bounds.position.y;

   return {static_cast<float>(static_cast<int32_t>(x_px - text_bounds.position.x)), static_cast<float>(static_cast<int32_t>(y_px))};
}

}  // namespace

int32_t MenuLabel::measureWidth(const std::string& source_text, uint32_t character_size)
{
   const auto width_px = createText(source_text, character_size, sf::Color::White).getLocalBounds().size.x;
   return std::max(1, static_cast<int32_t>(std::ceil(width_px)));
}

std::vector<MenuLabel::KeptRegion> MenuLabel::stretchedPlate(const sf::IntRect& source, int32_t target_x_px, int32_t width_px, int32_t cap_width_px)
{
   const auto height_px = source.size.y;
   const auto source_x_px = source.position.x;

   return {
      KeptRegion{
         ._source = sf::IntRect{{source_x_px, source.position.y}, {cap_width_px, height_px}},  //
         ._target = sf::Vector2i{target_x_px, 0}
      },
      KeptRegion{
         ._source = sf::IntRect{{source_x_px + cap_width_px, source.position.y}, {1, height_px}},
         ._target = sf::Vector2i{target_x_px + cap_width_px, 0},
         ._size = sf::Vector2i{width_px - 2 * cap_width_px, height_px}
      },
      KeptRegion{
         ._source = sf::IntRect{{source_x_px + source.size.x - cap_width_px, source.position.y}, {cap_width_px, height_px}},
         ._target = sf::Vector2i{target_x_px + width_px - cap_width_px, 0}
      },
   };
}

void MenuLabel::compose(Layer& layer, const sf::Vector2i& size, const std::vector<KeptRegion>& kept_regions, const std::vector<Label>& labels)
{
   if (!layer._texture || !layer._sprite)
   {
      Log::Error() << "menu label: layer '" << layer._name << "' has no image to compose from";
      return;
   }

   const auto texture_size = sf::Vector2u{static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y)};

#ifdef DECEPTUS_VRSFML
   auto created_render_texture = sf::RenderTexture::create(texture_size);
   if (!created_render_texture.hasValue())
   {
      Log::Error() << "menu label: failed to create render texture for layer '" << layer._name << "'";
      return;
   }
   auto render_texture = std::make_unique<sf::RenderTexture>(std::move(*created_render_texture));
#else
   std::unique_ptr<sf::RenderTexture> render_texture;
   try
   {
      render_texture = std::make_unique<sf::RenderTexture>(texture_size);
   }
   catch (const std::exception& exception)
   {
      Log::Error() << "menu label: failed to create render texture for layer '" << layer._name << "': " << exception.what();
      return;
   }
#endif

   render_texture->clear(sf::Color::Transparent);

   for (const auto& kept : kept_regions)
   {
      const auto source = sf::FloatRect{
         {static_cast<float>(kept._source.position.x), static_cast<float>(kept._source.position.y)},
         {static_cast<float>(kept._source.size.x), static_cast<float>(kept._source.size.y)}
      };
      const auto target_size = kept._size.value_or(kept._source.size);
      const auto scale = sf::Vector2f{
         static_cast<float>(target_size.x) / source.size.x,  //
         static_cast<float>(target_size.y) / source.size.y
      };

#ifdef DECEPTUS_VRSFML
      sf::Sprite sprite;
      sprite.textureRect = source;
      sprite.position = {static_cast<float>(kept._target.x), static_cast<float>(kept._target.y)};
      sprite.scale = scale;

      sf::RenderStates states;
      states.texture = layer._texture.get();
      states.blendMode = sf::BlendAlpha;
      render_texture->draw(sprite, states);
#else
      sf::Sprite sprite(*layer._texture);
      sprite.setTextureRect(kept._source);
      sprite.setPosition({static_cast<float>(kept._target.x), static_cast<float>(kept._target.y)});
      sprite.setScale(scale);
      render_texture->draw(sprite, sf::RenderStates{sf::BlendAlpha});
#endif
   }

   for (const auto& label : labels)
   {
      auto text = createText(label._text, label._character_size, label._color);
      sfcompat::setPosition(text, placement(text, label._box, label._align));
      render_texture->draw(text);
   }

   render_texture->display();

   const auto image = render_texture->getTexture().copyToImage();

#ifdef DECEPTUS_VRSFML
   auto created_texture = sf::Texture::create(texture_size);
   if (!created_texture.hasValue())
   {
      Log::Error() << "menu label: failed to create texture for layer '" << layer._name << "'";
      return;
   }
   auto texture = std::make_shared<sf::Texture>(std::move(*created_texture));
#else
   auto texture = std::make_shared<sf::Texture>(texture_size);
#endif

   texture->update(image);
   texture->setSmooth(false);

   const auto position = sfcompat::getPosition(*layer._sprite);

#ifdef DECEPTUS_VRSFML
   auto sprite = std::make_shared<sf::Sprite>();
   sprite->textureRect = sf::FloatRect{{0.0f, 0.0f}, {static_cast<float>(size.x), static_cast<float>(size.y)}};
#else
   auto sprite = std::make_shared<sf::Sprite>(*texture);
#endif
   sfcompat::setPosition(*sprite, position);

   layer._texture = texture;
   layer._sprite = sprite;
}

void MenuLabel::compose(Layer& layer, const std::vector<KeptRegion>& kept_regions, const std::vector<Label>& labels)
{
   if (!layer._texture)
   {
      Log::Error() << "menu label: layer '" << layer._name << "' has no image to compose from";
      return;
   }

   const auto texture_size = layer._texture->getSize();
   compose(layer, {static_cast<int32_t>(texture_size.x), static_cast<int32_t>(texture_size.y)}, kept_regions, labels);
}
