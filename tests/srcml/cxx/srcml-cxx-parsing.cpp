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

// These are tests of comparison and all of its phases.

#include "Catch/catch.hpp"

#include "tests.hpp"

TEST_CASE("Functions and their declarations are moved to a separate layer",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void readFile(const std::string &path);    /// Deletions
        void f(const std::string &path) {
            int anchor;
            varMap = parseOptions(argv, options);  /// Mixed
        }
    )", R"(
        void f(const std::string &path) {
            int anchor;
            varMap = parseOptions(args, options);  /// Mixed
        }
    )");

    diffSrcmlCxx(R"(
        static std::string readFile(const std::string &path);     /// Deletions
    )", R"(
        void                                                      /// Additions
        Environment::setup(const std::vector<std::string> &argv)  /// Additions
        {                                                         /// Additions
        }                                                         /// Additions
    )");
}

TEST_CASE("Statements are moved to a separate layer",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        SrcmlCxxLanguage::SrcmlCxxLanguage() {
            map["comment"]   = +SrcmlCxxSType::Comment;    /// Moves
            map["separator"] = +SrcmlCxxSType::Separator;
        }
    )", R"(
        SrcmlCxxLanguage::SrcmlCxxLanguage() {
            map["separator"] = +SrcmlCxxSType::Separator;
            map["comment"]   = +SrcmlCxxSType::Comment;    /// Moves
        }
    )");
}

TEST_CASE("Function body is spliced into function itself",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            float that;  /// Deletions
        }
    )", R"(
        void f() {
            int var;     /// Additions
        }
    )");
}

TEST_CASE("Argument list is spliced into the call",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            longAndBoringName(
                  "old line"    /// Updates
            );
        }
    )", R"(
        void f() {
            longAndBoringName(
                  "new string"  /// Updates
            );
        }
    )");
}

TEST_CASE("Parenthesis aren't mixed with operators",
          "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            return
                  (                                     /// Deletions
                  -x->stype == SrcmlCxxSType::Comment
                  )                                     /// Deletions
                  ;
        }
    )", R"(
        void f() {
            return -x->stype == SrcmlCxxSType::Comment
                || x->type == Type::StrConstants        /// Additions
                ;
        }
    )");
}

TEST_CASE("Various declaration aren't mixed up", "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        class SrcmlTransformer
        {
            SrcmlTransformer(const std::string &contents, TreeBuilder &tb,
                            const std::string &language                         /// Deletions
                            ,
                            const std::unordered_map<std::string, SType> &map   /// Deletions
                            ,
                            const std::unordered_set<std::string> &keywords);

        private:
            const std::unordered_map<std::string, SType> &map; // Tag -> SType
        };
    )", R"(
        enum class Type : std::uint8_t;                                        /// Additions

        class SrcmlTransformer
        {
            SrcmlTransformer(const std::string &contents, TreeBuilder &tb,
                            const std::unordered_set<std::string> &keywords);

            // Determines type of a child of the specified element.            /// Additions
            Type determineType(TiXmlElement *elem, boost::string_ref value);   /// Additions

        private:
            const std::unordered_map<std::string, SType> &map; // Tag -> SType
        };
    )");
}

TEST_CASE("Various statements aren't mixed up", "[.srcml][srcml-cxx][parsing]")
{
    diffSrcmlCxx(R"(
        void f() {
            if (elem->ValueStr() == "comment") {
                return Type::Comments;
            } else if (elem->ValueStr() == "name") {
                if (parentValue == "type") {
                    return bla ? Type::Keywords : Type::UserTypes;
                } else if (parentValue == "function" || parentValue == "call") {
                    return Type::Functions;
                }
            } else if (keywords.find(value.to_string()) != keywords.cend()) {
                return Type::Keywords;
            }
            return Type::Other;
        }
    )", R"(
        void f() {
            if (elem->ValueStr() == "comment") {
                return Type::Comments;
            }
            else if (inCppDirective) {                                           /// Additions
                return Type::Directives;                                         /// Additions
            } else if (value[0] == '(' || value[0] == '{' || value[0] == '[') {  /// Additions
                return Type::LeftBrackets;                                       /// Additions
            } else if (value[0] == ')' || value[0] == '}' || value[0] == ']') {  /// Additions
                return Type::RightBrackets;                                      /// Additions
            } else if (elem->ValueStr() == "operator") {                         /// Additions
                return Type::Operators;                                          /// Additions
            }                                                                    /// Additions
            else if (elem->ValueStr() == "name") {
                if (parentValue == "type") {
                    return bla ? Type::Keywords : Type::UserTypes;
                } else if (parentValue == "function" || parentValue == "call") {
                    return Type::Functions;
                }
            } else if (keywords.find(value.to_string()) != keywords.cend()) {
                return Type::Keywords;
            }
            return Type::Other;
        }
    )");
}
