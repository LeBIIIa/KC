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

    sc_nbwrite.h -- Interface between sc_signed/sc_unsigned and
    sc_signal_logic_vector/sc_signal_bool_vector.

    Original Author: Ali Dasdan. Synopsys, Inc. (dasdan@synopsys.com)

******************************************************************************/


/******************************************************************************

    MODIFICATION LOG - modifiers, enter your name, affliation and
    changes you are making here:

    Modifier Name & Affiliation:
    Description of Modification:
    

******************************************************************************/


#ifndef SC_NBWRITE_H
#define SC_NBWRITE_H

sc_signal_bool_vector&
sc_signal_bool_vector::write(const sc_unsigned& nv);

sc_signal_bool_vector&
sc_signal_bool_vector::write(const sc_signed& nv);

sc_signal_logic_vector&
sc_signal_logic_vector::write(const sc_unsigned& nv);

sc_signal_logic_vector&
sc_signal_logic_vector::write(const sc_signed& nv);

#endif
