#include "BoundingIntervalHierarchy.h"
#include <algorithm>
#include <stdexcept>

#define isnan std::isnan

template <class BoundsFunc, class PrimArray>
void BIH::build(const PrimArray& primitives, BoundsFunc& GetBounds, const uint32 leafSize)
{
    if (primitives.size() == 0)
    {
        initEmpty();
        return;
    }

    buildData dat;
    dat.maxPrims = leafSize;
    dat.numPrims = primitives.size();
    dat.indices = new uint32[dat.numPrims];
    dat.primBound = new G3D::AABox[dat.numPrims];
    GetBounds(primitives[0], bounds);
    for (uint32 i = 0; i < dat.numPrims; ++i)
    {
        dat.indices[i] = i;
        GetBounds(primitives[i], dat.primBound[i]);
        bounds.merge(dat.primBound[i]);
    }
    std::vector<uint32> tempTree;
    buildHierarchy(tempTree, dat);

    objects.resize(dat.numPrims);
    for (uint32 i = 0; i < dat.numPrims; ++i)
    {
        objects[i] = dat.indices[i];
    }
    tree = tempTree;
    delete[] dat.primBound;
    delete[] dat.indices;
}

void BIH::buildHierarchy(std::vector<uint32>& tempTree, buildData& dat)
{
    // Create space for the first node
    tempTree.push_back(static_cast<uint32>(3 << 30));  // Dummy leaf
    tempTree.insert(tempTree.end(), 2, 0);

    // Seed bbox
    AABound gridBox = { bounds.low(), bounds.high() };
    AABound nodeBox = gridBox;

    // Seed subdivide function
    subdivide(0, dat.numPrims - 1, tempTree, dat, gridBox, nodeBox, 0, 1);
}

void BIH::subdivide(const int left, int right, std::vector<uint32>& tempTree, buildData& dat, AABound& gridBox, AABound& nodeBox, int nodeIndex, int depth)
{
    if (right - left + 1 <= dat.maxPrims || depth >= MAX_STACK_SIZE)
    {
        // Write leaf node
        createNode(tempTree, nodeIndex, left, right);
        return;
    }

    // Calculate extents
    int axis = -1, rightOrig;
    float clipL, clipR;
    float prevClip = G3D::fnan();
    float split = G3D::fnan();
    bool wasLeft = true;
    while (true)
    {
        const int prevAxis = axis;
        const float prevSplit = split;

        // Perform quick consistency checks
        G3D::Vector3 d( gridBox.hi - gridBox.lo );
        if (d.x < 0 || d.y < 0 || d.z < 0)
            throw std::logic_error("negative node extents");

        for (int i = 0; i < 3; i++)
            if (nodeBox.hi[i] < gridBox.lo[i] || nodeBox.lo[i] > gridBox.hi[i])
                throw std::logic_error("invalid node overlap");

        // Find longest axis
        axis = d.primaryAxis();
        split = 0.5f * (gridBox.lo[axis] + gridBox.hi[axis]);
        // Partition L/R subsets
        clipL = -G3D::inf();
        clipR = G3D::inf();
        rightOrig = right; // Save this for later
        float nodeL = G3D::inf();
        float nodeR = -G3D::inf();
        for (int i = left; i <= right;)
        {
            const int obj = dat.indices[i];
            float minb = dat.primBound[obj].low()[axis];
            float maxb = dat.primBound[obj].high()[axis];
            if (const float center = (minb + maxb) * 0.5f; center <= split)
            {
                // Stay left
                i++;
                if (clipL < maxb)
                    clipL = maxb;
            }
            else
            {
                // Move to the right most
                const int t = dat.indices[i];
                dat.indices[i] = dat.indices[right];
                dat.indices[right] = t;
                right--;
                if (clipR > minb)
                    clipR = minb;
            }
            nodeL = std::min(nodeL, minb);
            nodeR = std::max(nodeR, maxb);
        }

        // Check for empty space
        if (nodeL > nodeBox.lo[axis] && nodeR < nodeBox.hi[axis])
        {
            const float nodeBoxW = nodeBox.hi[axis] - nodeBox.lo[axis];
            const float nodeNewW = nodeR - nodeL;
            // Node box is too big compare to space occupied by primitives?
            if (1.3f * nodeNewW < nodeBoxW)
            {
                const int nextIndex = tempTree.size();
                // Allocate child
                tempTree.push_back(0);
                tempTree.push_back(0);
                tempTree.push_back(0);
                // Write bvh2 clip node
                tempTree[nodeIndex + 0] = (axis << 30) | (1 << 29) | nextIndex;
                tempTree[nodeIndex + 1] = floatToRawIntBits(nodeL);
                tempTree[nodeIndex + 2] = floatToRawIntBits(nodeR);
                // Update nodebox and recurse
                nodeBox.lo[axis] = nodeL;
                nodeBox.hi[axis] = nodeR;
                subdivide(left, rightOrig, tempTree, dat, gridBox, nodeBox, nextIndex, depth + 1);
                return;
            }
        }
        // Ensure we are making progress in the subdivision
        if (right == rightOrig)
        {
            // All left
            if (prevAxis == axis && G3D::fuzzyEq(prevSplit, split))
            {
                // We are stuck here - create a leaf
                createNode(tempTree, nodeIndex, left, right);
                return;
            }
            if (clipL <= split)
            {
                // Keep looping on left half
                gridBox.hi[axis] = split;
                prevClip = clipL;
                wasLeft = true;
                continue;
            }
            gridBox.hi[axis] = split;
            prevClip = G3D::fnan();
        }
        else if (left > right)
        {
            // All right
            right = rightOrig;
            if (prevAxis == axis && G3D::fuzzyEq(prevSplit, split))
            {
                // We are stuck here - create a leaf
                createNode(tempTree, nodeIndex, left, right);
                return;
            }
            if (clipR >= split)
            {
                // Keep looping on right half
                gridBox.lo[axis] = split;
                prevClip = clipR;
                wasLeft = false;
                continue;
            }
            gridBox.lo[axis] = split;
            prevClip = G3D::fnan();
        }
        else
        {
            // We are actually splitting stuff
            if (prevAxis != -1 && !isnan(prevClip))
            {
                // second time through - lets create the previous split
                // since it produced empty space
                const int nextIndex = tempTree.size();
                // allocate child node
                tempTree.push_back(0);
                tempTree.push_back(0);
                tempTree.push_back(0);
                if (wasLeft)
                {
                    // Create a node with a left child
                    // Write leaf node
                    tempTree[nodeIndex + 0] = (prevAxis << 30) | nextIndex;
                    tempTree[nodeIndex + 1] = floatToRawIntBits(prevClip);
                    tempTree[nodeIndex + 2] = floatToRawIntBits(G3D::inf());
                }
                else
                {
                    // Create a node with a right child
                    // Write leaf node
                    tempTree[nodeIndex + 0] = (prevAxis << 30) | (nextIndex - 3);
                    tempTree[nodeIndex + 1] = floatToRawIntBits(-G3D::inf());
                    tempTree[nodeIndex + 2] = floatToRawIntBits(prevClip);
                }
                // Count stats for the unused leaf
                depth++;
                // Now we keep going as we are, with a new nodeIndex:
                nodeIndex = nextIndex;
            }
            break;
        }
    }
    // Compute index of child nodes
    int nextIndex = tempTree.size();
    // allocate left node
    const int nl = right - left + 1;
    const int nr = rightOrig - (right + 1) + 1;
    if (nl > 0)
    {
        tempTree.push_back(0);
        tempTree.push_back(0);
        tempTree.push_back(0);
    }
    else
        nextIndex -= 3;
    // Allocate right node
    if (nr > 0)
    {
        tempTree.push_back(0);
        tempTree.push_back(0);
        tempTree.push_back(0);
    }
    // Write leaf node
    tempTree[nodeIndex + 0] = (axis << 30) | nextIndex;
    tempTree[nodeIndex + 1] = floatToRawIntBits(clipL);
    tempTree[nodeIndex + 2] = floatToRawIntBits(clipR);
    // Prepare L/R child boxes
    AABound gridBoxL(gridBox), gridBoxR(gridBox);
    AABound nodeBoxL(nodeBox), nodeBoxR(nodeBox);
    gridBoxL.hi[axis] = gridBoxR.lo[axis] = split;
    nodeBoxL.hi[axis] = clipL;
    nodeBoxR.lo[axis] = clipR;
    // Recurse
    if (nl > 0)
        subdivide(left, right, tempTree, dat, gridBoxL, nodeBoxL, nextIndex, depth + 1);
    if (nr > 0)
        subdivide(right + 1, rightOrig, tempTree, dat, gridBoxR, nodeBoxR, nextIndex + 3, depth + 1);
}

bool BIH::writeToFile(FILE* wf) const
{
    const uint32 treeSize = tree.size();
    uint32 check = 0, count;
    check += fwrite(&bounds.low(), sizeof(float), 3, wf);
    check += fwrite(&bounds.high(), sizeof(float), 3, wf);
    check += fwrite(&treeSize, sizeof(uint32), 1, wf);
    check += fwrite(&tree[0], sizeof(uint32), treeSize, wf);
    count = objects.size();
    check += fwrite(&count, sizeof(uint32), 1, wf);
    check += fwrite(&objects[0], sizeof(uint32), count, wf);
    return check == 3 + 3 + 2 + treeSize + count;
}

bool BIH::readFromFile(FILE* rf)
{
    uint32 treeSize;
    G3D::Vector3 lo, hi;
    uint32 check = 0, count = 0;
    check += fread(&lo, sizeof(float), 3, rf);
    check += fread(&hi, sizeof(float), 3, rf);
    bounds = G3D::AABox(lo, hi);
    check += fread(&treeSize, sizeof(uint32), 1, rf);
    tree.resize(treeSize);
    check += fread(&tree[0], sizeof(uint32), treeSize, rf);
    check += fread(&count, sizeof(uint32), 1, rf);
    objects.resize(count); // = new uint32[nObjects];
    check += fread(&objects[0], sizeof(uint32), count, rf);
    return static_cast<uint64>(check) == 3 + 3 + 1 + 1 + static_cast<uint64>(treeSize) + static_cast<uint64>(count);
}

template <typename RayCallback>
bool BIH::intersectRay(const G3D::Ray& r, RayCallback& intersectCallback, float& maxDist, bool stopAtFirstHit) const
{
    float intervalMin = -1.f;
    float intervalMax = -1.f;
    G3D::Vector3 org = r.origin();
    G3D::Vector3 dir = r.direction();
    G3D::Vector3 invDir;
    for (int i = 0; i < 3; ++i)
    {
        invDir[i] = 1.f / dir[i];
        if (G3D::fuzzyNe(dir[i], 0.0f))
        {
            float t1 = (bounds.low()[i]  - org[i]) * invDir[i];
            float t2 = (bounds.high()[i] - org[i]) * invDir[i];
            if (t1 > t2)
                std::swap(t1, t2);
            if (t1 > intervalMin)
                intervalMin = t1;
            if (t2 < intervalMax || intervalMax < 0.f)
                intervalMax = t2;
            // intervalMax can only become smaller for other axis, and intervalMin only larger respectively, so stop early
            if (intervalMax <= 0 || intervalMin >= maxDist)
                return false;
        }
    }

    if (intervalMin > intervalMax)
        return false;

    intervalMin = std::max(intervalMin, 0.f);
    intervalMax = std::min(intervalMax, maxDist);

    uint32 offsetFront[3];
    uint32 offsetBack[3];
    uint32 offsetFront3[3];
    uint32 offsetBack3[3];

    // Compute custom offsets from direction sign bit
    for (int i = 0; i < 3; ++i)
    {
        offsetFront[i] = floatToRawIntBits(dir[i]) >> 31;
        offsetBack[i] = offsetFront[i] ^ 1;
        offsetFront3[i] = offsetFront[i] * 3;
        offsetBack3[i] = offsetBack[i] * 3;

        // Avoid always adding 1 during the inner loop
        ++offsetFront[i];
        ++offsetBack[i];
    }

    StackNode stack[MAX_STACK_SIZE];
    int stackPos = 0;
    int node = 0;

    bool result = false;
    while (true)
    {
        while (true)
        {
            const uint32 tn = tree[node];
            const uint32 axis = (tn & (3 << 30)) >> 30;
            const bool BVH2 = tn & (1 << 29);
            int offset = tn & ~(7 << 29);
            if (!BVH2)
            {
                if (axis < 3)
                {
                    // "normal" interior node
                    float tf = (intBitsToFloat(tree[node + offsetFront[axis]]) - org[axis]) * invDir[axis];
                    float tb = (intBitsToFloat(tree[node + offsetBack[axis]]) - org[axis]) * invDir[axis];
                    // ray passes between clip zones
                    if (tf < intervalMin && tb > intervalMax)
                        break;
                    const int back = offset + offsetBack3[axis];
                    node = back;

                    // Ray passes through far node only
                    if (tf < intervalMin)
                    {
                        intervalMin = tb >= intervalMin ? tb : intervalMin;
                        continue;
                    }
                    node = offset + offsetFront3[axis]; // Front

                    // Ray passes through near node only
                    if (tb > intervalMax)
                    {
                        intervalMax = (tf <= intervalMax) ? tf : intervalMax;
                        continue;
                    }

                    // Ray passes through both nodes
                    // Push back node
                    stack[stackPos].node = back;
                    stack[stackPos].tnear = tb >= intervalMin ? tb : intervalMin;
                    stack[stackPos].tfar = intervalMax;
                    stackPos++;

                    // Update ray interval for front node
                    intervalMax = tf <= intervalMax ? tf : intervalMax;
                    continue;
                }

                // Leaf - test some objects
                int n = tree[node + 1];
                while (n > 0)
                {
                    if (intersectCallback(r, objects[offset], maxDist, stopAtFirstHit))
                    {
                        result = true;
                        if (stopAtFirstHit)
                            return result;
                    }
                    --n;
                    ++offset;
                }
                break;
            }

            if (axis > 2)
                return result;  // Should not happen
            float tf = (intBitsToFloat(tree[node + offsetFront[axis]]) - org[axis]) * invDir[axis];
            float tb = (intBitsToFloat(tree[node + offsetBack[axis]]) - org[axis]) * invDir[axis];
            node = offset;
            intervalMin = tf >= intervalMin ? tf : intervalMin;
            intervalMax = tb <= intervalMax ? tb : intervalMax;
            if (intervalMin > intervalMax)
                break;
        }

        do
        {
            // Stack is empty?
            if (stackPos == 0)
                return result;

            // Move back up the stack
            stackPos--;
            intervalMin = stack[stackPos].tnear;
            if (maxDist < intervalMin)
                continue;
            node = stack[stackPos].node;
            intervalMax = stack[stackPos].tfar;
            break;
        } while (true);
    }
}

template <typename IsectCallback>
bool BIH::intersectPoint(const G3D::Vector3& p, IsectCallback& intersectCallback) const
{
    if (!bounds.contains(p))
        return false;

    bool result = false;

    StackNode stack[MAX_STACK_SIZE];
    int stackPos = 0;
    int node = 0;

    while (true)
    {
        while (true)
        {
            const uint32 tn = tree[node];
            const uint32 axis = (tn & (3 << 30)) >> 30;
            const bool BVH2 = tn & (1 << 29);
            int offset = tn & ~(7 << 29);
            if (!BVH2)
            {
                if (axis < 3)
                {
                    // "normal" interior node
                    const float tl = intBitsToFloat(tree[node + 1]);
                    const float tr = intBitsToFloat(tree[node + 2]);

                    // Point is between clip zones
                    if (tl < p[axis] && tr > p[axis])
                        break;

                    const int right = offset + 3;
                    node = right;

                    // Point is in right node only
                    if (tl < p[axis])
                        continue;

                    node = offset; // Left

                    // Point is in left node only
                    if (tr > p[axis])
                        continue;

                    // Point is in both nodes
                    // Push back right node
                    stack[stackPos].node = right;
                    stackPos++;
                    continue;
                }

                // Leaf - test some objects
                int n = tree[node + 1];
                while (n > 0)
                {
                    if (intersectCallback(p, objects[offset]))
                        result = true;
                    --n;
                    ++offset;
                }
                break;
            }

            // BVH2 node (empty space cut off left and right)
            if (axis > 2)
                return result;  // Should not happen

            const float tl = intBitsToFloat(tree[node + 1]);
            const float tr = intBitsToFloat(tree[node + 2]);
            node = offset;
            if (tl > p[axis] || tr < p[axis])
                break;
        }

        // Stack is empty?
        if (stackPos == 0)
            return result;

        // Move back up the stack
        stackPos--;
        node = stack[stackPos].node;
    }
}
