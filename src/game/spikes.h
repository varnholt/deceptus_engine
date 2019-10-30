#pragma once

class Spikes
{
public:

    enum class Mode
    {
        Invalid,
        Trap,
        Interval,
        Toggled,
    };

    Spikes();


private:

    Mode mMode = Mode::Invalid;
};

