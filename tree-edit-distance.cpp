#include "tree-edit-distance.hpp"

#define BOOST_DISABLE_ASSERTS

#include <boost/multi_array.hpp>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct Change
{
    int cost;
    int i, j;
};

enum { Wdel = 1, Wins = 1, Wren = 1 };

void
print(const Node &node, int lvl)
{
    if (node.satellite) {
        return;
    }

    std::string prefix = (lvl > 0) ? "`---" : "";

    std::string suffix;
    switch (node.state) {
        case State::Unchanged: break;
        case State::Deleted:   suffix = " (deleted)";  break;
        case State::Inserted:  suffix = " (inserted)"; break;
        case State::Updated:   suffix = " (updated with " + node.buddy->label + ")";  break;
    }

    std::cout << std::setw(4*lvl) << prefix << node.label
              << '[' << node.poID << ']'
              << '(' << node.line << ';' << node.col << ')'
              << (node.satellite ? "{S}" : "")
              << suffix << '\n';
    for (const Node &child : node.children) {
        print(child, lvl + 1);
    }
}

static void
postOrder(Node &node, std::vector<Node *> &v)
{
    if (node.satellite) {
        return;
    }

    for (Node &child : node.children) {
        postOrder(child, v);
    }
    node.poID = v.size();
    v.push_back(&node);
}

static std::vector<Node *>
postOrder(Node &root)
{
    std::vector<Node *> v;
    postOrder(root, v);
    return v;
}

static void
lmld(Node &node, std::vector<int> &l)
{
    if (node.satellite) {
        return;
    }

    auto isSatellite = [](const Node &n) { return n.satellite; };

    std::vector<Node> n = node.children;
    n.erase(std::remove_if(n.begin(), n.end(), isSatellite), n.end());

    if (n.empty()) {
        l[node.poID] = node.poID;
        return;
    }

    for (Node &child : n) {
        lmld(child, l);
    }
    l[node.poID] = l[n[0].poID];
}

static std::vector<int>
lmld(Node &root)
{
    // post-order ID of root is equal to number of nodes - 1
    std::vector<int> l(root.poID + 1);
    lmld(root, l);
    return l;
}

static int
countLeaves(Node &node)
{
    auto notSatellite = [](const Node &n) { return !n.satellite; };

    int n = std::count_if(node.children.cbegin(), node.children.cend(),
                          notSatellite) == 0;
    for (Node &child : node.children) {
        if (!child.satellite) {
            n += countLeaves(child);
        }
    }
    return n;
}

static std::vector<int>
makeKr(Node &root, std::vector<int> &l)
{
    int k = countLeaves(root);

    std::vector<bool> visited(l.size());

    std::vector<int> kr;
    kr.reserve(k);

    auto i = l.size() - 1U;
    while (k >= 1) {
        if (!visited[l[i]]) {
            kr.push_back(i);
            --k;
            visited[l[i]] = true;
        }
        --i;
    }

    std::sort(kr.begin(), kr.end());
    return kr;
}

void
printTree(const std::string &name, Node &root)
{
    std::cout << name << '\n';
    std::cout << std::setw(name.length())
              << std::setfill('=') << "" << std::setfill(' ') << '\n';
    print(root);

    std::cout << '\n';

    std::vector<Node *> po = postOrder(root);
    for (Node *node : po) {
        std::cout << "po(" << node->label << ") = " << node->poID + 1 << '\n';
    }

    std::cout << '\n';

    std::vector<int> l = lmld(root);
    for (unsigned int i = 0U; i < l.size(); ++i) {
        std::cout << "l(" << po[i]->label << ") = " << l[i] + 1 << '\n';
    }

    std::cout << '\n';

    std::vector<int> kr = makeKr(root, l);
    for (unsigned int i = 0U; i < kr.size(); ++i) {
        std::cout << "k[" << i << "] = " << kr[i] + 1 << '\n';
    }
}

static void
forestDist(int i, int j, const std::vector<int> &l1, const std::vector<int> &l2,
           boost::multi_array<Change, 2> &td,
           boost::multi_array<int, 2> &fd,
           const std::vector<Node *> &po1, const std::vector<Node *> &po2)
{
    fd[l1[i] - 1][l2[j] - 1] = 0;
    for (int di = l1[i]; di <= i; ++di) {
        fd[di][l2[j] - 1] = fd[di - 1][l2[j] - 1] + Wdel;
    }
    for (int dj = l2[j]; dj <= j; ++dj) {
        fd[l1[i] - 1][dj] = fd[l1[i] - 1][dj - 1] + Wins;
    }
    for (int di = l1[i]; di <= i; ++di) {
        const int ldi = l1[di];
        for (int dj = l2[j]; dj <= j; ++dj) {
            const int ldj = l2[dj];
            if (ldi == l1[i] && ldj == l2[j]) {
                const bool identicalRename = (po1[di]->label == po2[dj]->label);
                fd[di][dj] = std::min({
                    fd[di - 1][dj] + Wdel,
                    fd[di][dj - 1] + Wins,
                    fd[di - 1][dj - 1] + (identicalRename ? 0 : Wren)
                });
                td[di][dj] = Change { fd[di][dj], i, j };
            } else {
                fd[di][dj] =
                    std::min({ fd[di - 1][dj] + Wdel,
                               fd[di][dj - 1] + Wins,
                               fd[ldi - 1][ldj - 1] + td[di][dj].cost });
            }
        }
    }

    // for (int di = l1[i] - 1; di <= i; ++di) {
    //     for (int dj = l2[j] - 1; dj <= j; ++dj) {
    //         std::cout << std::setw(2) << fd[di][dj] << ' ';
    //     }
    //     std::cout << '\n';
    // }

    // std::cout << "--------\n";

    // for (unsigned int i = 0; i < l1.size(); ++i) {
    //     for (unsigned int j = 0; j < l2.size(); ++j) {
    //         std::string mark;
    //         // switch (td[i][j].state) {
    //         //     case State::Unchanged: mark = "(U)"; break;
    //         //     case State::Deleted:   mark = "(D)";  break;
    //         //     case State::Inserted:  mark = "(I)"; break;
    //         //     case State::Updated:   mark = "(R)";  break;
    //         // }

    //         std::cout << std::setw(2) << td[i][j].cost << mark << ' ';
    //     }
    //     std::cout << '\n';
    // }
}

class BacktrackingQueue
{
    using pair = std::pair<int, int>;

public:
    void enqueue(int i, int j, int di, int dj)
    {
        queue[pair{ i, j }].emplace_back(di, dj);
    }

    std::vector<pair> takeCurrent()
    {
        auto lastItem = --queue.end();
        std::vector<pair> r = lastItem->second;
        queue.erase(lastItem);
        return r;
    }

    bool hasMore() const
    {
        return !queue.empty();
    }

    pair getCurrent() const
    {
        return (--queue.cend())->first;
    }

private:
    std::map<pair, std::vector<pair>> queue;
};

static void
backtrackForests(const std::vector<int> &l1, const std::vector<int> &l2,
                 const boost::multi_array<Change, 2> &td,
                 boost::multi_array<int, 2> &fd,
                 const std::vector<Node *> &po1, const std::vector<Node *> &po2,
                 BacktrackingQueue &bq)
{
    int i, j;
    std::tie(i, j) = bq.getCurrent();

    // This creates forest table identical to the one created on forward pass,
    // but it doesn't update tree table.  Tree table is fully calculated by now,
    // so we can just use its values.
    fd[l1[i] - 1][l2[j] - 1] = 0;
    for (int di = l1[i]; di <= i; ++di) {
        fd[di][l2[j] - 1] = fd[di - 1][l2[j] - 1] + Wdel;
    }
    for (int dj = l2[j]; dj <= j; ++dj) {
        fd[l1[i] - 1][dj] = fd[l1[i] - 1][dj - 1] + Wins;
    }
    for (int di = l1[i]; di <= i; ++di) {
        for (int dj = l2[j]; dj <= j; ++dj) {
            if (l1[di] == l1[i] && l2[dj] == l2[j]) {
                const bool identicalRename = (po1[di]->label == po2[dj]->label);
                fd[di][dj] = std::min({
                    fd[di - 1][dj] + Wdel,
                    fd[di][dj - 1] + Wins,
                    fd[di - 1][dj - 1] + (identicalRename ? 0 : Wren)});
            } else {
                fd[di][dj] =
                    std::min({ fd[di - 1][dj] + Wdel,
                               fd[di][dj - 1] + Wins,
                               fd[l1[di] - 1][l2[dj] - 1] + td[di][dj].cost });
            }
        }
    }

    for (const auto &p : bq.takeCurrent()) {
        int di = p.first, dj = p.second;
        while (di > l1[i] - 1 || dj > l2[j] - 1) {
            if (di == l1[i] - 1) {
                po2[dj--]->state = State::Inserted;
            } else if (dj == l2[j] - 1) {
                po1[di--]->state = State::Deleted;
            } else if (l1[di] == l1[i] && l2[dj] == l2[j]) {
                if (fd[di][dj] == fd[di - 1][dj] + Wdel) {
                    po1[di--]->state = State::Deleted;
                } else if (fd[di][dj] == fd[di][dj - 1] + Wins) {
                    po2[dj--]->state = State::Inserted;
                } else if (fd[di][dj] != fd[di - 1][dj - 1]) {
                    po1[di]->buddy = po2[dj];
                    po2[dj]->buddy = po1[di];

                    po1[di--]->state = State::Updated;
                    po2[dj--]->state = State::Updated;
                } else {
                    --di;
                    --dj;
                }
            } else {
                if (fd[di][dj] == fd[di - 1][dj] + Wdel) {
                    po1[di--]->state = State::Deleted;
                } else if (fd[di][dj] == fd[di][dj - 1] + Wins) {
                    po2[dj--]->state = State::Inserted;
                } else {
                    bq.enqueue(td[di][dj].i, td[di][dj].j, di, dj);
                    di = l1[di] - 1;
                    dj = l2[dj] - 1;
                }
            }
        }
    }
}

int
ted(Node &T1, Node &T2)
{
    std::vector<Node *> po1 = postOrder(T1);
    std::vector<Node *> po2 = postOrder(T2);

    std::vector<int> l1 = lmld(T1);
    std::vector<int> l2 = lmld(T2);

    boost::multi_array<Change, 2> td(boost::extents[l1.size()][l2.size()]);
    for (unsigned int i = 0; i < l1.size(); ++i) {
        for (unsigned int j = 0; j < l2.size(); ++j) {
            td[i][j].cost = -1;
        }
    }

    std::vector<int> k1 = makeKr(T1, l1);
    std::vector<int> k2 = makeKr(T2, l2);

    using range = boost::multi_array_types::extent_range;
    boost::multi_array<int, 2> fd(boost::extents[range(-1, po1.size())]
                                                [range(-1, po2.size())]);
    // int step = 0;
    for (int x : k1) {
        for (int y : k2) {
            // std::cout << "step #" << ++step << '\n';
            forestDist(x, y, l1, l2, td, fd, po1, po2);
        }
    }

    // Mark nodes with states by backtracking through forest arrays.  We do this
    // in reversed order by regenerating only arrays that are actually needed to
    // recover solution.  Starting with cell containing the answer and figuring
    // out how we get there like in regular dynamic programming.  The difference
    // is that we need to process multiple arrays.  The tracing splits on steps
    // where forests are processed based on information from tree array.
    BacktrackingQueue bq;
    bq.enqueue(k1.back(), k2.back(), l1.size() - 1, l2.size() - 1);
    while (bq.hasMore()) {
        backtrackForests(l1, l2, td, fd, po1, po2, bq);
    }

    return td[l1.size() - 1][l2.size() - 1].cost;
}
