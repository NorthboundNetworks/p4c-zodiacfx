/*
Copyright 2019 Northbound Networks

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

#ifndef _BACKENDS_ZODIACFX_OPTIONS_H_
#define _BACKENDS_ZODIACFX_OPTIONS_H_

#include <getopt.h>
#include "frontends/common/options.h"

class ZodiacfxOptions : public CompilerOptions {
 public:
    // file to output to
    cstring outputFile = nullptr;
    bool parseOnly = false;
    bool validateOnly = false;
    bool loadIRFromJson = false;
    ZodiacfxOptions() {
        langVersion = CompilerOptions::FrontendVersion::P4_16;
        registerOption("-o", "outfile",
                        [this](const char* arg) { outputFile = arg; return true; },
                        "Write output to outfile");
        registerOption("--parse-only", nullptr,
                       [this](const char*) {
                           parseOnly = true;
                           return true; },
                       "only parse the P4 input, without any further processing");
        registerOption("--validate", nullptr,
                       [this](const char*) {
                           validateOnly = true;
                           return true;
                       },
                       "Validate the P4 input, running just the front-end");
        registerOption("--fromJSON", "file",
                       [this](const char* arg) {
                           loadIRFromJson = true;
                           file = arg;
                           return true;
                       },
                       "read previously dumped json instead of P4 source code");
     }
};

using ZodiacfxContext = P4CContextWithOptions<ZodiacfxOptions>;

#endif /* _BACKENDS_ZODIACFX_OPTIONS_H_ */
