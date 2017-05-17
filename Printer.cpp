#include "Printer.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#define BOOST_DISABLE_ASSERTS
#include <boost/multi_array.hpp>

#include <deque>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "decoration.hpp"
#include "tree-edit-distance.hpp"

enum class Diff
{
    Left,
    Right,
    Identical,
    Different,
    Fold,
};

struct DiffLine
{
    DiffLine(Diff type, int data = 0) : type(type), data(data)
    {
    }

    Diff type;
    int data;
};

static std::deque<DiffLine> compare(std::vector<std::string> &l,
                                    std::vector<std::string> &r);
static std::vector<std::string> split(const std::string &str, char with);
static std::string printSource(Node &root);
static unsigned int measureWidth(const std::string &s);

Printer::Printer(Node &left, Node &right) : left(left), right(right)
{
}

void
Printer::print()
{
    static std::string empty;

    std::vector<std::string> l = split(printSource(left), '\n');
    std::vector<std::string> r = split(printSource(right), '\n');

    std::deque<DiffLine> diff = compare(l, r);

    unsigned int maxWidth = 0U;
    std::vector<unsigned int> widths;
    widths.reserve(l.size());
    for (const std::string &ll : l) {
        const unsigned int width = measureWidth(ll);
        widths.push_back(width);

        maxWidth = std::max(width, maxWidth);
    }

    unsigned int i = 0U, j = 0U;
    for (DiffLine d : diff) {
        const std::string *ll = &empty;
        const std::string *rl = &empty;

        const char *marker;
        switch (d.type) {
            case Diff::Left:
                ll = &l[i++];
                marker = " << ";
                break;
            case Diff::Right:
                rl = &r[j++];
                marker = " >> ";
                break;
            case Diff::Identical:
                ll = &l[i++];
                rl = &r[j++];
                marker = " || ";
                break;
            case Diff::Different:
                ll = &l[i++];
                rl = &r[j++];
                marker = " <> ";
                break;
            case Diff::Fold:
                i += d.data;
                j += d.data;
                {
                    std::string msg = " @@ folded " + std::to_string(d.data)
                                    + " identical lines @@";
                    std::cout << std::right
                              << std::setw(maxWidth + 4 + msg.size()/2)
                              << msg << '\n';
                }
                continue;
        }

        unsigned int width = (ll->empty() ? 0U : widths[i - 1U]);
        width = maxWidth + (ll->size() - width);
        std::cout << std::left << std::setw(width) << *ll
                  << marker << *rl << '\n';
    }
}

static std::deque<DiffLine>
compare(std::vector<std::string> &l, std::vector<std::string> &r)
{
    using size_type = std::vector<std::string>::size_type;

    // Narrow portion of lines that should be compared by throwing away matching
    // leading and trailing lines.
    size_type ol = 0U, nl = 0U, ou = l.size(), nu = r.size();
    while (ol < ou && nl < nu && l[ol] == r[nl]) {
        ++ol;
        ++nl;
    }
    while (ou > ol && nu > nl && l[ou - 1U] == r[nu - 1U]) {
        --ou;
        --nu;
    }

    boost::multi_array<int, 2> d(boost::extents[ou - ol + 1U][nu - nl + 1U]);

    // Edit distance finding.
    for (size_type i = 0U; i <= ou - ol; ++i) {
        for (size_type j = 0U; j <= nu - nl; ++j) {
            if (i == 0U) {
                d[i][j] = j;
            } else if (j == 0U) {
                d[i][j] = i;
            } else {
                const bool same = (l[ol + i - 1U] == r[nl + j - 1U]);
                d[i][j] = std::min({ d[i - 1U][j] + 1, d[i][j - 1U] + 1,
                                     d[i - 1U][j - 1U] + (same ? 0 : 1) });
            }
        }
    }

    size_type identicalLines = 0U;

    const size_type minFold = 3;
    const size_type ctxSize = 2;

    std::deque<DiffLine> diffSeq;

    auto foldIdentical = [&](bool last) {
        size_type startContext = (last ? 0 : ctxSize);
        size_type endContext = (identicalLines == diffSeq.size() ? 0 : ctxSize);
        size_type context = startContext + endContext;

        if (identicalLines >= context && identicalLines - context > minFold) {
            diffSeq.erase(diffSeq.cbegin() + startContext,
                          diffSeq.cbegin() + (identicalLines - endContext));
            diffSeq.emplace(diffSeq.cbegin() + startContext, Diff::Fold,
                            identicalLines - context);
        }
        identicalLines = 0U;
    };

    auto handleSameLines = [&](size_type i, size_type j) {
        if (l[i] == r[j]) {
            ++identicalLines;
            diffSeq.emplace_front(Diff::Identical);
        } else {
            foldIdentical(false);
            diffSeq.emplace_front(Diff::Different);
        }
    };

    // Compose results with folding of long runs of identical lines (longer
    // than two lines).  Mind that we go from last to first element and loops
    // below process tail, middle and then head parts of the files.

    for (size_type k = l.size(), l = r.size(); k > ou; --k, --l) {
        handleSameLines(k - 1U, l - 1U);
    }

    int i = ou - ol, j = nu - nl;
    while (i != 0U || j != 0U) {
        if (i == 0) {
            --j;
            foldIdentical(false);
            diffSeq.emplace_front(Diff::Right);
        } else if (j == 0) {
            --i;
            foldIdentical(false);
            diffSeq.emplace_front(Diff::Left);
        } else if (d[i][j] == d[i][j - 1] + 1) {
            --j;
            foldIdentical(false);
            diffSeq.emplace_front(Diff::Right);
        } else if (d[i][j] == d[i - 1][j] + 1) {
            --i;
            foldIdentical(false);
            diffSeq.emplace_front(Diff::Left);
        } else {
            --i;
            --j;
            handleSameLines(ol + i, nl + j);
        }
    }

    for (size_type i = ol; i != 0U; --i) {
        handleSameLines(i - 1U, i - 1U);
    }

    foldIdentical(true);

    return diffSeq;
}

/**
 * @brief Splits string in a range-for loop friendly way.
 *
 * @param str String to split into substrings.
 * @param with Character to split at.
 *
 * @returns Array of results, empty on empty string.
 */
static std::vector<std::string>
split(const std::string &str, char with)
{
    if (str.empty()) {
        return {};
    }

    std::vector<std::string> results;
    boost::split(results, str, boost::is_from_range(with, with));
    return results;
}

static std::string
printSource(Node &root)
{
    std::ostringstream oss;

    int line = 1, col = 1;
    std::function<void(Node &)> visit = [&](Node &node) {
        if (node.line != 0 && node.col != 0) {
            while (node.line > line) {
                oss << '\n';
                ++line;
                col = 1;
            }

            while (node.col > col) {
                oss << ' ';
                ++col;
            }

            decor::Decoration dec;
            switch (node.state) {
                case State::Unchanged: break;
                case State::Deleted: dec = decor::red_fg + decor::inv + decor::black_bg + decor::bold; break;
                case State::Inserted: dec = decor::green_fg + decor::inv + decor::black_bg + decor::bold; break;
                case State::Updated: dec = decor::yellow_fg + decor::inv + decor::black_bg + decor::bold; break;
            }

            // if (node.state != State::Unchanged) {
            //     oss << (dec << '[') << node.label << (dec << ']');
            // } else {
            //     oss << node.label;
            // }

            const std::vector<std::string> lines = split(node.label, '\n');
            oss << (dec << lines.front());
            for (std::size_t i = 1U; i < lines.size(); ++i) {
                oss << '\n' << (dec << lines[i]);
                ++line;
            }

            col += node.label.size();
        }

        for (Node &child : node.children) {
            if (child.satellite) {
                child.state = node.state;
            }
            visit(child);
        }
    };
    visit(root);

    return oss.str();
}

/**
 * @brief Calculates width of a string ignoring embedded escape sequences.
 *
 * @param s String to measure.
 *
 * @returns The width.
 */
static unsigned int
measureWidth(const std::string &s)
{
    unsigned int valWidth = 0U;
    const char *str = s.c_str();
    while (*str != '\0') {
        if (*str != '\033') {
            ++valWidth;
            ++str;
            continue;
        }

        const char *const next = std::strchr(str, 'm');
        str = (next == nullptr) ? (str + std::strlen(str)) : (next + 1);
    }
    return valWidth;
}

