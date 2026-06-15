/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2026 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _PIANO_ROLL_H
#define _PIANO_ROLL_H

// piano roll interaction state types, kept out of the main GUI header

struct PrDragNote { int row, endRow, chan; short note, ins, vol; };
struct PrFxEntry { unsigned char code; char label[64]; };
struct PrClipEntry { int rowOff; short note; short ins; short vol; };
struct PrFxClipEntry { int rowOff; short code; short val; };
struct PrPolyGroup { int from, to; };
// a pitch-contour point in song coords (row plus absolute semitones)
struct PrPitchPoint { float row, pitch; int note; bool brk; };

#endif
