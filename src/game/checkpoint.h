#pragma once

#include "SFML/Graphics.hpp"

#include <functional>
#include <vector>


struct TmxObject;


class Checkpoint
{

public:

    using CheckpointCallback = std::function<void(void)>;

    static Checkpoint* getCheckpoint(uint32_t index);
    static uint32_t add(TmxObject*);
    static void update();
    static void resetAll();

    void reached();
    void addCallback(CheckpointCallback);
    sf::Vector2i calcCenter() const;


private:

    Checkpoint() = default;

    uint32_t mIndex = 0;
    std::string mName;

    sf::IntRect mRect;
    bool mReached = false;

    std::vector<CheckpointCallback> mCallbacks;

    static std::vector<Checkpoint> sCheckpoints;
};

