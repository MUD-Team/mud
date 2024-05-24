// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: e5f39f8b253e13785095fce2330d3a81b8bccb51 $
//
// Copyright (C) 2006-2020 by The Odamex Team.
// Copyright (C) 2024 by The MUD Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 3
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//  The idea here is to use huffman coding without having to send
//  the huffman tree explicitly across the network.
//
//		* For each packet, the client sends an ACK
//		* For each ACK the server gets back, both client and server have a
//			copy of the packet
//		* Statistically over time, packets contain similar data
//
//  Therefore:
//  -> Client and server can build huffman trees using past packet data
//  -> Client and server can use this tree to compress new data
//
//-----------------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include <cstring>

#include <iostream>

class huffman
{
    // Structures
    struct huff_bitstream_t
    {
        uint8_t *BytePtr;
        uint32_t   BitPos;
    };

    struct huff_sym_t
    {
        int32_t          Symbol;
        uint32_t Count;
        uint32_t Code;
        uint32_t Bits;
    };

    struct huff_encodenode_t
    {
        huff_encodenode_t *ChildA, *ChildB;
        int32_t                Count;
        int32_t                Symbol;
    };

    // Histogram of character frequency
    huff_sym_t   sym[256];
    uint32_t total_count;

    // Flag to indicate that tree needs rebuilding
    bool fresh_histogram;

    // Tree generated from the histogram
    /* The maximum number of nodes in the Huffman tree is 2^(8+1)-1 = 511 */
    huff_encodenode_t  nodes[511];
    huff_encodenode_t *root;

    void               _Huffman_InitBitstream(huff_bitstream_t *stream, uint8_t *buf);
    uint32_t       _Huffman_ReadBit(huff_bitstream_t *stream);
    uint32_t       _Huffman_Read8Bits(huff_bitstream_t *stream);
    void               _Huffman_WriteBits(huff_bitstream_t *stream, uint32_t x, uint32_t bits);
    void               _Huffman_Hist(uint8_t *in, huff_sym_t *sym, uint32_t size);
    huff_encodenode_t *_Huffman_MakeTree(huff_sym_t *sym, huff_encodenode_t *nodes);
    void _Huffman_StoreTree(huff_encodenode_t *node, huff_sym_t *sym, uint32_t code, uint32_t bits);

    bool Huffman_Compress_Using_Histogram(uint8_t *in, size_t insize, uint8_t *out, size_t &outsize,
                                          huff_sym_t *sym);
    bool Huffman_Uncompress_Using_Tree(uint8_t *in, size_t insize, uint8_t *out, size_t &outsize,
                                       huff_encodenode_t *tree_root);

  public:
    // Clear statistics
    void reset();

    // Analyse some raw data and add it to the compression statistics
    void extend(uint8_t *data, size_t len);

    // Compress a chunk of data using only previously generated stats
    bool compress(uint8_t *in_data, size_t in_len, uint8_t *out_data, size_t &out_len);

    // Decompress a chunk of data using only previously generated stats
    bool decompress(uint8_t *in_data, size_t in_len, uint8_t *out_data, size_t &out_len);

    // For debugging, this count can be used to see if two codecs have had the same length input
    int32_t get_count()
    {
        return total_count;
    }

    // Constructor
    huffman();

    // Copying invalidates all tree pointers
    huffman(const huffman &other) : total_count(other.total_count), fresh_histogram(true)
    {
        memcpy(sym, other.sym, sizeof(sym));
    }
};

#define HUFFMAN_RENEGOTIATE_DELAY 256

class huffman_server
{
    huffman alpha, beta, tmpcodec;
    bool    active_codec;

    uint32_t last_packet_id, last_ack_id;

    uint32_t missed_acks;

    bool awaiting_ack;

  public:
    huffman &get_codec()
    {
        return active_codec ? alpha : beta;
    }
    uint8_t get_codec_id()
    {
        return active_codec ? 1 : 0;
    }

    bool packet_sent(uint32_t id, uint8_t *in_data, size_t len);
    void packet_acked(uint32_t id);

    huffman_server() : active_codec(0), last_packet_id(0), last_ack_id(0), missed_acks(0), awaiting_ack(false)
    {
    }
    huffman_server(const huffman_server &other)
        : alpha(other.alpha), beta(other.beta), tmpcodec(other.tmpcodec), active_codec(other.active_codec),
          last_packet_id(other.last_packet_id), last_ack_id(other.last_ack_id), missed_acks(other.missed_acks),
          awaiting_ack(other.awaiting_ack)
    {
    }
};

class huffman_client
{
    huffman alpha, beta, tmpcodec;
    bool    active_codec;

    bool awaiting_ackack;

  public:
    void reset();

    void     ack_sent(uint8_t *in_data, size_t len);
    huffman &codec_for_received(uint8_t id);

    huffman_client()
    {
        reset();
    }
    huffman_client(const huffman_client &other)
        : alpha(other.alpha), beta(other.beta), tmpcodec(other.tmpcodec), active_codec(other.active_codec),
          awaiting_ackack(other.awaiting_ackack)
    {
    }
};
