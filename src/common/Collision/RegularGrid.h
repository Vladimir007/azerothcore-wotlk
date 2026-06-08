#ifndef REGULAR_GRID_H
#define REGULAR_GRID_H

#include <G3D/Ray.h>
#include <G3D/Table.h>

#include "Errors.h"

template <class Node>
class NodeArray
{
public:
    explicit NodeArray() { memset(&_nodes, 0, sizeof(_nodes)); }
    void AddNode(Node* n)
    {
        for (uint8 i = 0; i < 9; ++i)
        {
            if (_nodes[i] == 0)
            {
                _nodes[i] = n;
                return;
            }
            if (_nodes[i] == n)
                return;
        }
    }
    Node* _nodes[9];
};

template<class Node>
struct NodeCreator
{
    static Node* makeNode(int /*x*/, int /*y*/) { return new Node();}
};

template<class T, class Node, class NodeCreatorFunc = NodeCreator<Node>>
class RegularGrid2D
{
public:
    enum
    {
        CELL_NUMBER = 64,
    };

#define CELL_SIZE 533.33333f

    typedef G3D::Table<const T*, NodeArray<Node>> MemberTable;

    MemberTable memberTable;
    Node* nodes[CELL_NUMBER][CELL_NUMBER];

    RegularGrid2D()
    {
        memset(nodes, 0, sizeof(nodes));
    }

    ~RegularGrid2D()
    {
        for (int x = 0; x < CELL_NUMBER; ++x)
            for (int y = 0; y < CELL_NUMBER; ++y)
                delete nodes[x][y];
    }

    void insert(const T& value)
    {
        G3D::Vector3 pos[9];
        pos[0] = value.GetBounds().corner(0);
        pos[1] = value.GetBounds().corner(1);
        pos[2] = value.GetBounds().corner(2);
        pos[3] = value.GetBounds().corner(3);
        pos[4] = (pos[0] + pos[1]) / 2.0f;
        pos[5] = (pos[1] + pos[2]) / 2.0f;
        pos[6] = (pos[2] + pos[3]) / 2.0f;
        pos[7] = (pos[3] + pos[0]) / 2.0f;
        pos[8] = (pos[0] + pos[2]) / 2.0f;

        NodeArray<Node> na;
        for (uint8 i = 0; i < 9; ++i)
        {
            if (Cell c = Cell::ComputeCell(pos[i].x, pos[i].y); !c.isValid())
                continue;
            Node& node = getGridFor(pos[i].x, pos[i].y);
            na.AddNode(&node);
        }

        for (uint8 i = 0; i < 9; ++i)
        {
            if (na._nodes[i])
                na._nodes[i]->insert(value);
            else
                break;
        }

        memberTable.set(&value, na);
    }

    void remove(const T& value)
    {
        NodeArray<Node>& na = memberTable[&value];
        for (uint8 i = 0; i < 9; ++i)
        {
            if (na._nodes[i])
                na._nodes[i]->remove(value);
            else
                break;
        }

        // Remove the member
        memberTable.remove(&value);
    }

    void balance()
    {
        for (int x = 0; x < CELL_NUMBER; ++x)
            for (int y = 0; y < CELL_NUMBER; ++y)
                if (Node* n = nodes[x][y])
                    n->balance();
    }

    bool contains(const T& value) const { return memberTable.containsKey(&value); }
    int size() const { return memberTable.size(); }

    struct Cell
    {
        int x, y;
        bool operator == (const Cell& c2) const { return x == c2.x && y == c2.y;}

        static Cell ComputeCell(const float fx, const float fy)
        {
            Cell c = {
                static_cast<int>(fx * (1.f / CELL_SIZE) + CELL_NUMBER / 2),
                static_cast<int>(fy * (1.f / CELL_SIZE) + CELL_NUMBER / 2)
            };
            return c;
        }

        bool isValid() const { return x >= 0 && x < CELL_NUMBER && y >= 0 && y < CELL_NUMBER;}
    };

    Node& getGridFor(float fx, float fy)
    {
        Cell c = Cell::ComputeCell(fx, fy);
        return getGrid(c.x, c.y);
    }

    Node& getGrid(int x, int y)
    {
        ASSERT(x < CELL_NUMBER && y < CELL_NUMBER);
        if (!nodes[x][y])
        {
            nodes[x][y] = NodeCreatorFunc::makeNode(x, y);
        }
        return *nodes[x][y];
    }

    template<typename RayCallback>
    bool intersectRay(const G3D::Ray& ray, RayCallback& intersectCallback, float max_dist, bool stopAtFirstHit)
    {
        return intersectRay(ray, intersectCallback, max_dist, ray.origin() + ray.direction() * max_dist, stopAtFirstHit);
    }

    template<typename RayCallback>
    bool intersectRay(const G3D::Ray& ray, RayCallback& intersectCallback, float& max_dist, const G3D::Vector3& end, bool stopAtFirstHit)
    {
        Cell cell = Cell::ComputeCell(ray.origin().x, ray.origin().y);
        if (!cell.isValid())
            return false;

        Cell last_cell = Cell::ComputeCell(end.x, end.y);

        if (cell == last_cell)
        {
            if (Node* node = nodes[cell.x][cell.y])
                return node->intersectRay(ray, intersectCallback, max_dist, stopAtFirstHit);
            return false;
        }

        float voxel = CELL_SIZE;
        const float kx_inv = ray.invDirection().x;
        const float bx = ray.origin().x;
        const float ky_inv = ray.invDirection().y;
        const float by = ray.origin().y;

        int stepX, stepY;
        float tMaxX, tMaxY;
        if (kx_inv >= 0)
        {
            stepX = 1;
            const float x_border = (cell.x + 1) * voxel;
            tMaxX = (x_border - bx) * kx_inv;
        }
        else
        {
            stepX = -1;
            const float x_border = (cell.x - 1) * voxel;
            tMaxX = (x_border - bx) * kx_inv;
        }

        if (ky_inv >= 0)
        {
            stepY = 1;
            const float y_border = (cell.y + 1) * voxel;
            tMaxY = (y_border - by) * ky_inv;
        }
        else
        {
            stepY = -1;
            const float y_border = (cell.y - 1) * voxel;
            tMaxY = (y_border - by) * ky_inv;
        }

        const float tDeltaX = voxel * std::fabs(kx_inv);
        const float tDeltaY = voxel * std::fabs(ky_inv);

        bool result = false;
        do
        {
            if (Node* node = nodes[cell.x][cell.y])
                result = node->intersectRay(ray, intersectCallback, max_dist, stopAtFirstHit);
            if (cell == last_cell)
                break;
            if (tMaxX < tMaxY)
            {
                tMaxX += tDeltaX;
                cell.x += stepX;
            }
            else
            {
                tMaxY += tDeltaY;
                cell.y += stepY;
            }
        } while (cell.isValid());
        return result;
    }

    template<typename IsectCallback>
    bool intersectPoint(const G3D::Vector3& point, IsectCallback& intersectCallback)
    {
        Cell cell = Cell::ComputeCell(point.x, point.y);
        if (!cell.isValid())
            return false;
        if (Node* node = nodes[cell.x][cell.y])
            return node->intersectPoint(point, intersectCallback);
        return false;
    }

    // Optimized version of intersectRay function for rays with vertical directions
    template<typename RayCallback>
    bool intersectZAlignedRay(const G3D::Ray& ray, RayCallback& intersectCallback, float& max_dist)
    {
        Cell cell = Cell::ComputeCell(ray.origin().x, ray.origin().y);
        if (!cell.isValid())
            return false;
        if (Node* node = nodes[cell.x][cell.y])
            return node->intersectRay(ray, intersectCallback, max_dist, false);
        return false;
    }
};

#undef CELL_SIZE

#endif
