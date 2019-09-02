#pragma once

#include "SFML/Graphics.hpp"

#include <functional>
#include <vector>


struct TmxObject;


class Checkpoint
{

public:

    using CheckpointCallback = std::function<void(void)>;

    Checkpoint* getCheckpoint(int32_t index);

    static void add(TmxObject*);
    static void update();
    static void resetAll();

    void reached();


private:

    Checkpoint() = default;

    int32_t mIndex = 0;
    sf::IntRect mRect;
    bool mReached = false;
    CheckpointCallback mCallback;

    static std::vector<Checkpoint> sCheckpoints;
};

