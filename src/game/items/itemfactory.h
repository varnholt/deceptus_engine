#pragma once

#include <memory>
#include <string>

class Item;

class ItemFactory
{
public:
   static std::shared_ptr<Item> create(const std::string& item_name);
};
