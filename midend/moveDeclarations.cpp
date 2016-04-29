#include "moveDeclarations.h"

namespace P4 {

const IR::Node* MoveDeclarations::postorder(IR::P4Action* action)  {
    if (findContext<IR::P4Control>() == nullptr) {
        // Else let the parent control get these
        auto body = new IR::Vector<IR::StatOrDecl>();
        auto m = getMoves();
        body->insert(body->end(), m->begin(), m->end());
        body->append(*action->body);
        action->body = body;
        pop();
    }
    return action;
}

const IR::Node* MoveDeclarations::postorder(IR::P4Control* control)  {
    LOG1("Visiting " << control << " toMove " << toMove.size());
    auto decls = new IR::NameMap<IR::Declaration, ordered_map>();
    for (auto decl : *getMoves()) {
        LOG1("Moved " << decl);
        decls->addUnique(decl->name, decl);
    }
    for (auto stat : control->stateful)
        decls->addUnique(stat.first, stat.second);
    control->stateful = std::move(*decls);
    pop();
    return control;
}

const IR::Node* MoveDeclarations::postorder(IR::P4Parser* parser)  {
    auto newStateful = new IR::Vector<IR::Declaration>();
    newStateful->append(*getMoves());
    newStateful->append(*parser->stateful);
    parser->stateful = newStateful;
    pop();
    return parser;
}

const IR::Node* MoveDeclarations::postorder(IR::Declaration_Variable* decl) {
    // We must keep the initializer here
    if (decl->initializer != nullptr) {
        LOG1("Moving and splitting " << decl);
        auto move = new IR::Declaration_Variable(decl->srcInfo, decl->name,
                                                 decl->annotations, decl->type, nullptr);
        addMove(move);
        auto varRef = new IR::PathExpression(decl->name);
        auto keep = new IR::AssignmentStatement(decl->srcInfo, varRef, decl->initializer);
        return keep;
    } else {
        LOG1("Moving " << decl);
        addMove(decl);
        return nullptr;
    }
}

const IR::Node* MoveDeclarations::postorder(IR::Declaration_Constant* decl) {
    if (findContext<IR::P4Control>() == nullptr &&
        findContext<IR::P4Action>() == nullptr &&
        findContext<IR::P4Parser>() == nullptr)
        // This is a global declaration
        return decl;
    addMove(decl);
    return nullptr;
}

}  // namespace P4
