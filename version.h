/*
#
# Copyright (C) 2010 by Chris Evich <cevich@redhat.com>
#
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Lesser General Public
#   License as published by the Free Software Foundation; either
#   version 2.1 of the License, or (at your option) any later version.
#
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public
#   License along with this library; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
*/

#ifndef VERSION_H
#define VERSION_H

/**************************************************
********************* MACROS
**************************************************/
#define PROGNAM "ringwrap"
#define PROGVER_X_s "1"
#define PROGVER_Y_s "2"
#define PROGVER_Z_s "1"
#define PROGREL_s "1"
#define PROGVER_X strtoul(PROGVER_X_s, NULL, 0)
#define PROGVER_Y strtoul(PROGVER_Y_s, NULL, 0)
#define PROGVER_Z strtoul(PROGVER_Z_s, NULL, 0)
#define PROGVERXY_s PROGVER_X_s"."PROGVER_Y_s
#define PROGVER_s PROGVER_X_s"."PROGVER_Y_s"."PROGVER_Z_s
#define PROGNVR_s PROGNAM"-"PROGVER_s"-"PROGREL_s
#define PROGAUTHOR "Chris Evich <cevich@redhat.com>"

#endif /* _VERSION_H */
