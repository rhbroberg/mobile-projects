#pragma once

#include "vmtype.h"

const VMUINT8 server_uuid[] = { 0x19, 0xA0, 0x1F, 0x49, 0xFF, 0xE5, 0x40, 0x56, 0x84, 0x5B, 0x6D, 0xF1, 0xF1, 0xB1, 0x6E, 0x9D };

const VMUINT8 my_service[] = { 0xFD, 0x36, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x19, 0x2A, 0x01, 0xFE };
const VMUINT8 my_other_service[] = { 0xF0, 0x40, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x19, 0x2A, 0x09, 0xFA };

const VMUINT8 my_char[] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x19, 0x2A, 0x00, 0xFF };
const VMUINT8 my_c2[] = { 0xFC, 0x35, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x19, 0x2A, 0x02, 0xFD };
const VMUINT8 my_s2c1[] = { 0xFC, 0x35, 0x9B, 0x5F, 0x90, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x01, 0x19, 0x2A, 0x02, 0xFD };
const VMUINT8 my_s2c2[] = { 0xFC, 0x35, 0x9B, 0x5F, 0xA0, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x02, 0x19, 0x2A, 0x02, 0xFD };
const VMUINT8 my_s2c3[] = { 0xFB, 0x34, 0x9B, 0x5F, 0xA0, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x02, 0x19, 0x2A, 0x03, 0xFC };
const VMUINT8 my_s2c4[] = { 0xFA, 0x33, 0x9B, 0x5F, 0xA0, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x02, 0x19, 0x2A, 0x04, 0xFB };