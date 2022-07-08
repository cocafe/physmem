/* $Id$ */
/*
** Copyright (C) 2002 Martin Roesch <roesch@sourcefire.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef __GETOPT_H__
#define __GETOPT_H__

#ifdef __cplusplus
 extern "C" {
#endif
 
#define _next_char(string)  (char)(*(string+1))

extern char * optarg; 
extern int    optind; 

int getopt(int, char**, const char*);

#ifdef WIN32
#define _next_char_w(string)  (wchar_t)(*(string+1))

extern wchar_t * optarg_w; 
extern int       optind_w; 
extern int       optopt_w;

int getopt_w(int, wchar_t**, const wchar_t*);

#ifdef __cplusplus
};
#endif

#endif /*WIN32*/

#endif /*__GETOPT_H__*/
