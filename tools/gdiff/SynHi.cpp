// Copyright (C) 2018 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#include "SynHi.hpp"

#include <vector>

#include <QSyntaxHighlighter>

#include "ColorCane.hpp"

SynHi::SynHi(QTextDocument *document, std::vector<ColorCane> &&hi)
    : QSyntaxHighlighter(document), hi(std::move(hi))
{
}

SynHi::~SynHi() = default;

const std::vector<ColorCane> &
SynHi::getHi() const
{
    return hi;
}

void
SynHi::highlightBlock(const QString &)
{
    int line = currentBlockState();
    if (line < 0 || line >= static_cast<int>(hi.size())) return;

    int from = 0;
    for (const ColorCanePiece &piece : hi[line]) {
        setFormat(from, piece.text.length(), cs[piece.hi]);
        from += piece.text.length();
    }
}
