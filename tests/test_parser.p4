/* -*- P4_16 -*- */
#include <core.p4>
#include <zodiacfx_model.p4>

const bit<16> TYPE_IPV4 = 0x800;

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

struct Headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800 : ip;
            default : accept;
        }
    }

    state ip {
        p.extract(headers.ipv4);
        transition accept;
    }
}

control swtch(inout Headers_t headers, in zodiacfx_input fxin, out zodiacfx_output fxout){
    apply {
        if (fxin.input_port == 1) {
        fxout.output_port = 2;
        } else {
        fxout.output_port = 1;
        }
    }
}
control deprs(in Headers_t headers, packet_out p, in zodiacfx_output fxout){
    apply {
        p.emit(headers.ethernet);
        p.emit(headers.ipv4);
    }
}

ZodiacfxSwitch(prs(), swtch(), deprs())main;
