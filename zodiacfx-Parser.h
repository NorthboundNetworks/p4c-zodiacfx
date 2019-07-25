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

#ifndef _BACKENDS_ZODIACFX_ZODIACFXPARSER_H_
#define _BACKENDS_ZODIACFX_ZODIACFXPARSER_H_

#include "ir/ir.h"
#include "zodiacfx-Object.h"
#include "zodiacfx-Program.h"

namespace ZODIACFX {

class ZODIACFXParser;

class ZODIACFXParserState : public ZODIACFXObject {
 public:
    const IR::ParserState* state;
    const ZODIACFXParser* parser;

    ZODIACFXParserState(const IR::ParserState* state, ZODIACFXParser* parser) :
            state(state), parser(parser) {}
    void emit(CodeBuilder* builder);
};

class ZODIACFXParser : public ZODIACFXObject {
 public:
    const ZODIACFXProgram*            program;
    const P4::TypeMap*                typeMap;
    const IR::ParserBlock*            parserBlock;
    std::vector<ZODIACFXParserState*> states;
    const IR::Parameter*              packet;
    const IR::Parameter*              headers;
    ZODIACFXType*                     headerType;

    explicit ZODIACFXParser(const ZODIACFXProgram* program, const IR::ParserBlock* block, const P4::TypeMap* typeMap);
    void emitDeclaration(CodeBuilder* builder, const IR::Declaration* decl);
    void emit(CodeBuilder* builder);
    bool build();
};

}  // namespace ZODIACFX

#endif /* _BACKENDS_ZODIACFX_ZODIACFXPARSER_H_ */
