#pragma once

// SFML::Audio is compiled for WASM (miniaudio WebAudio backend).
// VRSFML on the wasm-vrsfml branch has no Audio.hpp umbrella — list headers directly.
#include <SFML/Audio/AudioContext.hpp>
#include <SFML/Audio/InputSoundFile.hpp>
#include <SFML/Audio/Listener.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/MusicReader.hpp>
#include <SFML/Audio/OutputSoundFile.hpp>
#include <SFML/Audio/PlaybackDevice.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/SoundBufferRecorder.hpp>
#include <SFML/Audio/SoundFileFactory.hpp>
#include <SFML/Audio/SoundFileReader.hpp>
#include <SFML/Audio/SoundFileWriter.hpp>
#include <SFML/Audio/SoundRecorder.hpp>
#include <SFML/Audio/SoundStream.hpp>
