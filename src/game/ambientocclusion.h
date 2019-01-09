#ifndef AMBIENTOCCLUSION_H
#define AMBIENTOCCLUSION_H

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>

class AmbientOcclusion
{

private:

  sf::Texture mTexture;
  std::vector<sf::Sprite> mSprites;

public:

  AmbientOcclusion() = default;

  void load(
    const std::filesystem::path& path,
    const std::string& aoBaseFilename
  );

  void draw(sf::RenderTarget &window);
};

#endif // AMBIENTOCCLUSION_H
