#include "audio.h"

#include "gameconfiguration.h"

#include <string>
#include <iostream>
#include <filesystem>


Audio* Audio::sInstance = nullptr;

namespace
{
static const std::string SFX_ROOT = "data/sounds/";
}

//-----------------------------------------------------------------------------
/*!
 * \brief Audio::Audio
 * \param parent
 */
Audio::Audio()
{
   sInstance = this;
   initializeMusicVolume();
   initializeSamples();
   initializeTracks();
}


//-----------------------------------------------------------------------------
/*!
 * \brief Audio::getInstance
 * \return
 */
Audio* Audio::getInstance()
{
   if (sInstance == nullptr)
   {
      new Audio();
   }

   return sInstance;
}


//-----------------------------------------------------------------------------
void Audio::addSample(const std::string& sample)
{
   auto loader = [](const std::string& fileName) -> sf::SoundBuffer {
      sf::SoundBuffer buf;
      buf.loadFromFile(SFX_ROOT + fileName);
      return buf;
   };

   if (mSounds.find(sample) == mSounds.end())
   {
      mSounds[sample] = loader(sample);
   }
}



//-----------------------------------------------------------------------------
void Audio::initializeSamples()
{
   addSample("coin.wav");
   addSample("death.wav");
   addSample("footstep.wav");
   addSample("healthup.wav");
   addSample("hurt.wav");
   addSample("jump.wav");
   addSample("powerup.wav");
   addSample("splash.wav");
   addSample("impact.wav");
}


//-----------------------------------------------------------------------------
void Audio::initializeTracks()
{
    try {
        std::string path = "data/music";
        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            const auto path = entry.path().string();
            if (path.find(".ogg") != std::string::npos)
            {
                Track track;
                track.mFilename = path;
                mTracks.push_back(track);
            }

            // std::cout << entry.path() << std::endl;
        }
    }
    catch (std::exception&)
    {
    }
}


//-----------------------------------------------------------------------------
void Audio::initializeMusicVolume()
{
   auto& config = GameConfiguration::getInstance();
   const auto master = config.mAudioVolumeMaster * 0.01f;
   const auto music = config.mAudioVolumeMusic;
   mMusic.setVolume(master * music);
}


//-----------------------------------------------------------------------------
void Audio::playSample(const std::string& sample, float volume)
{
   sf::Sound* sound = nullptr;
   for (int i = 0; i < 10; i++)
   {
      if (mThreads[i].getStatus() == sf::Sound::Stopped)
      {
         sound = &mThreads[i];
         break;
      }
   }

   if (sound == nullptr)
   {
      return;
   }

   auto it = mSounds.find(sample);

   if (it != mSounds.end())
   {
      sound->setBuffer(it->second);

      const auto master = (GameConfiguration::getInstance().mAudioVolumeMaster * 0.01f);
      const auto sfx = (GameConfiguration::getInstance().mAudioVolumeSfx);

      sound->setVolume(master * sfx * volume);
      sound->play();
   }
}


//-----------------------------------------------------------------------------
void Audio::updateMusic()
{
    if (mTracks.empty())
    {
        return;
    }

    if (mMusic.getStatus() == sf::Music::Playing)
    {
        return;
    }

    mCurrentTrackIndex++;
    if (mCurrentTrackIndex >= mTracks.size())
    {
        mCurrentTrackIndex = 0;
    }

    mMusic.openFromFile(mTracks[mCurrentTrackIndex].mFilename);
    mMusic.play();
}


//-----------------------------------------------------------------------------
sf::Music& Audio::getMusic() const
{
   return mMusic;
}

