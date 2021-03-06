/******************************************************************************
    Copyright (c) 1996-2000 Synopsys, Inc.    ALL RIGHTS RESERVED

  The contents of this file are subject to the restrictions and limitations
  set forth in the SystemC(TM) Open Community License Software Download and
  Use License Version 1.1 (the "License"); you may not use this file except
  in compliance with such restrictions and limitations. You may obtain
  instructions on how to receive a copy of the License at
  http://www.systemc.org/. Software distributed by Original Contributor
  under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
  ANY KIND, either express or implied. See the License for the specific
  language governing rights and limitations under the License.

******************************************************************************/


/******************************************************************************

    sc_nbexterns.h -- External functions for both sc_signed and
    sc_unsigned classes. These functions work on two parameters u and
    v, and copy the result to the first parameter u. This is also the
    reason that they are suffixed with _on_help.
 
    The vec_* functions are called through either these functions or
    those in sc_nbfriends.cpp. The functions in sc_nbfriends.cpp
    perform their work on two inputs u and v, and return the result
    object.
 
    Original Author: Ali Dasdan. Synopsys, Inc. (dasdan@synopsys.com)
 
******************************************************************************/


/******************************************************************************

    MODIFICATION LOG - modifiers, enter your name, affliation and
    changes you are making here:

    Modifier Name & Affiliation:
    Description of Modification:
    

******************************************************************************/

#ifndef SC_NBEXTERNS_H
#define SC_NBEXTERNS_H

#include "sc_nbutils.h"

extern 
void add_on_help(small_type &us, 
                 length_type unb, length_type und, digit_type *ud, 
                 small_type vs, 
                 length_type vnb, length_type vnd, const digit_type *vd);

extern 
void mul_on_help_signed(small_type &us,
                        length_type unb, length_type und, digit_type *ud, 
                        length_type vnb, length_type vnd, const digit_type *vd);

void div_on_help_signed(small_type &us,
                        length_type unb, length_type und, digit_type *ud, 
                        length_type vnb, length_type vnd, const digit_type *vd);

extern 
void mod_on_help_signed(small_type &us,
                        length_type unb, length_type und, digit_type *ud, 
                        length_type vnb, length_type vnd, const digit_type *vd);

extern 
void mul_on_help_unsigned(small_type &us,
                          length_type unb, length_type und, digit_type *ud, 
                          length_type vnb, length_type vnd, const digit_type *vd);

void div_on_help_unsigned(small_type &us,
                          length_type unb, length_type und, digit_type *ud, 
                          length_type vnb, length_type vnd, const digit_type *vd);

extern 
void mod_on_help_unsigned(small_type &us,
                          length_type unb, length_type und, digit_type *ud, 
                          length_type vnb, length_type vnd, const digit_type *vd);

extern 
void and_on_help(small_type us, 
                 length_type unb, length_type und, digit_type *ud, 
                 small_type vs,
                 length_type vnb, length_type vnd, const digit_type *vd);

extern 
void or_on_help(small_type us, 
                length_type unb, length_type und, digit_type *ud, 
                small_type vs,
                length_type vnb, length_type vnd, const digit_type *vd);

extern 
void xor_on_help(small_type us, 
                 length_type unb, length_type und, digit_type *ud, 
                 small_type vs,
                 length_type vnb, length_type vnd, const digit_type *vd);

#endif
