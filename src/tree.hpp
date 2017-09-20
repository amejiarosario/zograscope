#ifndef TREE_HPP__
#define TREE_HPP__

#include <string>
#include <vector>

#include "TreeBuilder.hpp"
#include "types.hpp"

enum class State
{
    Unchanged,
    Deleted,
    Inserted,
    Updated
};

class SNode;

struct Node
{
    std::string label;
    std::string spelling;
    std::vector<Node *> children;
    int poID = -1; // post-order ID
    State state = State::Unchanged;
    int line = 0;
    int col = 0;
    Node *relative = nullptr;
    Node *parent = nullptr;
    bool satellite = false;
    Type type = Type::Virtual;
    SType stype = SType::None;
    Node *next = nullptr;
    int valueChild = -1;
    bool moved = false;
    bool last = false;
};

class Tree
{
public:
    Tree() = default;
    Tree(const std::string &contents, const PNode *node);
    Tree(const std::string &contents, const SNode *node);

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
    std::deque<Node> nodes;
    Node *root = nullptr;
};

void print(const Node &node);

std::vector<Node *> postOrder(Node &root);

void reduceTreesCoarse(Node *T1, Node *T2);

std::string printSubTree(const Node &root);

bool isUnmovable(const Node *x);

bool hasMoveableItems(const Node *x);

void markTreeAsMoved(Node *node);

#endif // TREE_HPP__
