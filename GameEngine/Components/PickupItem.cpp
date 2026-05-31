#include "PickupItem.h"

#include "Resources/Mesh.h"

PickupItem::~PickupItem()
{
    delete ownedMesh;
    ownedMesh = nullptr;
}
