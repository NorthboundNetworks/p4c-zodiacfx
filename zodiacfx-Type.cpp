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

#include "zodiacfx-Type.h"

namespace ZODIACFX {

ZODIACFXTypeFactory* ZODIACFXTypeFactory::instance;

ZODIACFXType* ZODIACFXTypeFactory::create(const IR::Type* type) {
    CHECK_NULL(type);
    CHECK_NULL(typeMap);
    ZODIACFXType* result = nullptr;
    if (type->is<IR::Type_Boolean>()) {
        result = new ZODIACFXBoolType();
    } else if (auto bt = type->to<IR::Type_Bits>()) {
        result = new ZODIACFXScalarType(bt);
    } else if (auto st = type->to<IR::Type_StructLike>()) {
        result = new ZODIACFXStructType(st);
    } else if (auto tt = type->to<IR::Type_Typedef>()) {
        auto canon = typeMap->getTypeType(type, true);
        result = create(canon);
        auto path = new IR::Path(tt->name);
        result = new ZODIACFXTypeName(new IR::Type_Name(path), result);
    } else if (auto tn = type->to<IR::Type_Name>()) {
        auto canon = typeMap->getTypeType(type, true);
        result = create(canon);
        result = new ZODIACFXTypeName(tn, result);
    } else if (auto te = type->to<IR::Type_Enum>()) {
        result = new ZODIACFXEnumType(te);
    } else if (auto ts = type->to<IR::Type_Stack>()) {
        auto et = create(ts->elementType);
        if (et == nullptr)
            return nullptr;
        result = new ZODIACFXStackType(ts, et);
    } else {
        ::error("Type %1% not supported", type);
    }

    return result;
}

void
ZODIACFXBoolType::declare(CodeBuilder* builder, cstring id, bool asPointer) {
    emit(builder);
    if (asPointer)
        builder->append("*");
    builder->appendFormat(" %s", id.c_str());
}

/////////////////////////////////////////////////////////////

void ZODIACFXStackType::declare(CodeBuilder* builder, cstring id, bool) {
    elementType->declareArray(builder, id, size);
}

void ZODIACFXStackType::emitInitializer(CodeBuilder* builder) {
    builder->append("{");
    for (unsigned i = 0; i < size; i++) {
        if (i > 0)
            builder->append(", ");
        elementType->emitInitializer(builder);
    }
    builder->append(" }");
}

unsigned ZODIACFXStackType::widthInBits() {
    return size * elementType->to<IHasWidth>()->widthInBits();
}

unsigned ZODIACFXStackType::implementationWidthInBits() {
    return size * elementType->to<IHasWidth>()->implementationWidthInBits();
}

/////////////////////////////////////////////////////////////

unsigned ZODIACFXScalarType::alignment() const {
    if (width <= 8)
        return 1;
    else if (width <= 16)
        return 2;
    else if (width <= 32)
        return 4;
    else if (width <= 64)
        return 8;
    else
        // compiled as uint8_t*
        return 1;
}

void ZODIACFXScalarType::emit(CodeBuilder* builder) {
    auto prefix = isSigned ? "int" : "uint";

    if (width <= 8)
        builder->appendFormat("%s8_t", prefix);
    else if (width <= 16)
        builder->appendFormat("%s16_t", prefix);
    else if (width <= 32)
        builder->appendFormat("%s32_t", prefix);
    else if (width <= 64)
        builder->appendFormat("%s64_t", prefix);
    else
        builder->appendFormat("uint8_t*");
}

void
ZODIACFXScalarType::declare(CodeBuilder* builder, cstring id, bool asPointer) {
    if (ZODIACFXScalarType::generatesScalar(width)) {
        emit(builder);
        if (asPointer)
            builder->append("*");
        builder->spc();
        builder->append(id);
    } else {
        if (asPointer)
            builder->append("uint8_t*");
        else
            builder->appendFormat("uint8_t %s[%d]", id.c_str(), bytesRequired());
    }
}

//////////////////////////////////////////////////////////

ZODIACFXStructType::ZODIACFXStructType(const IR::Type_StructLike* strct) :
        ZODIACFXType(strct) {
    if (strct->is<IR::Type_Struct>())
        kind = "struct";
    else if (strct->is<IR::Type_Header>())
        kind = "struct";
    else if (strct->is<IR::Type_HeaderUnion>())
        kind = "union";
    else
        BUG("Unexpected struct type %1%", strct);
    name = strct->name.name;
    width = 0;
    implWidth = 0;

    for (auto f : strct->fields) {
        auto type = ZODIACFXTypeFactory::instance->create(f->type);
        auto wt = dynamic_cast<IHasWidth*>(type);
        if (wt == nullptr) {
            ::error("ZODIACFX: Unsupported type in struct: %s", f->type);
        } else {
            width += wt->widthInBits();
            implWidth += wt->implementationWidthInBits();
        }
        fields.push_back(new ZODIACFXField(type, f));
    }
}

void
ZODIACFXStructType::declare(CodeBuilder* builder, cstring id, bool asPointer) {
    builder->append(kind);
    if (asPointer)
        builder->append("*");
    builder->appendFormat(" %s %s", name.c_str(), id.c_str());
}

void ZODIACFXStructType::emitInitializer(CodeBuilder* builder) {
    builder->blockStart();
    if (type->is<IR::Type_Struct>() || type->is<IR::Type_HeaderUnion>()) {
        for (auto f : fields) {
            builder->emitIndent();
            builder->appendFormat(".%s = ", f->field->name.name);
            f->type->emitInitializer(builder);
            builder->append(",");
            builder->newline();
        }
    } else if (type->is<IR::Type_Header>()) {
        builder->emitIndent();
        builder->appendLine(".zodiacfx_valid = 0");
    } else {
        BUG("Unexpected type %1%", type);
    }
    builder->blockEnd(false);
}

void ZODIACFXStructType::emit(CodeBuilder* builder) {
    builder->emitIndent();
    builder->append(kind);
    builder->spc();
    builder->append(name);
    builder->spc();
    builder->blockStart();

    for (auto f : fields) {
        auto type = f->type;
        builder->emitIndent();

        type->declare(builder, f->field->name, false);
        builder->append("; ");
        builder->append("/* ");
        builder->append(type->type->toString());
        if (f->comment != nullptr) {
            builder->append(" ");
            builder->append(f->comment);
        }
        builder->append(" */");
        builder->newline();
    }

    if (type->is<IR::Type_Header>()) {
        builder->emitIndent();
        auto type = ZODIACFXTypeFactory::instance->create(IR::Type_Boolean::get());
        if (type != nullptr) {
            type->declare(builder, "zodiacfx_valid", false);
            builder->endOfStatement(true);
        }
    }

    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void
ZODIACFXStructType::declareArray(CodeBuilder* builder, cstring id, unsigned size) {
    builder->appendFormat("%s %s[%d]", name.c_str(), id.c_str(), size);
}

///////////////////////////////////////////////////////////////

void ZODIACFXTypeName::declare(CodeBuilder* builder, cstring id, bool asPointer) {
    if (canonical != nullptr)
        canonical->declare(builder, id, asPointer);
}

void ZODIACFXTypeName::emitInitializer(CodeBuilder* builder) {
    if (canonical != nullptr)
        canonical->emitInitializer(builder);
}

unsigned ZODIACFXTypeName::widthInBits() {
    auto wt = dynamic_cast<IHasWidth*>(canonical);
    if (wt == nullptr) {
        ::error("Type %1% does not have a fixed witdh", type);
        return 0;
    }
    return wt->widthInBits();
}

unsigned ZODIACFXTypeName::implementationWidthInBits() {
    auto wt = dynamic_cast<IHasWidth*>(canonical);
    if (wt == nullptr) {
        ::error("Type %1% does not have a fixed witdh", type);
        return 0;
    }
    return wt->implementationWidthInBits();
}

void
ZODIACFXTypeName::declareArray(CodeBuilder* builder, cstring id, unsigned size) {
    declare(builder, id, false);
    builder->appendFormat("[%d]", size);
}

////////////////////////////////////////////////////////////////

void ZODIACFXEnumType::declare(ZODIACFX::CodeBuilder* builder, cstring id, bool asPointer) {
    builder->append("enum ");
    builder->append(getType()->name);
    if (asPointer)
        builder->append("*");
    builder->append(" ");
    builder->append(id);
}

void ZODIACFXEnumType::emit(ZODIACFX::CodeBuilder* builder) {
    builder->append("enum ");
    auto et = getType();
    builder->append(et->name);
    builder->blockStart();
    for (auto m : et->members) {
        builder->append(m->name);
        builder->appendLine(",");
    }
    builder->blockEnd(true);
}

}  // namespace ZODIACFX
