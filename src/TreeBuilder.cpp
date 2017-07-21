#include "TreeBuilder.hpp"

#include <cassert>
#include <cstddef>

#include <deque>
#include <functional>
#include <utility>
#include <vector>

std::ostream &
operator<<(std::ostream &os, SType stype)
{
    switch (stype) {
        case SType::None:                return (os << "None");
        case SType::TranslationUnit:     return (os << "TranslationUnit");
        case SType::Declaration:         return (os << "Declaration");
        case SType::Declarations:        return (os << "Declarations");
        case SType::FunctionDeclaration: return (os << "FunctionDeclaration");
        case SType::FunctionDefinition:  return (os << "FunctionDefinition");
        case SType::Directive:           return (os << "Directive");
        case SType::Comment:             return (os << "Comment");
        case SType::Macro:               return (os << "Macro");
        case SType::CompoundStatement:   return (os << "CompoundStatement");
        case SType::Separator:           return (os << "Separator");
        case SType::Statements:          return (os << "Statements");
        case SType::Statement:           return (os << "Statement");
        case SType::IfStmt:              return (os << "IfStmt");
        case SType::IfCond:              return (os << "IfCond");
        case SType::IfElse:              return (os << "IfElse");
        case SType::WhileStmt:           return (os << "WhileStmt");
        case SType::WhileCond:           return (os << "WhileCond");
        case SType::ForStmt:             return (os << "ForStmt");
        case SType::ForHead:             return (os << "ForHead");
        case SType::Expression:          return (os << "Expression");
        case SType::Declarator:          return (os << "Declarator");
        case SType::Initializer:         return (os << "Initializer");
        case SType::InitializerList:     return (os << "InitializerList");
        case SType::Specifiers:          return (os << "Specifiers");
        case SType::WithInitializer:     return (os << "WithInitializer");
        case SType::InitializerElement:  return (os << "InitializerElement");
    }

    assert("Unhandled enumeration item");
    return (os << "<UNKNOWN>");
}

PNode *
TreeBuilder::addNode(Text value, const Location &loc, int token, SType stype)
{
    value.token = token;

    if (value.postponedFrom != value.postponedTo) {
        std::vector<PNode *> children;
        children.reserve(value.postponedTo - value.postponedFrom);
        for (std::size_t i = value.postponedFrom; i < value.postponedTo; ++i) {
            nodes.emplace_back(postponed[i].value, postponed[i].loc,
                               postponed[i].stype, true);
            children.push_back(&nodes.back());
        }
        nodes.emplace_back(value, loc, stype);
        children.push_back(&nodes.back());
        nodes.emplace_back(std::move(children));
        return &nodes.back();
    }

    nodes.emplace_back(value, loc, stype);
    return &nodes.back();
}

PNode *
TreeBuilder::addNode(std::vector<PNode *> children, SType stype)
{
    for (unsigned int i = children.size(); i != 0U; --i) {
        movePostponed(children[i - 1U], children, children.cbegin() + (i - 1U));
    }
    nodes.emplace_back(std::move(children), stype);
    return &nodes.back();
}

void
TreeBuilder::finish(bool failed)
{
    if (failed) {
        this->failed = failed;
        return;
    }

    std::function<void(PNode *)> clean = [&](PNode *node) {
        std::vector<PNode *> &children = node->children;
        children.erase(children.begin(),
                       children.begin() + node->movedChildren);
        for (PNode *child : children) {
            clean(child);
        }
    };

    clean(root);

    std::vector<PNode *> children;
    children.reserve(newPostponed);
    for (std::size_t i = postponed.size() - newPostponed; i < postponed.size();
         ++i) {
        nodes.emplace_back(postponed[i].value, postponed[i].loc,
                           postponed[i].stype, true);
        children.push_back(&nodes.back());
    }

    root->children.insert(root->children.cend(),
                          children.cbegin(), children.cend());
}

void
TreeBuilder::movePostponed(PNode *&node, std::vector<PNode *> &nodes,
                           std::vector<PNode *>::const_iterator insertPos)
{
    auto pos = std::find_if_not(node->children.begin(), node->children.end(),
                                [](PNode *n) { return n->postponed; });
    if (pos == node->children.begin()) {
        return;
    }

    std::vector<PNode *> postponed(node->children.begin(), pos);
    node->movedChildren = postponed.size();

    if (node->children.end() - pos == 1U && (*pos)->empty()) {
        (*pos)->stype = node->stype;
        node = *pos;
    }

    nodes.insert(insertPos, postponed.cbegin(), postponed.cend());
}
