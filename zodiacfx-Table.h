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

#ifndef _BACKENDS_ZODIACFX_ZODIACFXTABLE_H_
#define _BACKENDS_ZODIACFX_ZODIACFXTABLE_H_

#include "zodiacfx-Object.h"
#include "zodiacfx-Program.h"
#include "frontends/p4/methodInstance.h"

namespace ZODIACFX {
class ZODIACFXTableBase : public ZODIACFXObject {
 public:
    const ZODIACFXProgram* program;

    cstring instanceName;
    cstring keyTypeName;
    cstring valueTypeName;
    cstring dataMapName;
    CodeGenInspector* codeGen;

 protected:
    ZODIACFXTableBase(const ZODIACFXProgram* program, cstring instanceName,
                  CodeGenInspector* codeGen) :
            program(program), instanceName(instanceName), codeGen(codeGen) {
        CHECK_NULL(codeGen); CHECK_NULL(program);
        keyTypeName = program->refMap->newName(instanceName + "_key");
        valueTypeName = program->refMap->newName(instanceName + "_value");
        dataMapName = instanceName;
    }
};

class ZODIACFXTable final : public ZODIACFXTableBase {
 public:
    const IR::Key*            keyGenerator;
    const IR::ActionList*     actionList;
    const IR::TableBlock*    table;
    cstring               defaultActionMapName;
    cstring               actionEnumName;
    std::map<const IR::KeyElement*, cstring> keyFieldNames;
    std::map<const IR::KeyElement*, ZODIACFXType*> keyTypes;

    ZODIACFXTable(const ZODIACFXProgram* program, const IR::TableBlock* table, CodeGenInspector* codeGen);
    void emitTypes(CodeBuilder* builder);
    void emitInstance(CodeBuilder* builder);
    void emitActionArguments(CodeBuilder* builder, const IR::P4Action* action, cstring name);
    void emitKeyType(CodeBuilder* builder);
    void emitValueType(CodeBuilder* builder);
    void emitKey(CodeBuilder* builder, cstring keyName);
    void emitAction(CodeBuilder* builder, cstring valueName);
    void emitInitializer(CodeBuilder* builder);
};

}  // namespace ZODIACFX

#endif /* _BACKENDS_ZODIACFX_ZODIACFXTABLE_H_ */
