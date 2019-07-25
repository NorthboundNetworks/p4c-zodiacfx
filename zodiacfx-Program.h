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

#ifndef _BACKENDS_ZODIACFX_ZODIACFXPROGRAM_H_
#define _BACKENDS_ZODIACFX_ZODIACFXPROGRAM_H_

#include "zodiacfx-Target.h"
#include "zodiacfx-Model.h"
#include "zodiacfx-Object.h"
#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/common/options.h"
#include "zodiacfx-CodeGen.h"

namespace ZODIACFX {

class ZODIACFXProgram;
class ZODIACFXParser;
class ZODIACFXControl;
class ZODIACFXDeparser;
class ZODIACFXTable;
class ZODIACFXType;

class ZODIACFXProgram : public ZODIACFXObject {
 public:
    const CompilerOptions& options;
    const IR::P4Program* program;
    const IR::ToplevelBlock*  toplevel;
    P4::ReferenceMap*    refMap;
    P4::TypeMap*         typeMap;
    ZODIACFXParser*      parser;
    ZODIACFXDeparser*    deparser;
    ZODIACFXControl*     control;
    ZODIACFXModel        &model;

    cstring endLabel, offsetVar, lengthVar;
    cstring zeroKey, functionName, errorVar;
    cstring packetStartVar, byteVar;
    cstring errorEnum;
    cstring license = "GPL";  // TODO: this should be a compiler option probably
    cstring arrayIndexType = "uint32_t";
    cstring inPacketLengthVar, outHeaderLengthVar;

    virtual bool build();  // return 'true' on success

    ZODIACFXProgram(const CompilerOptions &options, const IR::P4Program* program,
                P4::ReferenceMap* refMap, P4::TypeMap* typeMap, const IR::ToplevelBlock* toplevel) :
            options(options), program(program), toplevel(toplevel),
            refMap(refMap), typeMap(typeMap),
            parser(nullptr), control(nullptr), model(ZODIACFXModel::instance) {
        offsetVar = ZODIACFXModel::reserved("packetOffsetInBits");
        packetStartVar = ZODIACFXModel::reserved("packetStart");
        zeroKey = ZODIACFXModel::reserved("zero");
        functionName = ZODIACFXModel::reserved("zfx_switch");
        byteVar = ZODIACFXModel::reserved("byte");
        inPacketLengthVar = ZODIACFXModel::reserved("ul_size");
        outHeaderLengthVar = ZODIACFXModel::reserved("outHeaderLength");
        endLabel = ZODIACFXModel::reserved("end");
    }

    virtual void emitGeneratedComment(CodeBuilder* builder);
    virtual void emitPreamble(CodeBuilder* builder);
    virtual void emitTypes(CodeBuilder* builder);
    virtual void emitHeaderInstances(CodeBuilder* builder);
    virtual void emitLocalVariables(CodeBuilder* builder);
    virtual void emitPipeline(CodeBuilder* builder);
    virtual void emitH(CodeBuilder* builder, cstring headerFile);  // emits C headers
    virtual void emitC(CodeBuilder* builder, cstring headerFile);  // emits C program
    ZODIACFXControl* getSwitch() const;
};

}  // namespace ZODIACFX

#endif /* _BACKENDS_ZODIACFX_ZODIACFXPROGRAM_H_ */
