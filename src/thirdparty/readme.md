# Required Patches

## Box2D

### `b2Settings.h`

Make sure that is `b2_maxPolygonVertices` set to `8`.

```cpp
/// The maximum number of vertices on a convex polygon. You cannot increase
/// this too much because b2BlockAllocator has a maximum object size.
#define b2_maxPolygonVertices 8
```

Change `b2_velocityThreshold` to `0.1`.

```cpp
#define b2_velocityThreshold 0.1f
```


### `b2Contact.cpp`

Disable the re-enabling of the contact for each update cycle as follows:

```cpp
void b2Contact::Update(b2ContactListener* listener)
{
   b2Manifold oldManifold = m_manifold;

   // Re-enable this contact.
   // m_flags |= e_enabledFlag;

```

