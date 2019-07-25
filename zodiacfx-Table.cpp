/*
Copyright 2019 Northbound Networks.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "zodiacfx-Table.h"
#include "zodiacfx-Type.h"
#include "ir/ir.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"

namespace ZODIACFX {

namespace {
class ActionTranslationVisitor : public CodeGenInspector {
 protected:
    const ZODIACFXProgram*  program;
    const IR::P4Action* action;
    cstring             valueName;

 public:
    ActionTranslationVisitor(cstring valueName, const ZODIACFXProgram* program):
            CodeGenInspector(program->refMap, program->typeMap), program(program),
            action(nullptr), valueName(valueName)
    { CHECK_NULL(program); }

    bool preorder(const IR::PathExpression* expression) {
        auto decl = program->refMap->getDeclaration(expression->path, true);
        if (decl->is<IR::Parameter>()) {
            auto param = decl->to<IR::Parameter>();
            bool isParam = action->parameters->getParameter(param->name) == param;
            if (isParam) {
                builder->append(valueName);
                builder->append("->u.");
                cstring name = ZODIACFXObject::externalName(action);
                builder->append(name);
                builder->append(".");
                builder->append(expression->path->toString());  // original name
                return false;
            }
        }
        visit(expression->path);
        return false;
    }

    bool preorder(const IR::P4Action* act) {
        action = act;
        visit(action->body);
        return false;
    }
};  // ActionTranslationVisitor
}  // namespace

////////////////////////////////////////////////////////////////

ZODIACFXTable::ZODIACFXTable(const ZODIACFXProgram* program, const IR::TableBlock* table,
                     CodeGenInspector* codeGen) :
        ZODIACFXTableBase(program, ZODIACFXObject::externalName(table->container), codeGen), table(table) {
    cstring base = instanceName + "_defaultAction";
    defaultActionMapName = program->refMap->newName(base);

    base = table->container->name.name + "_actions";
    actionEnumName = program->refMap->newName(base);

    keyGenerator = table->container->getKey();
    actionList = table->container->getActionList();
}

void ZODIACFXTable::emitKeyType(CodeBuilder* builder) {
    builder->emitIndent();
    builder->appendFormat("struct %s ", keyTypeName.c_str());
    builder->blockStart();

    CodeGenInspector commentGen(program->refMap, program->typeMap);
    commentGen.setBuilder(builder);

    if (keyGenerator != nullptr) {
        // Use this to order elements by size
        std::multimap<size_t, const IR::KeyElement*> ordered;
        unsigned fieldNumber = 0;
        for (auto c : keyGenerator->keyElements) {
            auto type = program->typeMap->getType(c->expression);
            auto zodiacfxType = ZODIACFXTypeFactory::instance->create(type);
            cstring fieldName = cstring("field") + Util::toString(fieldNumber);
            if (!zodiacfxType->is<IHasWidth>()) {
                ::error("%1%: illegal type %2% for key field", c, type);
                return;
            }
            unsigned width = zodiacfxType->to<IHasWidth>()->widthInBits();
            ordered.emplace(width, c);
            keyTypes.emplace(c, zodiacfxType);
            keyFieldNames.emplace(c, fieldName);
            fieldNumber++;
        }

        // Emit key in decreasing order size - this way there will be no gaps
        for (auto it = ordered.rbegin(); it != ordered.rend(); ++it) {
            auto c = it->second;

            auto zodiacfxType = ::get(keyTypes, c);
            builder->emitIndent();
            cstring fieldName = ::get(keyFieldNames, c);
            zodiacfxType->declare(builder, fieldName, false);
            builder->append("; /* ");
            c->expression->apply(commentGen);
            builder->append(" */");
            builder->newline();

            auto mtdecl = program->refMap->getDeclaration(c->matchType->path, true);
            auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();
            if (matchType->name.name != P4::P4CoreLibrary::instance.exactMatch.name)
                ::error("Match of type %1% not supported", c->matchType);
        }
    }

    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void ZODIACFXTable::emitActionArguments(CodeBuilder* builder,
                                    const IR::P4Action* action, cstring name) {
    builder->emitIndent();
    builder->append("struct ");
    builder->blockStart();

    for (auto p : *action->parameters->getEnumerator()) {
        builder->emitIndent();
        auto type = ZODIACFXTypeFactory::instance->create(p->type);
        type->declare(builder, p->name.name, false);
        builder->endOfStatement(true);
    }

    builder->blockEnd(false);
    builder->spc();
    builder->append(name);
    builder->endOfStatement(true);
}

void ZODIACFXTable::emitValueType(CodeBuilder* builder) {
    // create an enum with tags for all actions
    builder->emitIndent();
    builder->append("enum ");
    builder->append(actionEnumName);
    builder->spc();
    builder->blockStart();

    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
        cstring name = ZODIACFXObject::externalName(action);
        builder->emitIndent();
        builder->append(name);
        builder->append(",");
        builder->newline();
    }

    builder->blockEnd(false);
    builder->endOfStatement(true);

    // a type-safe union: a struct with a tag and an union
    builder->emitIndent();
    builder->appendFormat("struct %s ", valueTypeName.c_str());
    builder->blockStart();

    builder->emitIndent();
    builder->appendFormat("enum %s action;", actionEnumName.c_str());
    builder->newline();

    builder->emitIndent();
    builder->append("union ");
    builder->blockStart();

    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
        cstring name = ZODIACFXObject::externalName(action);
        emitActionArguments(builder, action, name);
    }

    builder->blockEnd(false);
    builder->spc();
    builder->appendLine("u;");
    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void ZODIACFXTable::emitTypes(CodeBuilder* builder) {
    emitKeyType(builder);
    emitValueType(builder);
}

void ZODIACFXTable::emitInstance(CodeBuilder* builder) {
    if (keyGenerator != nullptr) {
        auto impl = table->container->properties->getProperty(
            program->model.tableImplProperty.name);
        if (impl == nullptr) {
            ::error("Table %1% does not have an %2% property",
                    table->container, program->model.tableImplProperty.name);
            return;
        }

        // Some type checking...
        if (!impl->value->is<IR::ExpressionValue>()) {
            ::error("%1%: Expected property to be an `extern` block", impl);
            return;
        }

        auto expr = impl->value->to<IR::ExpressionValue>()->expression;
        if (!expr->is<IR::ConstructorCallExpression>()) {
            ::error("%1%: Expected property to be an `extern` block", impl);
            return;
        }

        auto block = table->getValue(expr);
        if (block == nullptr || !block->is<IR::ExternBlock>()) {
            ::error("%1%: Expected property to be an `extern` block", impl);
            return;
        }

        bool isHash;
        auto extBlock = block->to<IR::ExternBlock>();
        if (extBlock->type->name.name == program->model.hash_table.name) {
            isHash = true;
        } else {
            ::error("%1%: implementation must be one of %3%",
                    impl, program->model.hash_table.name);
            return;
        }

        cstring name = ZODIACFXObject::externalName(table->container);
        builder->target->emitTableDecl(builder, name, isHash,
                                       cstring("struct ") + keyTypeName,
                                       cstring("struct ") + valueTypeName, 0);
    }
    builder->target->emitTableDecl(builder, defaultActionMapName, false,
                                   program->arrayIndexType,
                                   cstring("struct ") + valueTypeName, 1);
}

void ZODIACFXTable::emitKey(CodeBuilder* builder, cstring keyName) {
    if (keyGenerator == nullptr)
        return;
    for (auto c : keyGenerator->keyElements) {
        auto zodiacfxType = ::get(keyTypes, c);
        cstring fieldName = ::get(keyFieldNames, c);
        CHECK_NULL(fieldName);
        bool memcpy = false;
        ZODIACFXScalarType* scalar = nullptr;
        unsigned width = 0;
        if (zodiacfxType->is<ZODIACFXScalarType>()) {
            scalar = zodiacfxType->to<ZODIACFXScalarType>();
            width = scalar->implementationWidthInBits();
            memcpy = !ZODIACFXScalarType::generatesScalar(width);
        }

        builder->emitIndent();
        if (memcpy) {
            builder->appendFormat("memcpy(&%s.%s, &", keyName.c_str(), fieldName.c_str());
            codeGen->visit(c->expression);
            builder->appendFormat(", %d)", scalar->bytesRequired());
        } else {
            builder->appendFormat("%s.%s = ", keyName.c_str(), fieldName.c_str());
            codeGen->visit(c->expression);
        }
        builder->endOfStatement(true);
    }
}

void ZODIACFXTable::emitAction(CodeBuilder* builder, cstring valueName) {
    builder->emitIndent();
    builder->appendFormat("switch (%s->action) ", valueName.c_str());
    builder->blockStart();

    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
        builder->emitIndent();
        cstring name = ZODIACFXObject::externalName(action);
        builder->appendFormat("case %s: ", name.c_str());
        builder->newline();
        builder->emitIndent();

        ActionTranslationVisitor visitor(valueName, program);
        visitor.setBuilder(builder);
        visitor.copySubstitutions(codeGen);

        action->apply(visitor);
        builder->newline();
        builder->emitIndent();
        builder->appendLine("break;");
    }

    builder->emitIndent();
    builder->appendFormat("default: return %s", builder->target->abortReturnCode().c_str());
    builder->endOfStatement(true);

    builder->blockEnd(true);
}

void ZODIACFXTable::emitInitializer(CodeBuilder* builder) {
    // emit code to initialize the default action
    const IR::P4Table* t = table->container;
    const IR::Expression* defaultAction = t->getDefaultAction();
    BUG_CHECK(defaultAction->is<IR::MethodCallExpression>(),
              "%1%: expected an action call", defaultAction);
    auto mce = defaultAction->to<IR::MethodCallExpression>();
    auto mi = P4::MethodInstance::resolve(mce, program->refMap, program->typeMap);

    auto ac = mi->to<P4::ActionCall>();
    BUG_CHECK(ac != nullptr, "%1%: expected an action call", mce);
    auto action = ac->action;
    cstring name = ZODIACFXObject::externalName(action);
    cstring fd = "tableFileDescriptor";
    cstring defaultTable = defaultActionMapName;
    cstring value = "value";
    cstring key = "key";

    builder->emitIndent();
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("int %s = BPF_OBJ_GET(MAP_PATH \"/%s\")",
                          fd.c_str(), defaultTable.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("if (%s < 0) { fprintf(stderr, \"map %s not loaded\\n\"); exit(1); }",
                          fd.c_str(), defaultTable.c_str());
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("struct %s %s = ", valueTypeName.c_str(), value.c_str());
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat(".action = %s,", name.c_str());
    builder->newline();

    CodeGenInspector cg(program->refMap, program->typeMap);
    cg.setBuilder(builder);

    builder->emitIndent();
    builder->appendFormat(".u = {.%s = {", name.c_str());
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        auto arg = mi->substitution.lookup(p);
        arg->apply(cg);
        builder->append(",");
    }
    builder->append("}},\n");

    builder->blockEnd(false);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->append("int ok = ");
    builder->target->emitUserTableUpdate(builder, fd, program->zeroKey, value);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("if (ok != 0) { "
                          "perror(\"Could not write in %s\"); exit(1); }",
                          defaultTable.c_str());
    builder->newline();
    builder->blockEnd(true);

    // Emit code for table initializer
    auto entries = t->getEntries();
    if (entries == nullptr)
        return;

    builder->emitIndent();
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("int %s = BPF_OBJ_GET(MAP_PATH \"/%s\")",
                          fd.c_str(), dataMapName.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("if (%s < 0) { fprintf(stderr, \"map %s not loaded\\n\"); exit(1); }",
                          fd.c_str(), dataMapName.c_str());
    builder->newline();

    for (auto e : entries->entries) {
        builder->emitIndent();
        builder->blockStart();

        auto entryAction = e->getAction();
        builder->emitIndent();
        builder->appendFormat("struct %s %s = {", keyTypeName.c_str(), key.c_str());
        e->getKeys()->apply(cg);
        builder->append("}");
        builder->endOfStatement(true);

        BUG_CHECK(entryAction->is<IR::MethodCallExpression>(),
                  "%1%: expected an action call", defaultAction);
        auto mce = entryAction->to<IR::MethodCallExpression>();
        auto mi = P4::MethodInstance::resolve(mce, program->refMap, program->typeMap);

        auto ac = mi->to<P4::ActionCall>();
        BUG_CHECK(ac != nullptr, "%1%: expected an action call", mce);
        auto action = ac->action;
        cstring name = ZODIACFXObject::externalName(action);

        builder->emitIndent();
        builder->appendFormat("struct %s %s = ",
                              valueTypeName.c_str(), value.c_str());
        builder->blockStart();
        builder->emitIndent();
        builder->appendFormat(".action = %s,", name.c_str());
        builder->newline();

        CodeGenInspector cg(program->refMap, program->typeMap);
        cg.setBuilder(builder);

        builder->emitIndent();
        builder->appendFormat(".u = {.%s = {", name.c_str());
        for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
            auto arg = mi->substitution.lookup(p);
            arg->apply(cg);
            builder->append(",");
        }
        builder->append("}},\n");

        builder->blockEnd(false);
        builder->endOfStatement(true);

        builder->emitIndent();
        builder->append("int ok = ");
        builder->target->emitUserTableUpdate(builder, fd, key, value);
        builder->newline();

        builder->emitIndent();
        builder->appendFormat("if (ok != 0) { "
                              "perror(\"Could not write in %s\"); exit(1); }",
                              t->name.name.c_str());
        builder->newline();
        builder->blockEnd(true);
    }
    builder->blockEnd(true);
}

}  // namespace ZODIACFX
