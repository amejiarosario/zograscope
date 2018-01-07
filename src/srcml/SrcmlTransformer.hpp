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

#ifndef ZOGRASCOPE__SRCML__SRCMLTRANSFORMER_HPP__
#define ZOGRASCOPE__SRCML__SRCMLTRANSFORMER_HPP__

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <boost/utility/string_ref.hpp>

class PNode;
class TiXmlElement;
class TiXmlNode;
class TreeBuilder;

enum class SType : std::uint8_t;
enum class Type : std::uint8_t;

// Uses srcml to parse a file and transforms the result into PTree.
class SrcmlTransformer
{
public:
    // Remembers parameters to use them later.  `contents`, `map` and `keywords`
    // have to be lvalues.
    SrcmlTransformer(const std::string &contents, TreeBuilder &tb,
                     const std::string &language,
                     const std::unordered_map<std::string, SType> &map,
                     const std::unordered_set<std::string> &keywords);

public:
    // Does all the work of transforming.
    void transform();

private:
    // Transforms single element into a node.
    PNode * visit(TiXmlNode *node, int level);
    // Transforms text field.
    void visitLeaf(TiXmlNode *parent, PNode *pnode, TiXmlNode *leaf);
    // Determines type of a child of the specified element.
    Type determineType(TiXmlElement *elem, boost::string_ref value);

private:
    const std::string &contents;                       // Contents to parse.
    boost::string_ref left;                            // Unparsed part.
    TreeBuilder &tb;                                   // Result builder.
    std::string language;                              // Language name.
    const std::unordered_map<std::string, SType> &map; // Tag -> SType map.
    const std::unordered_set<std::string> &keywords;   // List of keywords.
    int line;                                          // Current line.
    int col;                                           // Current column.
    int inCppDirective;                                // Level of cpp nesting.
};

#endif // ZOGRASCOPE__SRCML__SRCMLTRANSFORMER_HPP__