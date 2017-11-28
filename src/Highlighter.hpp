// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#ifndef ZOGRASCOPE__HIGHLIGHTER_HPP__
#define ZOGRASCOPE__HIGHLIGHTER_HPP__

#include <memory>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include <boost/utility/string_ref.hpp>

class Node;

// Tree highlighter.  Highlights either all at once or by line ranges.
class Highlighter
{
    class ColorPicker;

public:
    // Stores arguments for future reference.  The original flag specifies
    // whether this is an old version of a file (matters only for trees marked
    // with results of comparison).
    Highlighter(Node &root, bool original = true);

    // No copying.
    Highlighter(const Highlighter&) = delete;
    // No assigning.
    Highlighter & operator=(const Highlighter&) = delete;

    // Destructs the highlighter.
    ~Highlighter();

public:
    // Prints lines in the range [from, from + n) into a string.  Each line can
    // be printed at most once, thus calls to this function need to increase
    // from argument.  Returns the string.
    std::string print(int from, int n);
    // Prints lines until the end into a string.  Returns the string.
    std::string print();

private:
    // Skips everything until target line is reached.
    void skipUntil(int targetLine);
    // Prints at most `n` lines.
    void print(int n);
    // Prints lines of spelling decreasing `n` on advancing through lines.
    void printSpelling(int &n);
    // Retrieves the next node to be processed.
    Node * getNode();
    // Advances processing to the next node.  The node here is the one that was
    // returned by `getNode()` earlier.
    void advanceNode(Node *node);

private:
    std::ostringstream oss;                   // Temporary output buffer.
    int line, col;                            // Current position.
    std::unique_ptr<ColorPicker> colorPicker; // Highlighting state.
    std::vector<boost::string_ref> olines;    // Undiffed spelling.
    std::vector<boost::string_ref> lines;     // Possibly diffed spelling.
    std::stack<Node *> toProcess;             // State of tree traversal.
    std::string spelling;                     // Storage behind `lines` field.
    bool original;                            // Whether this is an old version.
};

#endif // ZOGRASCOPE__HIGHLIGHTER_HPP__
