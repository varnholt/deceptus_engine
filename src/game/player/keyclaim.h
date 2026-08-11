#ifndef KEYCLAIM_H
#define KEYCLAIM_H

#include "game/constants.h"

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

/// \brief keeps track of which input keys currently belong to an owner.
/// \note the point of a claim is that a key can have exactly one owner at a time: while a key is
///       claimed, every regular input query reads it as released, so features that do not own it
///       cannot act on it and do not have to know that the owner exists. reading a key past its claim
///       is a capability of holding the claim rather than a call anyone can make, which is why the
///       axis readers are installed here by whoever owns the input state and are only reachable
///       through KeyClaim.
class KeyClaimRegistry
{
public:
   using AxisReader = std::function<float()>;

   /// \brief creates a registry over the two axis readers a claim holder may use.
   /// \param vertical_axis_reader reads the vertical input axis, negative points up.
   /// \param horizontal_axis_reader reads the horizontal controller axis.
   KeyClaimRegistry(AxisReader vertical_axis_reader, AxisReader horizontal_axis_reader);

   /// \brief determines whether a key currently belongs to an owner.
   /// \param key key to query.
   /// \return true when the key is claimed and reads as released for everyone but its owner.
   bool isClaimed(KeyPressed key) const;

   /// \brief adds one claim on each of the given keys.
   /// \param keys keys to claim.
   void claim(const std::vector<KeyPressed>& keys);

   /// \brief removes one claim from each of the given keys.
   /// \param keys keys to give back.
   void unclaim(const std::vector<KeyPressed>& keys);

   /// \brief reads the vertical input axis, ignoring claims.
   /// \return normalized vertical input in the range -1 to 1.
   float readVerticalAxis() const;

   /// \brief reads the horizontal controller axis, ignoring claims.
   /// \return normalized horizontal input in the range -1 to 1.
   float readHorizontalAxis() const;

private:
   AxisReader _vertical_axis_reader;
   AxisReader _horizontal_axis_reader;
   std::unordered_map<KeyPressed, int32_t> _claims;  //!< claim count per key, a key reads as released while claimed
};

/// \brief scoped ownership of a set of input keys, and the only way to read them past the claim.
class KeyClaim
{
public:
   /// \brief creates an empty claim that owns no keys.
   KeyClaim() = default;

   /// \brief claims the given keys until this object goes out of scope.
   /// \param registry registry the keys are claimed in.
   /// \param keys keys taken over by the owner.
   KeyClaim(const std::shared_ptr<KeyClaimRegistry>& registry, const std::vector<KeyPressed>& keys);

   ~KeyClaim();

   KeyClaim(const KeyClaim&) = delete;
   KeyClaim& operator=(const KeyClaim&) = delete;
   KeyClaim(KeyClaim&& other) noexcept;
   KeyClaim& operator=(KeyClaim&& other) noexcept;

   /// \brief reports whether this claim currently owns any keys.
   /// \return true when keys are claimed.
   bool isActive() const;

   /// \brief reads the vertical input axis past the claim, negative points up.
   /// \return normalized vertical input in the range -1 to 1, or 0 while this claim owns nothing.
   float readVerticalAxis() const;

   /// \brief reads the horizontal controller axis, paired with the vertical one for analogue aiming.
   /// \return normalized horizontal input in the range -1 to 1, or 0 while this claim owns nothing.
   float readHorizontalAxis() const;

private:
   /// \brief gives the claimed keys back and empties this claim.
   void release();

   std::weak_ptr<KeyClaimRegistry> _registry;
   std::vector<KeyPressed> _keys;
};

#endif  // KEYCLAIM_H
