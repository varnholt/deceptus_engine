#include "menuscreenaudio.h"

#include "framework/tools/localization.h"
#include "framework/tools/sfmlstring.h"
#include "game/audio/audio.h"
#include "game/audio/musicplayer.h"
#include "game/config/gameconfiguration.h"
#include "menu.h"
#include "menuaudio.h"

#include <format>

#define STEP_SIZE 10

MenuScreenAudio::MenuScreenAudio()
{
   setFilename("data/menus/audio.psd");
}

void MenuScreenAudio::up()
{
   auto next = static_cast<int32_t>(_selection);
   next--;
   if (next < 0)
   {
      next = static_cast<int32_t>(Selection::Count) - 1;
   }

   _selection = static_cast<Selection>(next);
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenAudio::down()
{
   auto next = static_cast<int32_t>(_selection);
   next++;
   if (next == static_cast<int32_t>(Selection::Count))
   {
      next = 0;
   }

   _selection = static_cast<Selection>(next);
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenAudio::select()
{
   MenuAudio::play(MenuAudio::SoundEffect::ItemSelect);
}

void MenuScreenAudio::back()
{
   Menu::getInstance()->show(Menu::MenuType::Options);
   MenuAudio::play(MenuAudio::SoundEffect::MenuBack);
}

void MenuScreenAudio::set(int32_t x)
{
   switch (_selection)
   {
      case Selection::Master:
      {
         GameConfiguration::getInstance()._audio_volume_master += x;
         break;
      }
      case Selection::Music:
      {
         GameConfiguration::getInstance()._audio_volume_music += x;
         break;
      }
      case Selection::SFX:
      {
         GameConfiguration::getInstance()._audio_volume_sfx += x;
         break;
      }
      case Selection::Count:
      {
         break;
      }
   }

   if (GameConfiguration::getInstance()._audio_volume_master > 100)
   {
      GameConfiguration::getInstance()._audio_volume_master = 100;
   }

   if (GameConfiguration::getInstance()._audio_volume_master < 0)
   {
      GameConfiguration::getInstance()._audio_volume_master = 0;
   }

   if (GameConfiguration::getInstance()._audio_volume_music > 100)
   {
      GameConfiguration::getInstance()._audio_volume_music = 100;
   }

   if (GameConfiguration::getInstance()._audio_volume_music < 0)
   {
      GameConfiguration::getInstance()._audio_volume_music = 0;
   }

   if (GameConfiguration::getInstance()._audio_volume_sfx > 100)
   {
      GameConfiguration::getInstance()._audio_volume_sfx = 100;
   }

   if (GameConfiguration::getInstance()._audio_volume_sfx < 0)
   {
      GameConfiguration::getInstance()._audio_volume_sfx = 0;
   }

   GameConfiguration::getInstance().serializeToFile();

   // update the volume of active threads
   Audio::getInstance().adjustActiveSampleVolume();
   MusicPlayer::getInstance().adjustActiveMusicVolume();

   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemTick);
}

void MenuScreenAudio::setDefaults()
{
   GameConfiguration::resetAudioDefaults();
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::Apply);
}

void MenuScreenAudio::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (key == sf::Keyboard::Key::Up)
   {
      up();
   }

   else if (key == sf::Keyboard::Key::Down)
   {
      down();
   }

   else if (key == sf::Keyboard::Key::Enter)
   {
      select();
   }

   else if (key == sf::Keyboard::Key::Left)
   {
      set(-STEP_SIZE);
   }

   else if (key == sf::Keyboard::Key::Right)
   {
      set(STEP_SIZE);
   }

   else if (key == sf::Keyboard::Key::Escape)
   {
      back();
   }

   else if (key == sf::Keyboard::Key::D)
   {
      setDefaults();
   }
}

void MenuScreenAudio::loadingFinished()
{
   for (auto index = 0; index < 11; index++)
   {
      const auto master_value_layer = std::format("master_value_{}", index);
      const auto music_value_layer = std::format("mscVolume_value_{}", index);
      const auto sfx_value_layer = std::format("sfxVolume_value_{}", index);

      _volume_layers_master.push_back(_layers[master_value_layer]);
      _volume_layers_music.push_back(_layers[music_value_layer]);
      _volume_layers_sfx.push_back(_layers[sfx_value_layer]);
   }

   ensureFontLoaded();

   _row_label_base_rect = _layers["master_text_0"]->_sprite->getGlobalBounds();
   _row_help_base_rect = _layers["master_help"]->_sprite->getGlobalBounds();
   _row_stride = _layers["mscVolume_text_0"]->_sprite->getGlobalBounds().position.y - _row_label_base_rect.position.y;

   for (const auto& layer_name :
        {"master_text_0",
         "master_text_1",
         "master_help",
         "mscVolume_text_0",
         "mscVolume_text_1",
         "mscVolume_help",
         "sfxVolume_text_0",
         "sfxVolume_text_1",
         "sfxVolume_help"})
   {
      _layers[layer_name]->_visible = false;
   }

   auto make_label = [this]() -> std::unique_ptr<sf::Text>
   {
      auto text = std::make_unique<sf::Text>(_font);
      text->setFont(_font);
      text->setCharacterSize(12);
      return text;
   };

   _master_label = make_label();
   _master_help_text = make_label();
   _master_help_text->setFillColor(color_help_text);

   _music_label = make_label();
   _music_help_text = make_label();
   _music_help_text->setFillColor(color_help_text);

   _sfx_label = make_label();
   _sfx_help_text = make_label();
   _sfx_help_text->setFillColor(color_help_text);

   _text_back_button = make_label();
   _text_back_button->setFillColor(color_label_normal);
   _text_defaults_button = make_label();
   _text_defaults_button->setFillColor(color_label_normal);

   updateLayers();
}

void MenuScreenAudio::showEvent()
{
   updateLayers();
}

void MenuScreenAudio::updateLayers()
{
   const auto master_selected = _selection == Selection::Master;
   const auto sfx_selected = _selection == Selection::SFX;
   const auto music_selected = _selection == Selection::Music;

   _layers["defaults_xbox_0"]->_visible = isControllerUsed();
   _layers["defaults_xbox_1"]->_visible = false;
   _layers["back_xbox_0"]->_visible = isControllerUsed();
   _layers["back_xbox_1"]->_visible = false;

   _layers["defaults_pc_0"]->_visible = !isControllerUsed();
   _layers["defaults_pc_1"]->_visible = false;
   _layers["back_pc_0"]->_visible = !isControllerUsed();
   _layers["back_pc_1"]->_visible = false;

   _layers["sfxVolume_body_0"]->_visible = !sfx_selected;
   _layers["sfxVolume_body_1"]->_visible = sfx_selected;
   _layers["sfxVolume_highlight"]->_visible = sfx_selected;
   _layers["sfxVolume_arrows"]->_visible = sfx_selected;
   _layers["sfxVolume_h_0"]->_visible = !sfx_selected;
   _layers["sfxVolume_h_1"]->_visible = sfx_selected;

   _layers["mscVolume_body_0"]->_visible = !music_selected;
   _layers["mscVolume_body_1"]->_visible = music_selected;
   _layers["mscVolume_highlight"]->_visible = music_selected;
   _layers["mscVolume_arrows"]->_visible = music_selected;
   _layers["mscVolume_h_0"]->_visible = !music_selected;
   _layers["mscVolume_h_1"]->_visible = music_selected;

   _layers["master_body_1"]->_visible = master_selected;
   _layers["master_highlight"]->_visible = master_selected;
   _layers["master_arrows"]->_visible = master_selected;
   _layers["master_h_0"]->_visible = !master_selected;
   _layers["master_h_1"]->_visible = master_selected;

   const auto master_volume = GameConfiguration::getInstance()._audio_volume_master;
   const auto sfx_volume = GameConfiguration::getInstance()._audio_volume_sfx;
   const auto music_volume = GameConfiguration::getInstance()._audio_volume_music;

   const auto master_volume_layer_index = static_cast<int32_t>(master_volume / 10);
   const auto sfx_volume_layer_index = static_cast<int32_t>(sfx_volume / 10);
   const auto music_volume_layer_index = static_cast<int32_t>(music_volume / 10);

   for (auto index = 0; index < 11; index++)
   {
      _volume_layers_master[index]->_visible = (index == master_volume_layer_index);
      _volume_layers_sfx[index]->_visible = (index == sfx_volume_layer_index);
      _volume_layers_music[index]->_visible = (index == music_volume_layer_index);
   }

   _layers["master_h_0"]->_sprite->setOrigin({50.0f - master_volume, 0.0f});
   _layers["sfxVolume_h_0"]->_sprite->setOrigin({50.0f - sfx_volume, 0.0f});
   _layers["mscVolume_h_0"]->_sprite->setOrigin({50.0f - music_volume, 0.0f});

   _layers["master_h_1"]->_sprite->setOrigin({50.0f - master_volume, 0.0f});
   _layers["sfxVolume_h_1"]->_sprite->setOrigin({50.0f - sfx_volume, 0.0f});
   _layers["mscVolume_h_1"]->_sprite->setOrigin({50.0f - music_volume, 0.0f});

   if (!_master_label)
   {
      return;
   }

   _master_label->setString(sftr("Master Volume"));
   _master_label->setFillColor(master_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_master_label, rowRect(_row_label_base_rect, 0));

   _master_help_text->setString(sftr("Adjust the Master Volume"));

   placeTextCentered(*_master_help_text, _row_help_base_rect);

   _music_label->setString(sftr("Music Volume"));
   _music_label->setFillColor(music_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_music_label, rowRect(_row_label_base_rect, 1));

   _music_help_text->setString(sftr("Adjust the Music Volume"));

   placeTextCentered(*_music_help_text, _row_help_base_rect);

   _sfx_label->setString(sftr("Sound FX Volume"));
   _sfx_label->setFillColor(sfx_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_sfx_label, rowRect(_row_label_base_rect, 2));

   _sfx_help_text->setString(sftr("Adjust the Sound FX Volume"));

   placeTextCentered(*_sfx_help_text, _row_help_base_rect);

   const auto& back_layer = isControllerUsed() ? _layers["back_xbox_0"] : _layers["back_pc_0"];
   _text_back_button->setString(sftr("Back"));
   placeTextRightOf(*_text_back_button, back_layer->_sprite->getGlobalBounds());

   const auto& defaults_layer = isControllerUsed() ? _layers["defaults_xbox_0"] : _layers["defaults_pc_0"];
   _text_defaults_button->setString(sftr("Defaults"));
   placeTextRightOf(*_text_defaults_button, defaults_layer->_sprite->getGlobalBounds());
}

void MenuScreenAudio::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   MenuScreen::draw(window, states);

   if (!_master_label)
   {
      return;
   }

   window.draw(*_master_label, states);
   if (_selection == Selection::Master)
   {
      window.draw(*_master_help_text, states);
   }

   window.draw(*_music_label, states);
   if (_selection == Selection::Music)
   {
      window.draw(*_music_help_text, states);
   }

   window.draw(*_sfx_label, states);
   if (_selection == Selection::SFX)
   {
      window.draw(*_sfx_help_text, states);
   }

   window.draw(*_text_back_button, states);
   window.draw(*_text_defaults_button, states);
}

/*
data/menus/audio.psd
    bg_temp
    audio-window-bg
    audio_window-main
    main_body_0
    header
*/
