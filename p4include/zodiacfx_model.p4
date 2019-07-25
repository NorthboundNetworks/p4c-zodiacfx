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

#ifndef _ZodiacfxSwitch_MODEL_P4_
#define _ZodiacfxSwitch_MODEL_P4_

#include <core.p4>

/**
 Implementation property for tables indicating that tables must be implemented
 using ZodiacfxSwitch hash map.
*/
extern hash_table {
    /// @param size: maximum number of entries in table
    hash_table(bit<32> size);
}

/* architectural model for ZodiacfxSwitch packet switch target architecture */
struct zodiacfx_input {
    bit<32> input_port;// input port of the packet
}

struct zodiacfx_output {
    bit<32> output_port;  // output port for packet
}

parser zfx_parse<H>(packet_in packet, out H headers);
control zfx_switch<H>(inout H headers, in zodiacfx_input imd, out zodiacfx_output omd);
control zfx_deparse<H>(in H headers, packet_out packet, in zodiacfx_output omd);

package ZodiacfxSwitch<H>(zfx_parse<H> prs, zfx_switch<H> swtch, zfx_deparse<H> deprs);

#endif
