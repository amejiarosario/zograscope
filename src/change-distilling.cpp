#include "change-distilling.hpp"

#include <cmath>

#include <vector>

#include "tree.hpp"
#include "tree-edit-distance.hpp"
#include "utils.hpp"

static void
postOrderAndInit(Node &node, std::vector<Node *> &v)
{
    if (node.satellite) {
        return;
    }

    node.relative = nullptr;
    node.parent = nullptr;

    for (Node *child : node.children) {
        postOrderAndInit(*child, v);
    }
    node.poID = v.size();

    v.push_back(&node);
}

static std::vector<Node *>
postOrderAndInit(Node &root)
{
    std::vector<Node *> v;
    postOrderAndInit(root, v);
    return v;
}

static bool
canMatch(const Node *x, const Node *y)
{
    const Type xType = canonizeType(x->type);
    const Type yType = canonizeType(y->type);

    if (xType != Type::Virtual && xType == yType && x->label == y->label) {
        return true;
    }

    if (xType >= Type::NonInterchangeable ||
        yType >= Type::NonInterchangeable ||
        xType != yType) {
        return false;
    }

    if (xType == Type::Virtual && x->stype != y->stype) {
        return false;
    }

    return true;
}

static void
clear(Node *node)
{
    if (node->satellite) {
        return;
    }

    node->relative = nullptr;
    node->state = State::Unchanged;

    for (Node *child : node->children) {
        clear(child);
    }
}

static void
unbind(Node *node)
{
    if (node->children.empty()) {
        return;
    }

    if (Node *relative = node->relative) {
        node->relative = nullptr;
        unbind(relative);
    }

    for (Node *child : node->children) {
        unbind(child);
    }
}

static void
markNode(Node &node, State state)
{
    node.state = state;

    State leafState = (state == State::Updated ? State::Unchanged : state);

    for (Node *child : node.children) {
        child->parent = &node;
        if (child->satellite) {
            if (child->stype == SType::None) {
                child->state = leafState;
            } else if (node.valueChild >= 0) {
                child->state = leafState;
            } else if (child->relative == nullptr) {
                child->state = leafState;
            }
        }
    }
}

static int
countLeaves(const Node *node)
{
    if (node->stype == SType::Separator) {
        return 0;
    }

    if (node->children.empty()) {
        return 1;
    }

    int count = 0;
    for (const Node *child : node->children) {
        count += countLeaves(child);
    }
    return count;
}

static int
countSatelliteNodes(const Node *node)
{
    if (node->satellite) {
        return (node->stype == SType::Separator ? 0 : countLeaves(node));
    }

    int count = 0;
    for (const Node *child : node->children) {
        count += countSatelliteNodes(child);
    }
    return count;
}

static bool
alwaysMatches(const Node *node)
{
    return (node->stype == SType::TranslationUnit);
}

void
distill(Node &T1, Node &T2)
{
    struct Match
    {
        Node *x;
        Node *y;
        float similarity;
        mutable int common;
    };

    std::vector<Node *> po1 = postOrderAndInit(T1);
    std::vector<Node *> po2 = postOrderAndInit(T2);

    auto unmatchedInternal = [](Node *node) {
        return !node->children.empty() && node->relative == nullptr;
    };

    auto commonAreaSize = [&](const Match &m) {
        if (m.common >= 0) {
            return m.common;
        }

        int size = 1;
        int i = m.x->poID - 1;
        int j = m.y->poID - 1;
        while (i >= 0 && j >= 0 && po1[i]->label == po2[j]->label) {
            ++size;
            --i;
            --j;
        }
        i = m.x->poID + 1;
        j = m.y->poID + 1;
        while (i < (int)po1.size() && j > (int)po2.size() &&
               po1[i]->label == po2[j]->label) {
            ++size;
            ++i;
            ++j;
        }

        m.common = size;
        return size;
    };

    std::vector<DiceString> dice1;
    dice1.reserve(po1.size());
    for (Node *x : po1) {
        dice1.push_back(x->label);
    }

    std::vector<DiceString> dice2;
    dice2.reserve(po2.size());
    for (Node *x : po2) {
        dice2.push_back(x->label);
    }

    std::vector<Match> matches;

    for (Node *x : po1) {
        if (!x->children.empty()) {
            continue;
        }

        for (Node *y : po2) {
            if (!y->children.empty()) {
                continue;
            }

            if (!canMatch(x, y)) {
                continue;
            }

            const float similarity = dice1[x->poID].compare(dice2[y->poID]);
            if (similarity >= 0.6f ||
                (x->type != Type::Virtual && y->type != Type::Virtual &&
                 x->children.empty() && y->children.empty())) {
                matches.push_back({ x, y, similarity, -1 });
            }
        }
    }

    std::stable_sort(matches.begin(), matches.end(),
                     [&](const Match &a, const Match &b) {
                         if (std::fabs(a.similarity - b.similarity) < 0.01f) {
                             return commonAreaSize(b) < commonAreaSize(a);
                         }
                         return b.similarity < a.similarity;
                     });

    std::function<int(const Node *)> lml = [&](const Node *node) {
        if (node->children.empty()) {
            return node->poID;
        }

        for (const Node *child : node->children) {
            if (!child->satellite) {
                return lml(child);
            }
        }

        return node->poID;
    };

    auto distillLeafs = [&]() {
        for (const Match &match : matches) {
            if (match.x->relative != nullptr || match.y->relative != nullptr) {
                continue;
            }

            match.x->relative = match.y;
            match.y->relative = match.x;

            const State state = (match.similarity == 1.0f &&
                                 match.y->label == match.x->label)
                              ? State::Unchanged
                              : State::Updated;
            match.x->state = state;
            match.y->state = state;
        }
    };

    auto distillInternal = [&]() {
        for (Node *x : po1) {
            if (!unmatchedInternal(x)) {
                continue;
            }

            for (Node *y : po2) {
                if (!unmatchedInternal(y) || !canMatch(x, y)) {
                    continue;
                }

                State state;
                if (!alwaysMatches(y)) {
                    const int xFrom = lml(x);
                    int common = 0;
                    int yLeaves = 0;
                    for (int i = lml(y); i < y->poID; ++i) {
                        if (!po2[i]->children.empty()) {
                            continue;
                        }
                        ++yLeaves;

                        if (po2[i]->parent &&
                            po2[i]->parent->relative == nullptr) {
                            // Skip children of unmatched internal nodes.
                            continue;
                        }

                        if (po2[i]->relative == nullptr) {
                            continue;
                        }

                        if (po2[i]->relative->poID >= xFrom &&
                            po2[i]->relative->poID < x->poID) {
                            ++common;
                        }
                    }

                    int xLeaves = std::count_if(&po1[xFrom], &po1[x->poID],
                                                [](const Node *n) {
                                                    return n->children.empty();
                                                });

                    int xExtra = countSatelliteNodes(x);
                    int yExtra = countSatelliteNodes(y);
                    xLeaves += xExtra;
                    yLeaves += yExtra;
                    common += std::min(xExtra, yExtra);

                    float t = (std::min(xLeaves, yLeaves) <= 4) ? 0.4f : 0.6f;

                    float similarity2 =
                        static_cast<float>(common)/std::max(xLeaves, yLeaves);
                    if (similarity2 < t) {
                        continue;
                    }

                    float similarity1 = dice1[x->poID].compare(dice2[y->poID]);
                    if (similarity1 < 0.6f && similarity2 < 0.8f) {
                        continue;
                    }

                    state = (similarity1 == 1.0f &&
                             x->label == y->label &&
                             similarity2 == 1.0f)
                          ? State::Unchanged
                          : State::Updated;
                } else {
                    state = State::Unchanged;
                }

                markNode(*x, state);
                markNode(*y, state);
                x->state = state;
                y->state = state;

                x->relative = y;
                y->relative = x;

                break;
            }
        }
    };

    auto distillInternalExtra = [&]() {
        struct Match
        {
            Node *x;
            Node *y;
            int common;
        };

        std::vector<Match> matches;

        // once we have matched internal nodes properly, do second pass matching
        // internal nodes that have at least one common leaf
        for (Node *x : po1) {
            if (!unmatchedInternal(x)) {
                continue;
            }

            for (Node *y : po2) {
                if (!unmatchedInternal(y) || !canMatch(x, y)) {
                    continue;
                }

                const int xFrom = lml(x);
                int common = 0;
                for (int i = lml(y); i < y->poID; ++i) {
                    if (!po2[i]->children.empty()) {
                        continue;
                    }

                    if (po2[i]->relative == nullptr) {
                        continue;
                    }

                    if (po2[i]->relative->poID >= xFrom &&
                        po2[i]->relative->poID < x->poID) {
                        ++common;
                    }
                }

                const float similarity = dice1[x->poID].compare(dice2[y->poID]);
                if (common > 0 && similarity >= 0.5f) {
                    matches.push_back({ x, y, common });
                }
            }
        }

        std::stable_sort(matches.begin(), matches.end(),
                        [&](const Match &a, const Match &b) {
                            return b.common < a.common;
                        });

        for (const Match &match : matches) {
            if (match.x->relative != nullptr || match.y->relative != nullptr) {
                continue;
            }
            markNode(*match.x, State::Unchanged);
            markNode(*match.y, State::Unchanged);
            match.x->relative = match.y;
            match.y->relative = match.x;
        }
    };

    distillLeafs();
    distillInternal();
    distillInternalExtra();

    clear(&T1);
    clear(&T2);

    std::stable_sort(matches.begin(), matches.end(),
                     [&](const Match &a, const Match &b) {
                         if (std::fabs(a.similarity - b.similarity) < 0.01f) {
                             bool forA = (a.x->parent ? a.x->parent->relative : nullptr)
                                      == a.y->parent;
                             bool forB = (b.x->parent ? b.x->parent->relative : nullptr)
                                      == b.y->parent;
                             const int commonB = commonAreaSize(b);
                             const int commonA = commonAreaSize(a);
                             if (commonA == commonB) {
                                 return forB < forA;
                             } else {
                                 return commonB < commonA;
                             }
                         }
                         return b.similarity < a.similarity;
                     });

    distillLeafs();
    distillInternal();
    distillInternalExtra();

    for (Node *x : po1) {
        if (x->relative == nullptr) {
            markNode(*x, State::Deleted);
        }
    }
    for (Node *y : po2) {
        if (y->relative == nullptr) {
            markNode(*y, State::Inserted);
        }
    }
}
