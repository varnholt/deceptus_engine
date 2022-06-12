#pragma once

#include <string>

struct TmxObject;

struct TmxTemplate
{
    TmxTemplate(const std::string& filename);

    TmxObject* _object = nullptr;
};
