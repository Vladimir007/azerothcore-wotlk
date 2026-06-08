#ifndef INTERMEDIATE_VALUES_H
#define INTERMEDIATE_VALUES_H

#include "Recast.h"

namespace MMAP
{
    struct IntermediateValues
    {
        rcHeightfield* heightfield{nullptr};
        rcCompactHeightfield* compactHeightfield{nullptr};
        rcContourSet* contours{nullptr};
        rcPolyMesh* polyMesh{nullptr};
        rcPolyMeshDetail* polyMeshDetail{nullptr};

        IntermediateValues() {}
        ~IntermediateValues()
        {
            rcFreeCompactHeightfield(compactHeightfield);
            rcFreeHeightField(heightfield);
            rcFreeContourSet(contours);
            rcFreePolyMesh(polyMesh);
            rcFreePolyMeshDetail(polyMeshDetail);

            heightfield = nullptr;
            compactHeightfield = nullptr;
            contours = nullptr;
            polyMesh = nullptr;
            polyMeshDetail = nullptr;
        }
    };
}
#endif
