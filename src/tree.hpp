#ifndef TREE_HPP__
#define TREE_HPP__

#include <string>
#include <vector>

#include "pmr/pmr_deque.hpp"
#include "pmr/pmr_vector.hpp"

#include "types.hpp"

enum class State
{
    Unchanged,
    Deleted,
    Inserted,
    Updated
};

class PNode;
class SNode;

enum class SType;

struct Node
{
    using allocator_type = cpp17::pmr::polymorphic_allocator<cpp17::byte>;

    std::string label;
    std::string spelling;
    cpp17::pmr::vector<Node *> children;
    int poID = -1; // post-order ID
    State state = State::Unchanged;
    int line = 0;
    int col = 0;
    Node *relative = nullptr;
    Node *parent = nullptr;
    bool satellite = false;
    Type type = Type::Virtual;
    SType stype = {};
    Node *next = nullptr;
    int valueChild = -1;
    bool moved = false;
    bool last = false;

    Node(allocator_type al = {}) : children(al)
    {
    }
    Node(const Node &rhs) = delete;
    Node(Node &&rhs) = default;
    Node(Node &&rhs, allocator_type al = {})
        : children(std::move(rhs.children), al)
    {
    }
    Node & operator=(const Node &rhs) = delete;
    Node & operator=(Node &&rhs) = default;

    bool hasValue() const
    {
        return (valueChild >= 0);
    }

    Node * getValue()
    {
        return (valueChild >= 0 ? children[valueChild] : nullptr);
    }

    const Node * getValue() const
    {
        return (valueChild >= 0 ? children[valueChild] : nullptr);
    }
};

class Tree
{
    using allocator_type = cpp17::pmr::polymorphic_allocator<cpp17::byte>;

public:
    Tree(allocator_type al = {}) : nodes(al)
    {
    }
    Tree(const Tree &rhs) = delete;
    Tree(Tree &&rhs) = default;
    Tree(const std::string &contents, const PNode *node,
         allocator_type al = {});
    Tree(const std::string &contents, const SNode *node,
         allocator_type al = {});

    Tree & operator=(const Tree &rhs) = delete;
    Tree & operator=(Tree &&rhs) = default;

public:
    Node * getRoot()
    {
        return root;
    }

    const Node * getRoot() const
    {
        return root;
    }

    Node & makeNode()
    {
        nodes.emplace_back();
        return nodes.back();
    }

private:
    cpp17::pmr::deque<Node> nodes;
    Node *root = nullptr;
};

void print(const Node &node);

std::vector<Node *> postOrder(Node &root);

void reduceTreesCoarse(Node *T1, Node *T2);

std::string printSubTree(const Node &root, bool withComments);

bool canBeFlattened(const Node *parent, const Node *child, int level);

bool isUnmovable(const Node *x);

bool hasMoveableItems(const Node *x);

bool isContainer(const Node *x);

// Checks whether the node enforces fixed structure (fixed number of children at
// particular places).
bool hasFixedStructure(const Node *x);

// For children of nodes with fixed structure this checks whether this child is
// first-class member of the structure or not (e.g., not punctuation).
bool isPayloadOfFixed(const Node *x);

// Checks whether node doesn't have fixed position within a tree and can move
// between internal nodes as long as post-order of leafs is preserved.
bool isTravellingNode(const Node *x);

bool canForceLeafMatch(const Node *x, const Node *y);

void markTreeAsMoved(Node *node);

#endif // TREE_HPP__
