#pragma once

#include <cassert>
#include <vector>

namespace PathMerge
{

/// \brief A flat, resizable buffer backed by std::vector.
template <typename T>
class DataBuffer
{
public:
   explicit DataBuffer(int reserve_count = 0)
   {
      if (reserve_count > 0)
      {
         _data.reserve(static_cast<size_t>(reserve_count));
      }
   }

   void reset()
   {
      _data.clear();
   }
   [[nodiscard]] bool isEmpty() const
   {
      return _data.empty();
   }
   [[nodiscard]] int size() const
   {
      return static_cast<int>(_data.size());
   }
   [[nodiscard]] T* data()
   {
      return _data.data();
   }
   [[nodiscard]] const T* data() const
   {
      return _data.data();
   }

   [[nodiscard]] T& at(int index)
   {
      assert(index >= 0 && index < size());
      return _data[index];
   }
   [[nodiscard]] const T& at(int index) const
   {
      assert(index >= 0 && index < size());
      return _data[index];
   }

   [[nodiscard]] T& last()
   {
      assert(!isEmpty());
      return _data.back();
   }
   [[nodiscard]] const T& last() const
   {
      assert(!isEmpty());
      return _data.back();
   }

   [[nodiscard]] T& first()
   {
      assert(!isEmpty());
      return _data.front();
   }
   [[nodiscard]] const T& first() const
   {
      assert(!isEmpty());
      return _data.front();
   }

   auto begin() noexcept
   {
      return _data.begin();
   }
   auto end() noexcept
   {
      return _data.end();
   }
   auto begin() const noexcept
   {
      return _data.cbegin();
   }
   auto end() const noexcept
   {
      return _data.cend();
   }

   void add(const T& value)
   {
      _data.push_back(value);
   }
   void pop_back()
   {
      assert(!isEmpty());
      _data.pop_back();
   }
   void resize(int new_size)
   {
      _data.resize(static_cast<size_t>(new_size));
   }
   void reserve(int new_capacity)
   {
      _data.reserve(static_cast<size_t>(new_capacity));
   }

   DataBuffer& operator<<(const T& value)
   {
      _data.push_back(value);
      return *this;
   }

   void swap(DataBuffer<T>& other)
   {
      _data.swap(other._data);
   }

private:
   std::vector<T> _data;  //!< Underlying storage.
};

}  // namespace PathMerge
