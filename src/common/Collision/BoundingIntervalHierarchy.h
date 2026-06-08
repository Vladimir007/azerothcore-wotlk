#ifndef BIH_H
#define BIH_H

#include <cmath>
#include <cstring>
#include <vector>
#include <G3D/AABox.h>
#include <G3D/Ray.h>
#include <G3D/Vector3.h>

#include "Define.h"

#define MAX_STACK_SIZE 64

struct AABound
{
    G3D::Vector3 lo, hi;
};

class BIH
{
    void initEmpty()
    {
        tree.clear();
        objects.clear();
        bounds = G3D::AABox::empty();
        // Create space for the first node
        tree.push_back(3u << 30u); // Dummy leaf
        tree.insert(tree.end(), 2, 0);
    }

    // https://stackoverflow.com/a/4328396
    static uint32 floatToRawIntBits(const float f)
    {
        static_assert(sizeof(float) == sizeof(uint32), "Size of uint32 and float must be equal for this to work");
        uint32 ret;
        memcpy(&ret, &f, sizeof(float));
        return ret;
    }

    static float intBitsToFloat(const uint32 i)
    {
        static_assert(sizeof(float) == sizeof(uint32), "Size of uint32 and float must be equal for this to work");
        float ret;
        memcpy(&ret, &i, sizeof(uint32));
        return ret;
    }

public:
    BIH() { initEmpty(); }

    template<class BoundsFunc, class PrimArray>
    void build(const PrimArray& primitives, BoundsFunc& GetBounds, uint32 leafSize = 3);

    [[nodiscard]] uint32 primCount() const { return objects.size(); }
    G3D::AABox const& bound() const { return bounds; }

    bool writeToFile(FILE* wf) const;
    bool readFromFile(FILE* rf);

    template<typename RayCallback>
    bool intersectRay(const G3D::Ray& r, RayCallback& intersectCallback, float& maxDist, bool stopAtFirstHit) const;

    template<typename IsectCallback>
    bool intersectPoint(const G3D::Vector3& p, IsectCallback& intersectCallback) const;

protected:
    std::vector<uint32> tree;
    std::vector<uint32> objects;
    G3D::AABox bounds;

    struct buildData
    {
        uint32* indices;
        G3D::AABox* primBound;
        uint32 numPrims;
        int maxPrims;
    };
    struct StackNode
    {
        uint32 node;
        float tnear;
        float tfar;
    };

    void buildHierarchy(std::vector<uint32>& tempTree, buildData& dat);

    static void createNode(std::vector<uint32>& tempTree, const int nodeIndex, const uint32 left, const uint32 right)
    {
        // Write leaf node
        tempTree[nodeIndex + 0] = (3 << 30) | left;
        tempTree[nodeIndex + 1] = right - left + 1;
    }

    static void subdivide(int left, int right, std::vector<uint32>& tempTree, buildData& dat, AABound& gridBox, AABound& nodeBox, int nodeIndex, int depth);
};

#endif
