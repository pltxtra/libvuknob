/*
 * vu|KNOB
 * (C) 2019 by Anton Persson
 *
 * http://www.vuknob.com/
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License version 2; see COPYING for the complete License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef MUSICAL_CONSTANTS_HH
#define MUSICAL_CONSTANTS_HH

#define __MIN_BPM__ 20
#define __MAX_BPM__ 200
#define __MIN_LPB__ 2
#define __MAX_LPB__ 24
#define __MIN_SHUFFLE__ 0
#define __MAX_SHUFFLE__ 100

#define BITS_PER_LINE 4
#define MACHINE_TICKS_PER_LINE (1 << BITS_PER_LINE)
#define MACHINE_LINE_BITMASK 0xffffff0
#define MACHINE_TICK_BITMASK 0x000000f
#define MAX_STATIC_SIGNALS 256

#endif
