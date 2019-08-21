#include "blur.h"


WaveBlur::WaveBlur() :
    Effect("wave + blur")
{
}


bool WaveBlur::onLoad()
{
    // Create the text
    mText.setString(
        "Praesent suscipit augue in velit pulvinar hendrerit varius purus aliquam.\n"
        "Mauris mi odio, bibendum quis fringilla a, laoreet vel orci. Proin vitae vulputate tortor.\n"
        "Praesent cursus ultrices justo, ut feugiat ante vehicula quis.\n"
        "Donec fringilla scelerisque mauris et viverra.\n"
        "Maecenas adipiscing ornare scelerisque. Nullam at libero elit.\n"
        "Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas.\n"
        "Nullam leo urna, tincidunt id semper eget, ultricies sed mi.\n"
        "Morbi mauris massa, commodo id dignissim vel, lobortis et elit.\n"
        "Fusce vel libero sed neque scelerisque venenatis.\n"
        "Integer mattis tincidunt quam vitae iaculis.\n"
        "Vivamus fringilla sem non velit venenatis fermentum.\n"
        "Vivamus varius tincidunt nisi id vehicula.\n"
        "Integer ullamcorper, enim vitae euismod rutrum, massa nisl semper ipsum,\n"
        "vestibulum sodales sem ante in massa.\n"
        "Vestibulum in augue non felis convallis viverra.\n"
        "Mauris ultricies dolor sed massa convallis sed aliquet augue fringilla.\n"
        "Duis erat eros, porta in accumsan in, blandit quis sem.\n"
        "In hac habitasse platea dictumst. Etiam fringilla est id odio dapibus sit amet semper dui laoreet.\n"
    );

    mText.setFont(mFont);
    mText.setCharacterSize(22);
    mText.setPosition(30, 20);

    // Load the shader
    if (!mShader.loadFromFile("resources/wave.vert", "resources/blur.frag"))
        return false;

    return true;
}


void WaveBlur::onUpdate(const sf::Time& time, float x, float y)
{
    mShader.setUniform("wave_phase", time.asSeconds());
    mShader.setUniform("wave_amplitude", sf::Vector2f(x * 40, y * 40));
    mShader.setUniform("blur_radius", (x + y) * 0.008f);
}


void WaveBlur::onDraw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.shader = &mShader;
    target.draw(mText, states);
}
