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

// These are tests of basic properties of a parser.  Things like:
//  - whether some constructs can be parsed
//  - whether elements are identified correctly

#include "Catch/catch.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include <iostream>

#include "make/MakeSType.hpp"
#include "utils/strings.hpp"
#include "Highlighter.hpp"
#include "tree.hpp"

#include "tests.hpp"

using namespace makestypes;

TEST_CASE("Error is printed on incorrect Makefile syntax", "[make][parser]")
{
    StreamCapture coutCapture(std::cout);
    CHECK_FALSE(makeIsParsed(":"));
    REQUIRE(boost::starts_with(coutCapture.get(), "<input>:1:1:"));
}

TEST_CASE("Empty Makefile is OK", "[make][parser]")
{
    CHECK(makeIsParsed(""));
    CHECK(makeIsParsed("      "));
    CHECK(makeIsParsed("   \n   "));
    CHECK(makeIsParsed("\t\n \t \n"));
}

TEST_CASE("Root always has Makefile stype", "[make][parser]")
{
    Tree tree;

    tree = parseMake("");
    CHECK(tree.getRoot()->stype == +MakeSType::Makefile);

    tree = parseMake("a = b");
    CHECK(tree.getRoot()->stype == +MakeSType::Makefile);
}

TEST_CASE("Useless empty temporary containers are dropped", "[make][parser]")
{
    auto pred = [](const Node *node) {
        return node->stype == +MakeSType::TemporaryContainer
            && node->label.empty();
    };

    Tree tree = parseMake("set := to this");
    CHECK(findNode(tree, pred) == nullptr);
}

TEST_CASE("Comments are parsed in a Makefile", "[make][parser]")
{
    CHECK(makeIsParsed(" # comment"));
    CHECK(makeIsParsed("# comment "));
    CHECK(makeIsParsed(" # comment "));

    const char *const multiline = R"(
        a := a#b \
               asdf
    )";
    CHECK(makeIsParsed(multiline));
}

TEST_CASE("Assignments are parsed in a Makefile", "[make][parser]")
{
    SECTION("Whitespace around assignment statement") {
        CHECK(makeIsParsed("OS := os "));
        CHECK(makeIsParsed(" OS := os "));
        CHECK(makeIsParsed(" OS := os"));
    }
    SECTION("Whitespace around assignment") {
        CHECK(makeIsParsed("OS := os"));
        CHECK(makeIsParsed("OS:= os"));
        CHECK(makeIsParsed("OS :=os"));
    }
    SECTION("Various assignment kinds") {
        CHECK(makeIsParsed("CXXFLAGS = $(CFLAGS)"));
        CHECK(makeIsParsed("CXXFLAGS += -std=c++11 a:b"));
        CHECK(makeIsParsed("CXX ?= g++"));
        CHECK(makeIsParsed("CXX ::= g++"));
        CHECK(makeIsParsed("CXX != whereis g++"));
    }
    SECTION("One variable") {
        CHECK(makeIsParsed("CXXFLAGS = prefix$(CFLAGS)"));
        CHECK(makeIsParsed("CXXFLAGS = prefix$(CFLAGS)suffix"));
        CHECK(makeIsParsed("CXXFLAGS = $(CFLAGS)suffix"));
    }
    SECTION("Two variables") {
        CHECK(makeIsParsed("CXXFLAGS = $(var1)$(var2)"));
        CHECK(makeIsParsed("CXXFLAGS = $(var1)val$(var2)"));
        CHECK(makeIsParsed("CXXFLAGS = val$(var1)$(var2)val"));
        CHECK(makeIsParsed("CXXFLAGS = val$(var1)val$(var2)val"));
    }
    SECTION("Override and/or export") {
        CHECK(makeIsParsed("override CXXFLAGS ::="));
        CHECK(makeIsParsed("override CXXFLAGS = $(CFLAGS)suffix"));
        CHECK(makeIsParsed("override export CXXFLAGS = $(CFLAGS)suffix"));
        CHECK(makeIsParsed("export override CXXFLAGS = $(CFLAGS)suffix"));
        CHECK(makeIsParsed("export CXXFLAGS = $(CFLAGS)suffix"));
    }
    SECTION("Keywords in the value") {
        CHECK(makeIsParsed("KEYWORDS := ifdef/ifndef/ifeq/ifneq/else/endif"));
        CHECK(makeIsParsed("KEYWORDS += include"));
        CHECK(makeIsParsed("KEYWORDS += override/export/unexport"));
        CHECK(makeIsParsed("KEYWORDS += define/endef/undefine"));
    }
    SECTION("Keywords in the name") {
        CHECK(makeIsParsed("a.ifndef.b = a"));
        CHECK(makeIsParsed("ifndef.b = a"));
        CHECK(makeIsParsed("ifndef/b = a"));
        CHECK(makeIsParsed(",ifdef,ifndef,ifeq,ifneq,else,endif = b"));
        CHECK(makeIsParsed(",override,include,define,endef,undefine = b"));
        CHECK(makeIsParsed(",export,unexport = b"));
    }
    SECTION("Functions in the name") {
        CHECK(makeIsParsed(R"($(1).stuff :=)"));
    }
    SECTION("Special symbols in the value") {
        CHECK(makeIsParsed(R"(var += sed_first='s,^\([^/]*\)/.*$$,\1,';)"));
        CHECK(makeIsParsed(R"(var != test $$# -gt 0)"));
    }
    SECTION("Whitespace around assignment statement") {
        Tree tree;

        tree = parseMake("var := #comment");
        CHECK(findNode(tree, Type::Comments, "#comment") != nullptr);

        tree = parseMake("var := val#comment");
        CHECK(findNode(tree, Type::Comments, "#comment") != nullptr);

        tree = parseMake("override var := #comment");
        CHECK(findNode(tree, Type::Comments, "#comment") != nullptr);

        tree = parseMake("export var := val #comment");
        CHECK(findNode(tree, Type::Comments, "#comment") != nullptr);
    }
}

TEST_CASE("Variables are parsed in a Makefile", "[make][parser]")
{
    CHECK(makeIsParsed("$($(V))"));
    CHECK(makeIsParsed("$($(V)suffix)"));
    CHECK(makeIsParsed("$($(V)?endif)"));
    CHECK(makeIsParsed("$(bla?endif)"));
    CHECK(makeIsParsed("$(AT_$(V))"));
    CHECK(makeIsParsed("target: )()("));

    Tree tree = parseMake("target: $$($1.name)");
    CHECK(findNode(tree, Type::UserTypes, "$$") != nullptr);
    CHECK(findNode(tree, Type::UserTypes, "$1") != nullptr);
}

TEST_CASE("Functions are parsed in a Makefile", "[make][parser]")
{
    SECTION("Whitespace around function statement") {
        CHECK(makeIsParsed("$(info a)"));
        CHECK(makeIsParsed(" $(info a)"));
        CHECK(makeIsParsed(" $(info a) "));
        CHECK(makeIsParsed("$(info a) "));
        CHECK(makeIsParsed("$(info a )"));
    }
    SECTION("Whitespace around comma") {
        CHECK(makeIsParsed("$(info arg1,arg2)"));
        CHECK(makeIsParsed("$(info arg1, arg2)"));
        CHECK(makeIsParsed("$(info arg1 ,arg2)"));
        CHECK(makeIsParsed("$(info arg1 , arg2)"));
    }
    SECTION("Functions in arguments") {
        CHECK(makeIsParsed("$(info $(f1)$(f2))"));
    }
    SECTION("Empty arguments") {
        CHECK(makeIsParsed("$(patsubst , ,)"));
        CHECK(makeIsParsed("$(patsubst ,,)"));
        CHECK(makeIsParsed("$(patsubst a,,)"));
        CHECK(makeIsParsed("$(patsubst a,b,)"));
    }
    SECTION("Keywords in the arguments") {
        CHECK(makeIsParsed("$(info ifdef/ifndef/ifeq/ifneq/else/endif)"));
        CHECK(makeIsParsed("$(info include)"));
        CHECK(makeIsParsed("$(info override/export/unexport)"));
        CHECK(makeIsParsed("$(info define/endef/undefine)"));
    }
    SECTION("Curly braces") {
        Tree tree = parseMake("target: ${name}");
        CHECK(findNode(tree, Type::LeftBrackets, "${") != nullptr);
        CHECK(findNode(tree, Type::RightBrackets, "}") != nullptr);
    }
}

TEST_CASE("Targets are parsed in a Makefile", "[make][parser]")
{
    SECTION("Whitespace around colon") {
        CHECK(makeIsParsed("target: prereq"));
        CHECK(makeIsParsed("target : prereq"));
        CHECK(makeIsParsed("target :prereq"));
    }
    SECTION("No prerequisites") {
        CHECK(makeIsParsed("target:"));
    }
    SECTION("Multiple prerequisites") {
        CHECK(makeIsParsed("target: prereq1 prereq2"));
    }
    SECTION("Multiple targets") {
        CHECK(makeIsParsed("debug release sanitize-basic: all"));
    }
    SECTION("Double-colon") {
        CHECK(makeIsParsed("debug:: all"));
    }
    SECTION("Keywords in targets") {
        CHECK(makeIsParsed("include.b: all"));
        CHECK(makeIsParsed("x.include.b: all"));
        CHECK(makeIsParsed("all: b.override.b"));
        CHECK(makeIsParsed("all: override.b"));
    }
    SECTION("Expressions in prerequisites and targets") {
        CHECK(makeIsParsed("target: $(dependencies)"));
        CHECK(makeIsParsed("$(target): dependencies"));
        CHECK(makeIsParsed("$(target): $(dependencies)"));
        CHECK(makeIsParsed("$(tar)$(get): $(dependencies)"));
    }
}

TEST_CASE("Recipes are parsed in a Makefile", "[make][parser]")
{
    const char *const singleLine = R"(
target: prereq
	the only recipe
    )";
    CHECK(makeIsParsed(singleLine));

    const char *const multipleLines = R"(
target: prereq
	first recipe
	second recipe
    )";
    CHECK(makeIsParsed(multipleLines));

    const char *const withComments = R"(
target: prereq
# comment
	first recipe
# comment
	second recipe
	# comment
    )";
    CHECK(makeIsParsed(withComments));

    const char *const withParens = R"(
target: prereq
	(echo something)
    )";
    CHECK(makeIsParsed(withParens));

    const char *const withFunctions = R"(
target: prereq
	$a$b
    )";
    CHECK(makeIsParsed(withFunctions));

    const char *const assignment = R"(
        $(out_dir)/tests/tests: EXTRA_CXXFLAGS += -Wno-error=parentheses
    )";
    CHECK(makeIsParsed(assignment));
}

TEST_CASE("Conditionals are parsed in a Makefile", "[make][parser]")
{
    const char *const withBody_withoutElse = R"(
        ifneq ($(OS),Windows_NT)
            bin_suffix :=
        endif
    )";
    CHECK(makeIsParsed(withBody_withoutElse));

    const char *const withoutBody_withoutElse = R"(
        ifneq ($(OS),Windows_NT)
        endif
    )";
    CHECK(makeIsParsed(withoutBody_withoutElse));

    const char *const withBody_withElse = R"(
        ifndef tool_template
            bin_suffix :=
        else
            bin_suffix := .exe
        endif
    )";
    CHECK(makeIsParsed(withBody_withElse));

    const char *const withoutBody_withElse = R"(
        ifeq ($(OS),Windows_NT)
            bin_suffix :=
        else
        endif
    )";
    CHECK(makeIsParsed(withoutBody_withElse));

    const char *const nested = R"(
        ifneq ($(OS),Windows_NT)
            ifdef OS
            endif
        endif
    )";
    CHECK(makeIsParsed(nested));

    const char *const emptyArgs = R"(
        ifneq ($(OS),)
        endif
        ifneq (,$(OS))
        endif
        ifneq (,)
        endif
    )";
    CHECK(makeIsParsed(emptyArgs));

    const char *const elseIf = R"(
        ifneq ($(OS),)
        else ifndef OS
        else ifeq (,)
        endif
    )";
    CHECK(makeIsParsed(elseIf));

    const char *const withComments = R"(
        ifneq ($(OS),)  # ifneq-comment
        else            # else-comment
        endif           # endif-comment
    )";
    CHECK(makeIsParsed(withComments));

    const char *const conditionalInRecipes = R"(
reset-coverage:
ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
endif
    )";
    CHECK(makeIsParsed(conditionalInRecipes));

    const char *const conditionalsInRecipes = R"(
reset-coverage:
ifdef tool_template
	find $(out_dir)/ -name '*.gcda' -delete
endif
ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
else
endif
    )";
    CHECK(makeIsParsed(conditionalsInRecipes));

    const char *const elseIfInRecipes = R"(
reset-coverage:
ifdef tool_template
	find $(out_dir)/ -name '*.gcda' -delete
else ifeq ($(with_cov),1)
	find $(out_dir)/ -name '*.gcda' -delete
else ifdef something
else
endif
    )";
    CHECK(makeIsParsed(elseIfInRecipes));

    const char *const conditionalInRecipesWithComments = R"(
reset-coverage:
ifeq ($(with_cov),1)  # ifeq-comment
	find $(out_dir)/ -name '*.gcda' -delete
else                  # else-comment
endif                 # endif-comment
    )";
    CHECK(makeIsParsed(conditionalInRecipesWithComments));

}

TEST_CASE("Defines are parsed in a Makefile", "[make][parser]")
{
    const char *const noBody = R"(
        define pattern
        endef
    )";
    CHECK(makeIsParsed(noBody));

    const char *const noSuffix = R"(
        define pattern
            bin_suffix :=
        endef
    )";
    CHECK(makeIsParsed(noSuffix));

    const char *const weirdName = R"(
        define )name
        endef
    )";
    CHECK(makeIsParsed(weirdName));

    const char *const withOverride = R"(
        override define pattern ?=
        endef
    )";
    CHECK(makeIsParsed(withOverride));

    const char *const withExport = R"(
        export define pattern
        endef
    )";
    CHECK(makeIsParsed(withExport));

    const char *const withOverrideAndExport = R"(
        export override define pattern
        endef
    )";
    CHECK(makeIsParsed(withOverrideAndExport));

    const char *const eqSuffix = R"(
        define pattern =
            bin_suffix :=
        endef
    )";
    CHECK(makeIsParsed(eqSuffix));

    const char *const immSuffix = R"(
        define pattern :=
        endef

        define pattern ::=
            bin_suffix :=
        endef
    )";
    CHECK(makeIsParsed(immSuffix));

    const char *const appendSuffix = R"(
        define pattern +=
            bin_suffix :=
        endef
    )";
    CHECK(makeIsParsed(appendSuffix));

    const char *const bangSuffix = R"(
        define pattern +=
        endef
    )";
    Tree tree = parseMake(bangSuffix);
    CHECK(findNode(tree, Type::Assignments, "+=") != nullptr);

    const char *const expresion = R"(
        define tool_template
            $1.bin
        endef
    )";
    CHECK(makeIsParsed(expresion));

    const char *const expresions = R"(
        define vifm_SOURCES :=
            $(cfg) $(compat) $(engine) endef $(int) $(io) $(menus) $(modes)
        endef
    )";
    CHECK(makeIsParsed(expresions));

    const char *const textFirstEmptyLine = R"(
        define vifm_SOURCES :=

            bla $(cfg) $(compat) $(engine) endef $(int) $(io) $(menus) $(modes)
        endef
    )";
    CHECK(makeIsParsed(textFirstEmptyLine));

    const char *const multipleEmptyLines = R"(
        define vifm_SOURCES :=


            bla$(cfg) $(compat) $(engine) endef $(int) $(io) $(menus) $(modes)
        endef
    )";
    CHECK(makeIsParsed(multipleEmptyLines));

    const char *const emptyLinesEverywhere = R"(
        define vifm_SOURCES :=

            $(cfg) $(compat) $(engine) endef $(int) $(io) $(menus) $(modes)

            something

        endef
    )";
    CHECK(makeIsParsed(emptyLinesEverywhere));

    const char *const keywordsInName = R"(
        define keywords
            (
            )
            ,
            # comment
            include
            override
            export
            unexport
            ifdef
            ifndef
            ifeq
            ifneq
            else
            endif
            define
            undefine
        endef
    )";
    CHECK(makeIsParsed(keywordsInName));
}

TEST_CASE("Line escaping works in a Makefile", "[make][parser]")
{
    const char *const multiline = R"(
        pos = $(strip $(eval T := ) \
                      $(eval i := -1) \
                      $(foreach elem, $1, \
                                $(if $(filter $2,$(elem)), \
                                              $(eval i := $(words $T)), \
                                              $(eval T := $T $(elem)))) \
                      $i)
    )";
    CHECK(makeIsParsed(multiline));
}

TEST_CASE("Substitutions are parsed in a Makefile", "[make][parser]")
{
    CHECK(makeIsParsed("lib_objects := $(lib_sources:%.cpp=$(out_dir)/%.o)"));
    CHECK(makeIsParsed("am__test_logs1 = $(TESTS:=.log)"));
}

TEST_CASE("Makefile keywords are not found inside text/ids", "[make][parser]")
{
    CHECK(makeIsParsed("EXTRA_CXXFLAGS += -fsanitize=undefined"));
}

TEST_CASE("Includes are parsed in a Makefile", "[make][parser]")
{
    CHECK(makeIsParsed("include $(wildcard *.d)"));
    CHECK(makeIsParsed("include config.mk"));
    CHECK(makeIsParsed("-include config.mk"));
}

TEST_CASE("Leading tabs are allowed not only for recipes in Makefiles",
          "[make][parser]")
{
    const char *const conditional = R"(
	ifeq (,$(findstring gcc,$(CC)))
		set := to this
	endif
    )";
    CHECK(makeIsParsed(conditional));

    const char *const emptyConditional = R"(
	ifeq (,$(findstring gcc,$(CC)))
	endif
    )";
    CHECK(makeIsParsed(emptyConditional));

    const char *const statements = R"(
	set := to this
	# comment
    )";
    CHECK(makeIsParsed(statements));
}

TEST_CASE("Column of recipe is computed correctly", "[make][parser]")
{
    std::string input = R"(
target:
	command1
	command2
    )";
    std::string expected = R"(
target:
    command1
    command2)";

    Tree tree = parseMake(input);

    std::string output = Highlighter(tree).print();
    CHECK(split(output, '\n') == split(expected, '\n'));
}

TEST_CASE("Export/unexport directives", "[make][parser]")
{
    CHECK(makeIsParsed("export"));
    CHECK(makeIsParsed("export var"));
    CHECK(makeIsParsed("export $(vars)"));
    CHECK(makeIsParsed("export var $(vars)"));
    CHECK(makeIsParsed("export var$(vars)"));

    CHECK(makeIsParsed("unexport"));
    CHECK(makeIsParsed("unexport var"));
    CHECK(makeIsParsed("unexport $(vars)"));
    CHECK(makeIsParsed("unexport var $(vars)"));
    CHECK(makeIsParsed("unexport var$(vars)"));
}

TEST_CASE("Undefine directive", "[make][parser]")
{
    CHECK(makeIsParsed("undefine something"));
    CHECK(makeIsParsed("undefine this and that"));
    CHECK(makeIsParsed("undefine some $(things)"));
    CHECK(makeIsParsed("undefine override something"));
    CHECK(makeIsParsed("override undefine something"));
}