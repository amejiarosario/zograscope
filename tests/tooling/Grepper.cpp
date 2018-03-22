// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#include "Catch/catch.hpp"

#include <string>

#include "tooling/Grepper.hpp"
#include "tooling/Matcher.hpp"
#include "mtypes.hpp"
#include "tree.hpp"

#include "tests.hpp"

TEST_CASE("Grep starts each processing with clear match", "[tooling][grepper]")
{
    Matcher matcher(MType::Parameter, nullptr);
    Grepper grepper({ "*", "const" });

    int nMatches = 0, nGreps = 0;

    auto matchHandler = [&](Node *node) {
        auto grepHandler = [&](const std::vector<Node *> &/*match*/) {
            ++nGreps;
        };

        grepper.grep(node, grepHandler);
        ++nMatches;
    };

    Tree tree = parseC("void f(const char *, const char);", true);

    CHECK(matcher.match(tree.getRoot(), *tree.getLanguage(), matchHandler));
    CHECK(nMatches == 2);
    CHECK(nGreps == 0);
}