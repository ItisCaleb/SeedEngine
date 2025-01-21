#ifndef QUAD_TREE_H_
#define QUAD_TREE_H_

#include <vector>
#include <set>

#include "aabb.h"
#include "core/entity.h"
#include "core/container/freelist.h"

namespace Seed {
struct OcElement {
    // points to next element if this is a branch
    // else -1
    int next;

    // set to -1 if this is a branch
    // else it will represent element count
    int shapeIdx;
};

struct OcNode {
    // points to first child if this is a branch
    // else first element
    // if no element then -1
    int next;

    // set to -1 if this is a branch
    // else it will represent element count
    int count;
};

// use in node find
struct OcNodeData {
    AABB boundary;
    int nodeIdx;
    int depth;
};

class OcTree {
   public:
    OcTree(int w, int h, int d, int max_depth, int split_threshold);
    void insert(AABB *shape);
    void erase(AABB *shape);
    void query(AABB *shape, std::vector<AABB *> &collides);
    void cleanup();
    std::vector<AABB *> &getAllShape();

   private:
    void findNodes(AABB *shape, std::vector<OcNodeData> &nodes);
    void subDivide(OcNodeData &data);
    void appendToElements(OcNodeData &data, int shapdIdx);
    inline void getNodeShapes(int eleIdx, std::vector<AABB *> &shape,
                              std::set<int> &pushed) {
        for (int i = eleIdx; i != -1;) {
            auto ele = this->elements[i];
            int shapeIdx = ele.shapeIdx;
            i = ele.next;
            if (!pushed.count(shapeIdx)) {
                shape.push_back(this->shapes[shapeIdx]);
                pushed.insert(shapeIdx);
            }
        }
    }
    FreeList<AABB *> shapes;
    FreeList<OcElement> elements;
    std::vector<OcNode> nodes;
    AABB boundary;
    int free_node = -1;
    int max_depth;
    int split_threshold;

    // for search
    std::vector<OcNodeData> nodeData;

    std::vector<AABB *> leaves;
};
}  // namespace Seed

#endif