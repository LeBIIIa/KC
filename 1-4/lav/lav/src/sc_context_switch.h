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

    sc_context_switch.h -- manage context switching.

    Original Author: Stan Y. Liao. Synopsys, Inc. (stanliao@synopsys.com)

******************************************************************************/

/******************************************************************************

    MODIFICATION LOG - modifiers, enter your name, affliation and
    changes you are making here:

    Modifier Name & Affiliation:
    Description of Modification:
    

******************************************************************************/

#ifndef SC_CONTEXT_SWITCH
#define SC_CONTEXT_SWITCH

#ifdef _MSC_VER
#include <winbase.h>
#endif

struct qt_t;
class sc_simcontext;

extern "C" {
    typedef void* (*AFT)(qt_t*,void*,void*);
}

#ifndef WIN32
extern void context_switch( AFT yieldhelper, void* data, void*, qt_t* qt );
#else
extern void context_switch( LPVOID data );
#endif

#endif
