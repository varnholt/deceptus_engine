#pragma once

#include <memory>
#include <string>

struct TmxObject;
struct TmxParseData;

struct TmxTemplate
{
    TmxTemplate(const std::string& filename, const std::shared_ptr<TmxParseData>&);

    TmxObject* _object = nullptr;
};
