#ifndef KERNEL_KEYMAPS_H
#define KERNEL_KEYMAPS_H

#include <stdint.h>

#define C(x) ((x) - '@')                ///< Key code for Ctrl+x

// Map scan codes in the "normal" state to key codes
static uint8_t
normalmap[256] =
{
  '\0', 0x1B, '1',  '2',  '3',  '4',  '5',  '6',    // 0x00
  '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
  'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',    // 0x10
  'o',  'p',  '[',  ']',  '\n', '\0', 'a',  's',
  'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',    // 0x20
  '\'', '`',  '\0', '\\', 'z',  'x',  'c',  'v',
  'b',  'n',  'm',  ',',  '.',  '/',  '\0', '*',    // 0x30
  '\0', ' ',  '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '7',    // 0x40
  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
  '2',  '3',  '0',  '.',  '\0', '\0', '\0', '\0',   // 0x50
  [0x9C]  '\n',
  [0xB5]  '/',
};

// Map scan codes in the "shift" state to key codes
static uint8_t
shiftmap[256] =
{
  '\0', 033,  '!',  '@',  '#',  '$',  '%',  '^',    // 0x00
  '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
  'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',    // 0x10
  'O',  'P',  '{',  '}',  '\n', '\0',  'A', 'S',
  'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',    // 0x20
  '"',  '~',  '\0',  '|',  'Z',  'X',  'C', 'V',
  'B',  'N',  'M',  '<',  '>',  '?',  '\0', '*',    // 0x30
  '\0', ' ',  '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '7',    // 0x40
  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
  '2',  '3',  '0',  '.',  '\0', '\0', '\0', '\0',   // 0x50
  [0x9C]  '\n',
  [0xB5]  '/',
};

// Map scan codes in the "ctrl" state to key codes
static uint8_t
ctrlmap[256] =
{
  '\0',   '\0',   '\0',   '\0',     '\0',   '\0',   '\0',   '\0',     // 0x00
  '\0',   '\0',   '\0',   '\0',     '\0',   '\0',   '\0',   '\0',
  C('Q'), C('W'), C('E'), C('R'),   C('T'), ('Y'),  C('U'), C('I'),   // 0x10
  C('O'), C('P'), '\0',   '\0',     '\r',   '\0',   C('A'), C('S'),
  C('D'), C('F'), C('G'), C('H'),   C('J'), C('K'), C('L'), '\0',     // 0x20
  '\0',   '\0',   '\0',   C('\\'),  C('Z'), C('X'), C('C'), C('V'),
  C('B'), C('N'), C('M'), '\0',    '\0',    C('/'), '\0',   '\0',     // 0x30
  [0x9C]  '\r',
  [0xB5]  C('/'),
};

// Map scan codes to key codes
static uint8_t *
keymaps[4] = {
  normalmap,
  shiftmap,
  ctrlmap,
  ctrlmap,
};

#endif  // !KERNEL_KEYMAPS_H