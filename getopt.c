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


#include <stdio.h>                  /* for EOF */ 
#include <string.h>                 /* for strchr() */ 

#ifdef WIN32
#include <windows.h>                /* for TEXT */ 
#include <stdlib.h>
#endif


#include "getopt.h" 
 
/* static (global) variables that are specified as exported by getopt() */ 
char *optarg = NULL;    /* pointer to the start of the option argument  */ 
int   optind = 1;       /* number of the next argv[] to be evaluated    */ 
int   optopt = 0;       
int   opterr = 1;       /* non-zero if a question mark should be returned 
                           when a non-valid option character is detected */
int   optopt;

int getopt(int argc, char *argv[], const char *opstring) 
{ 
    static char *pIndexPosition = NULL; /* place inside current argv string */ 
    char *pArgString = NULL;        /* where to start from next */ 
    char *pOptString;               /* the string in our program */ 
 
 
    if (pIndexPosition != NULL) { 
        /* we last left off inside an argv string */ 
        if (*(++pIndexPosition)) { 
            /* there is more to come in the most recent argv */ 
            pArgString = pIndexPosition; 
        } 
    } 
 
    if (pArgString == NULL) { 
        /* we didn't leave off in the middle of an argv string */ 
        if (optind >= argc) { 
            /* more command-line arguments than the argument count */ 
            pIndexPosition = NULL;  /* not in the middle of anything */ 
            return EOF;             /* used up all command-line arguments */ 
        } 
 
        /*--------------------------------------------------------------------- 
         * If the next argv[] is not an option, there can be no more options. 
         *-------------------------------------------------------------------*/ 
        pArgString = argv[optind++]; /* set this to the next argument ptr */ 
 
        if (('/' != *pArgString) && /* doesn't start with a slash or a dash? */ 
            ('-' != *pArgString)) { 
            --optind;               /* point to current arg once we're done */ 
            optarg = NULL;          /* no argument follows the option */ 
            pIndexPosition = NULL;  /* not in the middle of anything */ 
            return EOF;             /* used up all the command-line flags */ 
        } 

        /* check for special end-of-flags markers */ 
        if ((strcmp(pArgString, "-") == 0) || 
            (strcmp(pArgString, "--") == 0)) { 
            optarg = NULL;          /* no argument follows the option */ 
            pIndexPosition = NULL;  /* not in the middle of anything */ 
            return EOF;             /* encountered the special flag */ 
        } 
 
        pArgString++;               /* look past the / or - */ 
    } 
 
    if (':' == *pArgString) {       /* is it a colon? */ 
        /*--------------------------------------------------------------------- 
         * Rare case: if opterr is non-zero, return a question mark; 
         * otherwise, just return the colon we're on. 
         *-------------------------------------------------------------------*/ 
        optopt = *pArgString;
        return (opterr ? (int)'?' : (int)':'); 
    } 
    else if ((pOptString = strchr(opstring, *pArgString)) == 0) { 
        /*--------------------------------------------------------------------- 
         * The letter on the command-line wasn't any good. 
         *-------------------------------------------------------------------*/ 
        optarg = NULL;              /* no argument follows the option */ 
        pIndexPosition = NULL;      /* not in the middle of anything */ 
        optopt = *pArgString;
        return (opterr ? (int)'?' : (int)*pArgString); 
    } 
    else { 
        /*--------------------------------------------------------------------- 
         * The letter on the command-line matches one we expect to see 
         *-------------------------------------------------------------------*/ 
        if (':' == _next_char(pOptString)) { /* is the next letter a colon? */ 
            /* It is a colon.  Look for an argument string. */ 
            if ('\0' != _next_char(pArgString)) {  /* argument in this argv? */ 
                optarg = &pArgString[1];   /* Yes, it is */ 
            } 
            else { 
                /*------------------------------------------------------------- 
                 * The argument string must be in the next argv. 
                 * But, what if there is none (bad input from the user)? 
                 * In that case, return the letter, and optarg as NULL. 
                 *-----------------------------------------------------------*/ 
                if (optind < argc) 
                    optarg = argv[optind++]; 
                else { 
                    optarg = NULL; 
                    optopt = *pArgString;
                    return (opterr ? (int)'?' : (int)*pArgString); 
                } 
            } 

            pIndexPosition = NULL;  /* not in the middle of anything */ 
        } 
        else { 
            /* it's not a colon, so just return the letter */ 
            optarg = NULL;          /* no argument follows the option */ 
            pIndexPosition = pArgString;    /* point to the letter we're on */ 
        } 
        return (int)*pArgString;    /* return the letter that matched */ 
    } 
}
#ifdef WIN32
/* static (global) variables that are specified as exported by getopt() */ 
wchar_t *optarg_w = NULL;    /* pointer to the start of the option argument  */ 
int   optind_w = 1;       /* number of the next argv[] to be evaluated    */ 
int   optopt_w = 0;

int getopt_w(int argc, wchar_t *argv[], const wchar_t *opstring) 
{ 
    static wchar_t *pIndexPosition = NULL; /* place inside current argv string */ 
    wchar_t *pArgString = NULL;        /* where to start from next */ 
    wchar_t *pOptString;               /* the string in our program */ 
 
 
    if (pIndexPosition != NULL) { 
        /* we last left off inside an argv string */ 
        if (*(++pIndexPosition)) { 
            /* there is more to come in the most recent argv */ 
            pArgString = pIndexPosition; 
        } 
    } 
 
    if (pArgString == NULL) { 
        /* we didn't leave off in the middle of an argv string */ 
        if (optind_w >= argc) { 
            /* more command-line arguments than the argument count */ 
            pIndexPosition = NULL;  /* not in the middle of anything */ 
            return EOF;             /* used up all command-line arguments */ 
        } 
 
        /*--------------------------------------------------------------------- 
         * If the next argv[] is not an option, there can be no more options. 
         *-------------------------------------------------------------------*/ 
        pArgString = argv[optind_w++]; /* set this to the next argument ptr */ 
 
        if (('/' != *pArgString) && /* doesn't start with a slash or a dash? */ 
            ('-' != *pArgString)) { 
            --optind_w;               /* point to current arg once we're done */ 
            optarg_w = NULL;          /* no argument follows the option */ 
            pIndexPosition = NULL;  /* not in the middle of anything */ 
            return EOF;             /* used up all the command-line flags */ 
        } 

        /* check for special end-of-flags markers */ 
        if ((wcscmp(pArgString, TEXT("-")) == 0) || 
            (wcscmp(pArgString, TEXT("--")) == 0)) { 
            optarg_w = NULL;          /* no argument follows the option */ 
            pIndexPosition = NULL;  /* not in the middle of anything */ 
            return EOF;             /* encountered the special flag */ 
        } 
 
        pArgString++;               /* look past the / or - */ 
    } 
 
    if (TEXT(':') == *pArgString) {       /* is it a colon? */ 
        /*--------------------------------------------------------------------- 
         * Rare case: if opterr is non-zero, return a question mark; 
         * otherwise, just return the colon we're on. 
         *-------------------------------------------------------------------*/ 
        optopt_w = *pArgString;
        return (opterr ? (int)TEXT('?') : (int)TEXT(':')); 
    } 
    else if ((pOptString = wcschr(opstring, *pArgString)) == 0) { 
        /*--------------------------------------------------------------------- 
         * The letter on the command-line wasn't any good. 
         *-------------------------------------------------------------------*/ 
        optarg_w = NULL;              /* no argument follows the option */ 
        pIndexPosition = NULL;      /* not in the middle of anything */ 
        optopt_w = *pArgString;
        return (opterr ? (int)TEXT('?') : (int)*pArgString); 
    } 
    else { 
        /*--------------------------------------------------------------------- 
         * The letter on the command-line matches one we expect to see 
         *-------------------------------------------------------------------*/ 
        if (TEXT(':') == _next_char(pOptString)) { /* is the next letter a colon? */ 
            /* It is a colon.  Look for an argument string. */ 
            if (TEXT('\0') != _next_char(pArgString)) {  /* argument in this argv? */ 
                optarg_w = &pArgString[1];   /* Yes, it is */ 
            } 
            else { 
                /*------------------------------------------------------------- 
                 * The argument string must be in the next argv. 
                 * But, what if there is none (bad input from the user)? 
                 * In that case, return the letter, and optarg_w as NULL. 
                 *-----------------------------------------------------------*/ 
                if (optind_w < argc) 
                    optarg_w = argv[optind_w++]; 
                else { 
                    optarg_w = NULL; 
                    optopt_w = *pArgString;
                    return (opterr ? (int)TEXT('?') : (int)*pArgString); 
                } 
            } 

            pIndexPosition = NULL;  /* not in the middle of anything */ 
        } 
        else { 
            /* it's not a colon, so just return the letter */ 
            optarg_w = NULL;          /* no argument follows the option */ 
            pIndexPosition = pArgString;    /* point to the letter we're on */ 
        } 
        return (int)*pArgString;    /* return the letter that matched */ 
    } 
}
#endif
