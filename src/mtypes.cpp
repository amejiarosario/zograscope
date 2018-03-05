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

#include "mtypes.hpp"

#include <cassert>

#include <ostream>

std::ostream &
operator<<(std::ostream &os, MType mtype)
{
    switch (mtype) {
        case MType::Other:       return (os << "Other");
        case MType::Declaration: return (os << "Declaration");
        case MType::Function:    return (os << "Function");
        case MType::Comment:     return (os << "Comment");
        case MType::Directive:   return (os << "Directive");
    }

    assert(false && "Unhandled enumeration item");
    return (os << "<UNKNOWN:" << static_cast<int>(mtype) << '>');
}