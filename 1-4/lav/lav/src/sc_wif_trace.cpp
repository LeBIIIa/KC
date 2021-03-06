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

/*****************************************************************************

     sc_wif_trace.cpp - implementation of WIF tracing.

     Original Author - Abhijit Ghosh. Synopsys, Inc. (ghosh@synopsys.com)

******************************************************************************/

/******************************************************************************

    MODIFICATION LOG - modifiers, enter your name, affliation and
    changes you are making here:

    Modifier Name & Affiliation:
    Description of Modification:
    

******************************************************************************/


/*******************************************************************

   Acknowledgement: The tracing mechanism is based on the tracing
   mechanism developed at Infineon (formerly Siemens HL). Though this
   code is somewhat different, and significantly enhanced, the basics
   are identical to what was originally contributed by Infineon.  The
   contribution of Infineon in the development of this tracing
   technology is hereby acknowledged.

********************************************************************/

/********************************************************************

  Instead of creating the binary WIF format, we create the ASCII
  WIF format which can be converted to the binary format using
  a2wif (utility that comes with VSS from Synopsys). This way, 
  a user who does not have Synopsys VSS can still create WIF 
  files, but they can only be viewed by users who have VSS.

*********************************************************************/

#include <assert.h>
#include <time.h>
#include <stdlib.h>
#ifdef __BCPLUSPLUS__
#pragma hdrstop
#endif
#include "sc_string.h"
#include "sc_logic.h"
#include "sc_logic_vector.h"

#include "sc_bool_vector.h"
#include "sc_wif_trace.h"
#include "sc_ver.h"
#include "sc_simcontext.h"
#include "numeric_bit/numeric_bit.h"
#include "sc_resolved.h"
#include "sc_dump.h"

static bool running_regression = false;

// Forward declarations for functions that come later in the file
static char map_sc_logic_state_to_wif_state(char in_char);
static void wif_put_error_message(const char* msg, bool just_warning);


// Base class for the traces
class wif_trace {
public:
    wif_trace(const sc_string& _name, const sc_string& _wif_name);

    // Needs to be pure virtual as has to be defined by the particular
    // type being traced
    virtual void write(FILE* f) = 0;
    
    virtual void set_width();

    // Comparison function needs to be pure virtual too
    virtual bool changed() = 0;

    // Got to declare this virtual as this will be overwritten by one base class
    virtual void print_variable_declaration_line(FILE* f);

    virtual ~wif_trace();

    const sc_string name;     // Name of the variable
    const sc_string wif_name; // Name of the variable in WIF file
    const char* wif_type;     // WIF data type
    int bit_width; 
};


wif_trace::wif_trace(const sc_string& _name, const sc_string& _wif_name)
        : name(_name), wif_name(_wif_name), bit_width(0)
{
    /* Intentionally blank */
}
        
void wif_trace::print_variable_declaration_line(FILE* f)
{
    char buf[2000];

    if(bit_width == 0){
        sprintf(buf, "Traced object \"%s\" has 0 Bits, cannot be traced.", (const char *) name);
        wif_put_error_message(buf, false);
    }
    else{
        fprintf(f, "declare  %s   \"%s\"  %s  ",
                (const char *) wif_name, (const char *) name, wif_type);
	if (bit_width > 1)
	  fprintf(f, "0 %d ", bit_width-1);
	fprintf(f, "variable ;\n");
	fprintf(f, "start_trace %s ;\n", (const char *) wif_name);
    }
}

void wif_trace::set_width()
{
    /* Intentionally Blank, should be defined for each type separately */
}

wif_trace::~wif_trace()
{
    /* Intentionally Blank */
}

// Classes for tracing individual data types

/**********************************************************************************************/

class wif_bool_trace : public wif_trace {
public:
    wif_bool_trace(const bool& object, const sc_string& _name, const sc_string& _wif_name);
    void write(FILE* f);
    bool changed();

protected:    
    const bool& object;
    bool old_value;
};

wif_bool_trace::wif_bool_trace(const bool& _object, const sc_string& _name, const sc_string& _wif_name)
    : wif_trace(_name, _wif_name), object(_object)
{
    bit_width = 1;
    old_value = object;
    wif_type = "BIT";
}

bool wif_bool_trace::changed()
{
    return object != old_value;
}

void wif_bool_trace::write(FILE* f)
{
    if (object == true) fprintf(f, "assign %s \'1\' ;\n", (const char *) wif_name);
    else fprintf(f, "assign %s \'0\' ;\n", (const char *) wif_name);
    old_value = object;
}

/*******************************************************************************************/

class wif_sc_logic_trace: public wif_trace {
public:
    wif_sc_logic_trace(const sc_logic& object, const sc_string& _name, const sc_string& _wif_name);
    void write(FILE* f);
    bool changed();

protected:    
    const sc_logic& object;
    sc_logic old_value;
};


wif_sc_logic_trace::wif_sc_logic_trace(const sc_logic& _object, const sc_string& _name, const sc_string& _wif_name) 
  : wif_trace(_name, _wif_name), object(_object)
{
    bit_width = 1;
    old_value = object;
    wif_type = "MVL";
}


bool wif_sc_logic_trace::changed()
{
    return object != old_value;
}


void wif_sc_logic_trace::write(FILE* f)
{
    char wif_char;
    fprintf(f, "assign %s \'", (const char *) wif_name);
    wif_char = map_sc_logic_state_to_wif_state(object.to_char());
    fputc(wif_char, f); 
    fprintf(f,"\' ;\n");
    old_value = object;
}

/******************************************************************************************/

class wif_bool_vector_trace: public wif_trace {
public:
    wif_bool_vector_trace(const sc_bool_vector& object, const sc_string& _name, const sc_string& _wif_name);
    void write(FILE* f);
    bool changed();
    void set_width();

protected:    
    const sc_bool_vector& object;
    sc_bool_vector old_value;
};


wif_bool_vector_trace::wif_bool_vector_trace(const sc_bool_vector& _object, const sc_string& _name, const sc_string& _wif_name) 
  : wif_trace(_name, _wif_name), object(_object), old_value(_object.length())
{
    old_value = object;
    wif_type = "BIT";
}

bool wif_bool_vector_trace::changed()
{
    return object != old_value;
}

void wif_bool_vector_trace::write(FILE* f)
{
    char buf[1000], *buf_ptr = buf;

    int bitindex;
    for(bitindex = object.length() - 1; bitindex >= 0; --bitindex) {
        *buf_ptr++ = "01"[(object)[bitindex]];
    }
    *buf_ptr = '\0';
    fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf); 
    old_value = object;
}

void wif_bool_vector_trace::set_width()
{
    bit_width = object.length();
}


/********************************************************************************/

class wif_sc_logic_vector_trace: public wif_trace {
public:
    wif_sc_logic_vector_trace(const sc_logic_vector& object, const sc_string& _name, const sc_string& _wif_name);
    void write(FILE* f);
    bool changed();
    void set_width();

protected:    
    const sc_logic_vector& object;
    sc_logic_vector old_value;
};

wif_sc_logic_vector_trace::wif_sc_logic_vector_trace(const sc_logic_vector& _object, const sc_string& _name, const sc_string& _wif_name) 
  : wif_trace(_name, _wif_name), object(_object), old_value(_object.length())
{
    old_value = object;
    wif_type = "MVL";
}

bool wif_sc_logic_vector_trace::changed()
{
    return object != old_value;
}


void wif_sc_logic_vector_trace::write(FILE* f)
{
    char in_char;
    char out_char;
    char buf[1000], *buf_ptr = buf;

    int bitindex;
    for(bitindex = object.length() - 1; bitindex >= 0; --bitindex) {
        in_char = (object)[bitindex].to_char();
        out_char = map_sc_logic_state_to_wif_state(in_char);
        *buf_ptr++ = out_char; 
    }
    *buf_ptr = '\0';
    fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf); 
    old_value = object;
}


void wif_sc_logic_vector_trace::set_width()
{
    bit_width = object.length();
}

/******************************************************************************************/

class wif_sc_unsigned_trace: public wif_trace {
public:
    wif_sc_unsigned_trace(const sc_unsigned& object, const sc_string& _name, const sc_string& _wif_name);
    void write(FILE* f);
    bool changed();
    void set_width();

protected:    
    const sc_unsigned& object;
    sc_unsigned old_value;
};


wif_sc_unsigned_trace::wif_sc_unsigned_trace(const sc_unsigned& _object, const sc_string& _name, const sc_string& _wif_name) 
  : wif_trace(_name, _wif_name), object(_object), old_value(_object.length())
{
    old_value = object;
    wif_type = "BIT";
}

bool wif_sc_unsigned_trace::changed()
{
    return object != old_value;
}

void wif_sc_unsigned_trace::write(FILE* f)
{
    char buf[1000], *buf_ptr = buf;

    int bitindex;
    for(bitindex = object.length() - 1; bitindex >= 0; --bitindex) {
        *buf_ptr++ = "01"[(object)[bitindex]];
    }
    *buf_ptr = '\0';
    fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf); 
    old_value = object;
}

void wif_sc_unsigned_trace::set_width()
{
    bit_width = object.length();
}


/******************************************************************************************/

class wif_sc_signed_trace: public wif_trace {
public:
    wif_sc_signed_trace(const sc_signed& object, const sc_string& _name, const sc_string& _wif_name);
    void write(FILE* f);
    bool changed();
    void set_width();

protected:    
    const sc_signed& object;
    sc_signed old_value;
};


wif_sc_signed_trace::wif_sc_signed_trace(const sc_signed& _object, const sc_string& _name, const sc_string& _wif_name) 
  : wif_trace(_name, _wif_name), object(_object), old_value(_object.length())
{
    old_value = object;
    wif_type = "BIT";
}

bool wif_sc_signed_trace::changed()
{
    return object != old_value;
}

void wif_sc_signed_trace::write(FILE* f)
{
    char buf[1000], *buf_ptr = buf;

    int bitindex;
    for(bitindex = object.length() - 1; bitindex >= 0; --bitindex) {
        *buf_ptr++ = "01"[(object)[bitindex]];
    }
    *buf_ptr = '\0';

    fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf); 
    old_value = object;
}

void wif_sc_signed_trace::set_width()
{
    bit_width = object.length();
}

/******************************************************************************************/

class wif_sc_uint_base_trace: public wif_trace {
public:
    wif_sc_uint_base_trace(const sc_uint_base& object, const sc_string& _name, const sc_string& _wif_name);
    void write(FILE* f);
    bool changed();
    void set_width();

protected:    
    const sc_uint_base& object;
    sc_uint_base old_value;
};


wif_sc_uint_base_trace::wif_sc_uint_base_trace(const sc_uint_base& _object, const sc_string& _name, const sc_string& _wif_name) 
  : wif_trace(_name, _wif_name), object(_object), old_value(_object.width)
{
    old_value = object;
    wif_type = "BIT";
}

bool wif_sc_uint_base_trace::changed()
{
    return object != old_value;
}

void wif_sc_uint_base_trace::write(FILE* f)
{
    char buf[1000], *buf_ptr = buf;

    int bitindex;
    for(bitindex = object.width - 1; bitindex >= 0; --bitindex) {
        *buf_ptr++ = "01"[(object)[bitindex]];
    }
    *buf_ptr = '\0';
    fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf); 
    old_value = object;
}

void wif_sc_uint_base_trace::set_width()
{
    bit_width = object.width;
}


/******************************************************************************************/

class wif_sc_int_base_trace: public wif_trace {
public:
    wif_sc_int_base_trace(const sc_int_base& object, const sc_string& _name, const sc_string& _wif_name);
    void write(FILE* f);
    bool changed();
    void set_width();

protected:    
    const sc_int_base& object;
    sc_int_base old_value;
};


wif_sc_int_base_trace::wif_sc_int_base_trace(const sc_int_base& _object, const sc_string& _name, const sc_string& _wif_name) 
  : wif_trace(_name, _wif_name), object(_object), old_value(_object.width)
{
    old_value = object;
    wif_type = "BIT";
}

bool wif_sc_int_base_trace::changed()
{
    return object != old_value;
}

void wif_sc_int_base_trace::write(FILE* f)
{
    char buf[1000], *buf_ptr = buf;

    int bitindex;
    for(bitindex = object.width - 1; bitindex >= 0; --bitindex) {
        *buf_ptr++ = "01"[(object)[bitindex]];
    }
    *buf_ptr = '\0';

    fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf); 
    old_value = object;
}

void wif_sc_int_base_trace::set_width()
{
    bit_width = object.width;
}


#ifdef SC_INCLUDE_FX

/*****************************************************************************/

class wif_sc_fxval_trace: public wif_trace
{
public:

    wif_sc_fxval_trace( const sc_fxval& object,
			const sc_string& _name,
			const sc_string& _wif_name );
    void write( FILE* f );
    bool changed();

protected:

    const sc_fxval& object;
    sc_fxval old_value;

};

wif_sc_fxval_trace::wif_sc_fxval_trace( const sc_fxval& _object,
				        const sc_string& _name,
					const sc_string& _wif_name )
: wif_trace( _name, _wif_name ),
  object( _object )
{
    bit_width = 1;
    old_value = object;
    wif_type = "real";
}

bool
wif_sc_fxval_trace::changed()
{
    return object != old_value;
}

void
wif_sc_fxval_trace::write( FILE* f )
{
    fprintf( f, "assign  %s %f ; \n", (const char *) wif_name,
	     object.to_double() );
    old_value = object;
}

/*****************************************************************************/

class wif_sc_fxval_fast_trace: public wif_trace
{
public:

    wif_sc_fxval_fast_trace( const sc_fxval_fast& object,
			     const sc_string& _name,
			     const sc_string& _wif_name );
    void write( FILE* f );
    bool changed();

protected:

    const sc_fxval_fast& object;
    sc_fxval_fast old_value;

};

wif_sc_fxval_fast_trace::wif_sc_fxval_fast_trace( const sc_fxval_fast& _object,
						  const sc_string& _name,
						  const sc_string& _wif_name )
: wif_trace( _name, _wif_name ),
  object( _object )
{
    bit_width = 1;
    old_value = object;
    wif_type = "real";
}

bool
wif_sc_fxval_fast_trace::changed()
{
    return object != old_value;
}

void
wif_sc_fxval_fast_trace::write( FILE* f )
{
    fprintf( f, "assign  %s %f ; \n", (const char *) wif_name,
	     object.to_double() );
    old_value = object;
}

/*****************************************************************************/

class wif_sc_fxnum_trace: public wif_trace
{
public:

    wif_sc_fxnum_trace( const sc_fxnum& object,
			const sc_string& _name,
			const sc_string& _wif_name );
    void write( FILE* f );
    bool changed();
    void set_width();

protected:

    const sc_fxnum& object;
    sc_fxnum old_value;

};

wif_sc_fxnum_trace::wif_sc_fxnum_trace( const sc_fxnum& _object,
				        const sc_string& _name,
					const sc_string& _wif_name )
: wif_trace( _name, _wif_name ),
  object( _object ),
  old_value( _object._params.type_params(),
	     _object._params.enc(),
	     _object._params.cast_switch(),
	     0 )
{
    old_value = object;
    wif_type = "BIT";
}

bool
wif_sc_fxnum_trace::changed()
{
    return object != old_value;
}

void
wif_sc_fxnum_trace::write( FILE* f )
{
    char buf[1000], *buf_ptr = buf;

    int bitindex;
    for( bitindex = object.wl() - 1; bitindex >= 0; -- bitindex )
    {
        *buf_ptr ++ = "01"[(object)[bitindex]];
    }
    *buf_ptr = '\0';

    fprintf( f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf );
    old_value = object;
}

void
wif_sc_fxnum_trace::set_width()
{
    bit_width = object.wl();
}

/*****************************************************************************/

class wif_sc_fxnum_fast_trace: public wif_trace
{
public:

    wif_sc_fxnum_fast_trace( const sc_fxnum_fast& object,
			     const sc_string& _name,
			     const sc_string& _wif_name );
    void write( FILE* f );
    bool changed();
    void set_width();

protected:

    const sc_fxnum_fast& object;
    sc_fxnum_fast old_value;

};

wif_sc_fxnum_fast_trace::wif_sc_fxnum_fast_trace( const sc_fxnum_fast& _object,
						  const sc_string& _name,
						  const sc_string& _wif_name )
: wif_trace( _name, _wif_name ),
  object( _object ),
  old_value( _object._params.type_params(),
	     _object._params.enc(),
	     _object._params.cast_switch(),
	     0 )
{
    old_value = object;
    wif_type = "BIT";
}

bool
wif_sc_fxnum_fast_trace::changed()
{
    return object != old_value;
}

void
wif_sc_fxnum_fast_trace::write( FILE* f )
{
    char buf[1000], *buf_ptr = buf;

    int bitindex;
    for( bitindex = object.wl() - 1; bitindex >= 0; -- bitindex )
    {
        *buf_ptr ++ = "01"[(object)[bitindex]];
    }
    *buf_ptr = '\0';

    fprintf( f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf );
    old_value = object;
}

void
wif_sc_fxnum_fast_trace::set_width()
{
    bit_width = object.wl();
}

#endif


/**************************************************************************************/

class wif_unsigned_int_trace: public wif_trace {
public:
    wif_unsigned_int_trace(const unsigned& object, const sc_string& _name, const sc_string& _wif_name, int _width);
    void write(FILE* f);
    bool changed();

protected:
    const unsigned& object;
    unsigned old_value;
    unsigned mask; 
};


wif_unsigned_int_trace::wif_unsigned_int_trace(const unsigned& _object, const sc_string& _name, const sc_string& _wif_name, int _width) 
  : wif_trace(_name, _wif_name), object(_object)
{
    bit_width = _width;
    if (bit_width < 32) {
        mask = ~(-1 << bit_width);
    } else {
        mask = 0xffffffff;
    }

    old_value = object;
    wif_type = "BIT";
}


bool wif_unsigned_int_trace::changed()
{
    return object != old_value;
}


void wif_unsigned_int_trace::write(FILE* f)
{
    char buf[1000];
    int bitindex;

    // Check for overflow
    if ((object & mask) != object) {
        for (bitindex = 0; bitindex < bit_width; bitindex++){
            buf[bitindex] = '0';
        }
    }
    else{
        unsigned bit_mask = 1 << (bit_width-1);
        for (bitindex = 0; bitindex < bit_width; bitindex++) {
            buf[bitindex] = (object & bit_mask)? '1' : '0';
            bit_mask = bit_mask >> 1;
        }
    }
    buf[bitindex] = '\0';
    fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf); 
    old_value = object;
}


/**************************************************************************************/

class wif_unsigned_short_trace: public wif_trace {
public:
    wif_unsigned_short_trace(const unsigned short& object, const sc_string& _name, const sc_string& _wif_name, int _width);
    void write(FILE* f);
    bool changed();

protected:
    const unsigned short& object;
    unsigned short old_value;
    unsigned short mask; 
};


wif_unsigned_short_trace::wif_unsigned_short_trace(const unsigned short& _object, const sc_string& _name, const sc_string& _wif_name, int _width) 
  : wif_trace(_name, _wif_name), object(_object)
{
    bit_width = _width;
    if (bit_width < 16) {
        mask = ~(-1 << bit_width);
    } else {
        mask = 0xffff;
    }

    old_value = object;
    wif_type = "BIT";
}


bool wif_unsigned_short_trace::changed()
{
    return object != old_value;
}


void wif_unsigned_short_trace::write(FILE* f)
{
    char buf[1000];
    int bitindex;

    // Check for overflow
    if ((object & mask) != object) {
        for (bitindex = 0; bitindex < bit_width; bitindex++){
            buf[bitindex]='0';
        }
    }
    else{
        unsigned bit_mask = 1 << (bit_width-1);
        for (bitindex = 0; bitindex < bit_width; bitindex++) {
            buf[bitindex] = (object & bit_mask)? '1' : '0';
            bit_mask = bit_mask >> 1;
        }
    }
    buf[bitindex] = '\0';
    fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf); 
    old_value = object;
}

/**************************************************************************************/

class wif_unsigned_char_trace: public wif_trace {
public:
    wif_unsigned_char_trace(const unsigned char& object, const sc_string& _name, const sc_string& _wif_name, int _width);
    void write(FILE* f);
    bool changed();

protected:
    const unsigned char& object;
    unsigned char old_value;
    unsigned char mask; 
};


wif_unsigned_char_trace::wif_unsigned_char_trace(const unsigned char& _object, const sc_string& _name, const sc_string& _wif_name, int _width) 
  : wif_trace(_name, _wif_name), object(_object)
{
    bit_width = _width;
    if (bit_width < 8) {
        mask = ~(-1 << bit_width);
    } else {
        mask = 0xff;
    }

    old_value = object;
    wif_type = "BIT";
}


bool wif_unsigned_char_trace::changed()
{
    return object != old_value;
}


void wif_unsigned_char_trace::write(FILE* f)
{
    char buf[1000];
    int bitindex;

    // Check for overflow
    if ((object & mask) != object) {
        for (bitindex = 0; bitindex < bit_width; bitindex++){
            buf[bitindex]='0';
        }
    }
    else{
        unsigned bit_mask = 1 << (bit_width-1);
        for (bitindex = 0; bitindex < bit_width; bitindex++) {
            buf[bitindex] = (object & bit_mask)? '1' : '0';
            bit_mask = bit_mask >> 1;
        }
    }
    buf[bitindex] = '\0';
    fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf); 
    old_value = object;
}

/**************************************************************************************/

class wif_unsigned_long_trace: public wif_trace {
public:
    wif_unsigned_long_trace(const unsigned long& object, const sc_string& _name, const sc_string& _wif_name, int _width);
    void write(FILE* f);
    bool changed();

protected:
    const unsigned long& object;
    unsigned long old_value;
    unsigned long mask; 
};


wif_unsigned_long_trace::wif_unsigned_long_trace(const unsigned long& _object, const sc_string& _name, const sc_string& _wif_name, int _width) 
  : wif_trace(_name, _wif_name), object(_object)
{
    bit_width = _width;
    if (bit_width < 32) {
        mask = ~(-1 << bit_width);
    } else {
        mask = 0xffffffff;
    }

    old_value = object;
    wif_type = "BIT";
}


bool wif_unsigned_long_trace::changed()
{
    return object != old_value;
}


void wif_unsigned_long_trace::write(FILE* f)
{
    char buf[1000];
    int bitindex;

    // Check for overflow
    if ((object & mask) != object) {
        for (bitindex = 0; bitindex < bit_width; bitindex++){
            buf[bitindex]='0';
        }
    }
    else{
        unsigned bit_mask = 1 << (bit_width-1);
        for (bitindex = 0; bitindex < bit_width; bitindex++) {
            buf[bitindex] = (object & bit_mask)? '1' : '0';
            bit_mask = bit_mask >> 1;
        }
    }
    buf[bitindex] = '\0';
    fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf); 
    old_value = object;
}

/**************************************************************************************/

class wif_signed_int_trace: public wif_trace {
public:
    wif_signed_int_trace(const int& object, const sc_string& _name, const sc_string& _wif_name, int _width);
    void write(FILE* f);
    bool changed();

protected:
    const int& object;
    int old_value;
    unsigned mask; 
};


wif_signed_int_trace::wif_signed_int_trace(const signed& _object, const sc_string& _name, const sc_string& _wif_name, int _width) 
  : wif_trace(_name, _wif_name), object(_object)
{
    bit_width = _width;
    if (bit_width < 32) {
        mask = ~(-1 << bit_width);
    } else {
        mask = 0xffffffff;
    }

    old_value = object;
    wif_type = "BIT";
}


bool wif_signed_int_trace::changed()
{
    return object != old_value;
}


void wif_signed_int_trace::write(FILE* f)
{
    char buf[1000];
    int bitindex;

    // Check for overflow
    if (((unsigned) object & mask) != (unsigned) object) {
        for (bitindex = 0; bitindex < bit_width; bitindex++){
            buf[bitindex]='0';
        }
    }
    else{
        unsigned bit_mask = 1 << (bit_width-1);
        for (bitindex = 0; bitindex < bit_width; bitindex++) {
            buf[bitindex] = (object & bit_mask)? '1' : '0';
            bit_mask = bit_mask >> 1;
        }
    }
    buf[bitindex] = '\0';
    fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf); 
    old_value = object;
}

/**************************************************************************************/

class wif_signed_short_trace: public wif_trace {
public:
    wif_signed_short_trace(const short& object, const sc_string& _name, const sc_string& _wif_name, int _width);
    void write(FILE* f);
    bool changed();

protected:
    const short& object;
    short old_value;
    unsigned short mask; 
};


wif_signed_short_trace::wif_signed_short_trace(const short& _object, const sc_string& _name, const sc_string& _wif_name, int _width) 
  : wif_trace(_name, _wif_name), object(_object)
{
    bit_width = _width;
    if (bit_width < 16) {
        mask = ~(-1 << bit_width);
    } else {
        mask = 0xffff;
    }

    old_value = object;
    wif_type = "BIT";
}


bool wif_signed_short_trace::changed()
{
    return object != old_value;
}


void wif_signed_short_trace::write(FILE* f)
{
    char buf[1000];
    int bitindex;

    // Check for overflow
    if (((unsigned short) object & mask) != (unsigned short) object) {
        for (bitindex = 0; bitindex < bit_width; bitindex++){
            buf[bitindex]='0';
        }
    }
    else{
        unsigned bit_mask = 1 << (bit_width-1);
        for (bitindex = 0; bitindex < bit_width; bitindex++) {
            buf[bitindex] = (object & bit_mask)? '1' : '0';
            bit_mask = bit_mask >> 1;
        }
    }
    buf[bitindex] = '\0';
    fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf); 
    old_value = object;
}

/**************************************************************************************/

class wif_signed_char_trace: public wif_trace {
public:
    wif_signed_char_trace(const char& object, const sc_string& _name, const sc_string& _wif_name, int _width);
    void write(FILE* f);
    bool changed();

protected:
    const char& object;
    char old_value;
    unsigned char mask; 
};


wif_signed_char_trace::wif_signed_char_trace(const char& _object, const sc_string& _name, const sc_string& _wif_name, int _width) 
  : wif_trace(_name, _wif_name), object(_object)
{
    bit_width = _width;
    if (bit_width < 8) {
        mask = ~(-1 << bit_width);
    } else {
        mask = 0xff;
    }

    old_value = object;
    wif_type = "BIT";
}


bool wif_signed_char_trace::changed()
{
    return object != old_value;
}


void wif_signed_char_trace::write(FILE* f)
{
    char buf[1000];
    int bitindex;

    // Check for overflow
    if (((unsigned char) object & mask) != (unsigned char) object) {
        for (bitindex = 0; bitindex < bit_width; bitindex++){
            buf[bitindex]='0';
        }
    }
    else{
        unsigned bit_mask = 1 << (bit_width-1);
        for (bitindex = 0; bitindex < bit_width; bitindex++) {
            buf[bitindex] = (object & bit_mask)? '1' : '0';
            bit_mask = bit_mask >> 1;
        }
    }
    buf[bitindex] = '\0';
    fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf); 
    old_value = object;
}

/**************************************************************************************/

class wif_signed_long_trace: public wif_trace {
public:
    wif_signed_long_trace(const long& object, const sc_string& _name, const sc_string& _wif_name, int _width);
    void write(FILE* f);
    bool changed();

protected:
    const long& object;
    long old_value;
    unsigned long mask; 
};


wif_signed_long_trace::wif_signed_long_trace(const long& _object, const sc_string& _name, const sc_string& _wif_name, int _width) 
  : wif_trace(_name, _wif_name), object(_object)
{
    bit_width = _width;
    if (bit_width < 32) {
        mask = ~(-1 << bit_width);
    } else {
        mask = 0xffffffff;
    }

    old_value = object;
    wif_type = "BIT";
}


bool wif_signed_long_trace::changed()
{
    return object != old_value;
}


void wif_signed_long_trace::write(FILE* f)
{
    char buf[1000];
    int bitindex;

    // Check for overflow
    if (((unsigned long) object & mask) != (unsigned long) object) {
        for (bitindex = 0; bitindex < bit_width; bitindex++){
            buf[bitindex]='0';
        }
    }
    else{
        unsigned bit_mask = 1 << (bit_width-1);
        for (bitindex = 0; bitindex < bit_width; bitindex++) {
            buf[bitindex] = (object & bit_mask)? '1' : '0';
            bit_mask = bit_mask >> 1;
        }
    }
    buf[bitindex] = '\0';
    fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, buf); 
    old_value = object;
}


/**********************************************************************************************/

class wif_float_trace: public wif_trace {
public:
    wif_float_trace(const float& object, const sc_string& _name, const sc_string& _wif_name);
    void write(FILE* f);
    bool changed();

protected:    
    const float& object;
    float old_value;
};

wif_float_trace::wif_float_trace(const float& _object, const sc_string& _name, const sc_string& _wif_name)
  : wif_trace(_name, _wif_name), object(_object)
{
    bit_width = 1;
    old_value = object;
    wif_type = "real";
}

bool wif_float_trace::changed()
{
    return object != old_value;
}

void wif_float_trace::write(FILE* f)
{
    fprintf(f,"assign  %s %f ; \n", (const char *) wif_name, object);
    old_value = object;
}

/**********************************************************************************************/

class wif_double_trace: public wif_trace {
public:
    wif_double_trace(const double& object, const sc_string& _name, const sc_string& _wif_name);
    void write(FILE* f);
    bool changed();

protected:    
    const double& object;
    double old_value;
};

wif_double_trace::wif_double_trace(const double& _object, const sc_string& _name, const sc_string& _wif_name)
  : wif_trace(_name, _wif_name), object(_object)
{
    bit_width = 1;
    old_value = object;
    wif_type = "real";
}

bool wif_double_trace::changed()
{
    return object != old_value;
}

void wif_double_trace::write(FILE* f)
{
    fprintf(f,"assign  %s %f ; \n", (const char *) wif_name, object);
    old_value = object;
}


/********************************************************************************************/

class wif_enum_trace : public wif_trace {
public:
    wif_enum_trace(const unsigned& _object, const sc_string& _name, const sc_string& _wif_name, const char** enum_literals);
    void write(FILE* f);
    bool changed();
    // Hides the definition of the same (virtual) function in wif_trace
    void print_variable_declaration_line(FILE* f);

protected:
    const unsigned& object;
    unsigned old_value;
    
    const char** literals;
    unsigned nliterals;
    sc_string type_name;

    ~wif_enum_trace();
};


wif_enum_trace::wif_enum_trace(const unsigned& _object, const sc_string& _name, const sc_string& _wif_name, const char** _enum_literals) 
  : wif_trace(_name, _wif_name), object(_object), literals(_enum_literals)
{
    // find number of enumeration literals - counting loop
    for (nliterals = 0; _enum_literals[nliterals]; nliterals++);

    bit_width = 1;
    old_value = object;
    type_name = _name + "__type__";
    wif_type = (const char *) type_name;
}       

void wif_enum_trace::print_variable_declaration_line(FILE* f)
{
    fprintf(f, "type scalar \"%s\" enum ", wif_type);

    for (unsigned i = 0; i < nliterals; i++)
      fprintf(f, "\"%s\", ", literals[i]);
    fprintf(f, "\"SC_WIF_UNDEF\" ;\n");

    fprintf(f, "declare  %s   \"%s\"  \"%s\" ",
	    (const char *) wif_name, (const char *) name, wif_type);
    fprintf(f, "variable ;\n");
    fprintf(f, "start_trace %s ;\n", (const char *) wif_name);
}

bool wif_enum_trace::changed()
{
    return object != old_value;
}

void wif_enum_trace::write(FILE* f)
{
    char buf[2000];
    static bool warning_issued = false;

    if (object >= nliterals) { // Note unsigned value is always greater than 0
        if (!warning_issued) {
	    sprintf(buf, "Tracing Error: Value of enumerated type undefined");
	    wif_put_error_message(buf, false);
	    warning_issued = true;
	}
	fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, "SC_WIF_UNDEF");
    }
    else 
        fprintf(f, "assign %s \"%s\" ;\n", (const char *) wif_name, literals[object]);
    old_value = object;
}

wif_enum_trace::~wif_enum_trace()
{
    /* Intentionally blank */
}


/**************************************************************************************************
           wif_trace_file functions
***************************************************************************************************/


wif_trace_file::wif_trace_file(const char * name)
{
    sc_string file_name = name ;
    file_name += ".awif";
    fp = fopen((const char *) file_name, "w");
    if (!fp) {
        sc_string msg = sc_string("Cannot write trace file '") + file_name + "'";
        fprintf(stderr, "FATAL: %s\n", (const char *) msg);
        exit(1);
    }
    trace_delta_cycles = false; // make it the default
    initialized = false;
    wif_name_index = 0;

    //default timestep = secs: 
    timescale_unit = 1; 
    timescale_set_by_user = false;
}


void wif_trace_file::initialize()
{
    char buf[2000];

    // init
    fprintf(fp, "init ;\n");

    //timescale:
    if     (timescale_unit == 1e-15) sprintf(buf,"0");
    else if(timescale_unit == 1e-14) sprintf(buf,"1");
    else if(timescale_unit == 1e-13) sprintf(buf,"2");
    else if(timescale_unit == 1e-12) sprintf(buf,"3");
    else if(timescale_unit == 1e-11) sprintf(buf,"4");
    else if(timescale_unit == 1e-10) sprintf(buf,"5");
    else if(timescale_unit == 1e-9)  sprintf(buf,"6");
    else if(timescale_unit == 1e-8)  sprintf(buf,"7");
    else if(timescale_unit == 1e-7)  sprintf(buf,"8");
    else if(timescale_unit == 1e-6)  sprintf(buf,"9");
    else if(timescale_unit == 1e-5)  sprintf(buf,"10");
    else if(timescale_unit == 1e-4)  sprintf(buf,"11");
    else if(timescale_unit == 1e-3)  sprintf(buf,"12");
    else if(timescale_unit == 1e-2)  sprintf(buf,"13");
    else if(timescale_unit == 1e-1)  sprintf(buf,"14");
    else if(timescale_unit == 1e0)   sprintf(buf,"15");
    else if(timescale_unit == 1e1)   sprintf(buf,"16");
    else if(timescale_unit == 1e2)   sprintf(buf,"17");
    fprintf(fp,"header  %s \"%s\" ;\n\n", buf, sc_version());

    //date:
    time_t long_time;
    time(&long_time);
    struct tm* p_tm;
    p_tm = localtime(&long_time);
    strftime(buf, 199, "%b %d, %Y       %H:%M:%S", p_tm);
    fprintf(fp, "comment \"ASCII WIF file produced on date:  %s\" ;\n", buf);
 
    //version:
    fprintf(fp, "comment \"Created by %s\" ;\n", sc_version());
    //conversion info
    fprintf(fp, "comment \"Convert this file to binary WIF format using a2wif\" ;\n\n");


    running_regression = (getenv("SCENIC_REGRESSION") == NULL);
    if(timescale_set_by_user == false && running_regression){
        // Don't produce this message if running regression
        fprintf(stderr,"WARNING: Default time step (1 s) is used for WIF tracing.\n");
    }

    // Define the two types we need to represent bool and sc_logic
    fprintf(fp, "type scalar \"BIT\" enum '0', '1' ;\n");
    fprintf(fp, "type scalar \"MVL\" enum '0', '1', 'X', 'Z', '?' ;\n");
    fprintf(fp, "\n");

    //variable definitions:
    int i;
    for (i = 0; i < traces.size(); i++) {
        wif_trace* t = traces[i];
        t->set_width(); //needed for all vectors
        t->print_variable_declaration_line(fp);
    }

    double inittime = sc_simulation_time();
    previous_time = inittime/timescale_unit;

    // Dump all values at initial time
    sprintf(buf,
            "All initial values are dumped below at time "
            "%g sec = %g timescale units.",
            inittime,
            inittime/timescale_unit
            );
    write_comment(buf);

    double_to_special_int64(inittime/timescale_unit, &previous_time_units_high, &previous_time_units_low );

    for (i = 0; i < traces.size(); i++) {
        wif_trace* t = traces[i];
        t->write(fp);
    }
}


void wif_trace_file::sc_set_wif_time_unit(int exponent10_seconds)
{
    if(initialized){
        wif_put_error_message("WIF trace timescale unit cannot be changed once tracing has begun.\n"
                              "To change the scale, create a new trace file.",
                              false);
        return;
    }

    if(exponent10_seconds < -15 || exponent10_seconds >  2){
        wif_put_error_message("set_wif_time_unit() has valid exponent range -15...+2.", false);
        return;
    }

    if     (exponent10_seconds == -15) timescale_unit = 1e-15;
    else if(exponent10_seconds == -14) timescale_unit = 1e-14;
    else if(exponent10_seconds == -13) timescale_unit = 1e-13;
    else if(exponent10_seconds == -12) timescale_unit = 1e-12;
    else if(exponent10_seconds == -11) timescale_unit = 1e-11;
    else if(exponent10_seconds == -10) timescale_unit = 1e-10;
    else if(exponent10_seconds ==  -9) timescale_unit = 1e-9;
    else if(exponent10_seconds ==  -8) timescale_unit = 1e-8;
    else if(exponent10_seconds ==  -7) timescale_unit = 1e-7;
    else if(exponent10_seconds ==  -6) timescale_unit = 1e-6;
    else if(exponent10_seconds ==  -5) timescale_unit = 1e-5;
    else if(exponent10_seconds ==  -4) timescale_unit = 1e-4;
    else if(exponent10_seconds ==  -3) timescale_unit = 1e-3;
    else if(exponent10_seconds ==  -2) timescale_unit = 1e-2;
    else if(exponent10_seconds ==  -1) timescale_unit = 1e-1;
    else if(exponent10_seconds ==   0) timescale_unit = 1e0;
    else if(exponent10_seconds ==   1) timescale_unit = 1e1;
    else if(exponent10_seconds ==   2) timescale_unit = 1e2;

    char buf[200];
    sprintf(buf, "Note: WIF trace timescale unit is set by user to 1e%d sec.\n", exponent10_seconds);
    fputs(buf,stderr);
    timescale_set_by_user = true;
}

void wif_trace_file::trace(const bool& object, const sc_string& name)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);


    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_bool_trace(object, name, temp_wif_name));
}

void wif_trace_file::trace(const sc_bool_vector& object, const sc_string& name)
{
    if (initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_bool_vector_trace(object, name, temp_wif_name));
}


void wif_trace_file::trace(const sc_logic& object, const sc_string& name)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_sc_logic_trace(object, name, temp_wif_name));
}

void wif_trace_file::trace(const sc_logic_vector& object, const sc_string& name)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_sc_logic_vector_trace(object,name, temp_wif_name));
}

void wif_trace_file::trace(const unsigned& object, const sc_string& name, int _width)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_unsigned_int_trace(object, name, temp_wif_name, _width));
}

void wif_trace_file::trace(const unsigned char& object, const sc_string& name, int _width)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_unsigned_char_trace(object, name, temp_wif_name, _width));
}

void wif_trace_file::trace(const unsigned short& object, const sc_string& name, int _width)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_unsigned_short_trace(object, name, temp_wif_name, _width));
}

void wif_trace_file::trace(const unsigned long& object, const sc_string& name, int _width)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_unsigned_long_trace(object, name, temp_wif_name, _width));
}

void wif_trace_file::trace(const int& object, const sc_string& name, int _width)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_signed_int_trace(object, name, temp_wif_name, _width));
}

void wif_trace_file::trace(const char& object, const sc_string& name, int _width)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_signed_char_trace(object, name, temp_wif_name, _width));
}

void wif_trace_file::trace(const short& object, const sc_string& name, int _width)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_signed_short_trace(object, name, temp_wif_name, _width));
}

void wif_trace_file::trace(const long& object, const sc_string& name, int _width)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_signed_long_trace(object, name, temp_wif_name, _width));
}

void wif_trace_file::trace(const float& object, const sc_string& name)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_float_trace(object, name, temp_wif_name));
}

void wif_trace_file::trace(const double& object, const sc_string& name)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_double_trace(object, name, temp_wif_name));
}

void wif_trace_file::trace(const sc_unsigned& object, const sc_string& name)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_sc_unsigned_trace(object, name, temp_wif_name));
}

void wif_trace_file::trace(const sc_signed& object, const sc_string& name)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_sc_signed_trace(object, name, temp_wif_name));
}

void wif_trace_file::trace(const sc_int_base& object, const sc_string& name)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_sc_int_base_trace(object, name, temp_wif_name));
}

void wif_trace_file::trace(const sc_uint_base& object, const sc_string& name)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_sc_uint_base_trace(object, name, temp_wif_name));
}


#ifdef SC_INCLUDE_FX

#define DEFN_TRACE_METHOD(tp)                                                 \
void                                                                          \
wif_trace_file::trace( const tp& object, const sc_string& name )              \
{                                                                             \
    if( initialized )                                                         \
        wif_put_error_message(                                                \
	    "No traces can be added once simulation has started.\n"           \
            "To add traces, create a new wif trace file.", false );           \
                                                                              \
    sc_string temp_wif_name;                                                  \
    create_wif_name( &temp_wif_name );                                        \
    traces.push_back( new wif_ ## tp ## _trace( object,                       \
						name,                         \
						temp_wif_name ) );            \
}

DEFN_TRACE_METHOD(sc_fxval)
DEFN_TRACE_METHOD(sc_fxval_fast)
DEFN_TRACE_METHOD(sc_fxnum)
DEFN_TRACE_METHOD(sc_fxnum_fast)

#undef DEFN_TRACE_METHOD

#endif


void wif_trace_file::trace(const unsigned& object, const sc_string& name, const char** enum_literals)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_enum_trace(object, name, temp_wif_name, enum_literals));
}

void wif_trace_file::trace(const sc_signal_bool_vector& object, const sc_string& name)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_bool_vector_trace((const sc_bool_vector&) object, name, temp_wif_name));
}

void wif_trace_file::trace(const sc_signal_logic_vector& object, const sc_string& name)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_sc_logic_vector_trace((const sc_logic_vector&) object, name, temp_wif_name));
}

void wif_trace_file::trace(const sc_signal_resolved& object, const sc_string& name)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_sc_logic_trace((const sc_logic&) *(object.get_data_ptr()), name, temp_wif_name));
}

void wif_trace_file::trace(const sc_signal_resolved_vector& object, const sc_string& name)
{
    if(initialized)  
        wif_put_error_message("No traces can be added once simulation has started.\n"
                              "To add traces, create a new wif trace file.", false);

    sc_string temp_wif_name;
    create_wif_name(&temp_wif_name);
    traces.push_back(new wif_sc_logic_vector_trace((const sc_logic_vector&) object, name, temp_wif_name));
}

void wif_trace_file::write_comment(const sc_string& comment)
{
    //no newline in comments allowed
    fprintf(fp, "comment \"%s\" ;\n", (const char *) comment);
}


void wif_trace_file::delta_cycles(bool flag)
{
    trace_delta_cycles = flag;
}

void wif_trace_file::cycle(bool this_is_a_delta_cycle)
{
    unsigned now_units_high, now_units_low;

    // Trace delta cycles only when enabled
    if (!trace_delta_cycles && this_is_a_delta_cycle) return;

    // Check for initialization
    if (!initialized) {
        initialize();
        initialized = true;
        return;
    };

    double now_units = sc_simulation_time() / timescale_unit;
    double_to_special_int64(now_units, &now_units_high, &now_units_low );

    // Now do the real stuff
    unsigned delta_units_high, delta_units_low;
    double diff_time;
    diff_time = now_units - previous_time;
    double_to_special_int64(diff_time, &delta_units_high, &delta_units_low);
    if (this_is_a_delta_cycle && (diff_time == 0.0)) delta_units_low++; // Increment time for delta cycle simulation
    // Note that in the last statement above, we are assuming no more than 2^32 delta cycles - seems realistic
    
    bool time_printed = false;
    wif_trace* const* const l_traces = traces.raw_data();
    for (int i = 0; i < traces.size(); i++) {
        wif_trace* t = l_traces[i];
        if(t->changed()){
            if(time_printed == false){
                if(delta_units_high){
                    fprintf(fp, "delta_time %u%09u ;\n", delta_units_high, delta_units_low);
                }
                else{ 
                    fprintf(fp, "delta_time %u ;\n", delta_units_low);
                }
                time_printed = true;
            }

	    // Write the variable
            t->write(fp);
        }
    }

    if(time_printed) {
        fprintf(fp, "\n");     // Put another newline
	// We update previous_time_units only when we print time because
	// this field stores the previous time that was printed, not the
	// previous time this function was called
	previous_time_units_high = now_units_high;
	previous_time_units_low = now_units_low;
	previous_time = now_units;
    }
}

// Create a WIF name for a variable
void wif_trace_file::create_wif_name(sc_string* ptr_to_str)
{
    char buf[50];
    sprintf(buf,"O%d", wif_name_index);
    *ptr_to_str = buf; 
    wif_name_index++;
}

// Cleanup and close trace file
wif_trace_file::~wif_trace_file()
{
    int i;
    for (i = 0; i < traces.size(); i++) {
        wif_trace* t = traces[i];
        delete t;
    }
    fclose(fp);
}

// Map sc_logic values to values understandable by WIF
static char
map_sc_logic_state_to_wif_state(char in_char)
{
    char out_char;

    switch(in_char){
        case 'U':
        case 'X': 
        case 'W':
        case 'D':
            out_char = 'X';
            break;
        case '0':
        case 'L':
            out_char = '0';
            break;
        case  '1':
        case  'H': 
            out_char = '1';
            break;
        case  'Z': 
            out_char = 'Z';
            break;
        default:
            out_char = '?';
    }
    return out_char;
}


// Print an error message to stderr
static void
wif_put_error_message(const char* msg, bool just_warning)
{
    if(just_warning){
        fprintf(stderr, "WIF Trace Warning:\n%s\n\n",msg);
    }
    else{
        fprintf(stderr, "WIF Trace ERROR:\n%s\n\n", msg);
    }
}

// Create the trace file
sc_trace_file *sc_create_wif_trace_file(const char * name)
{
    sc_trace_file *tf;

    tf = new wif_trace_file(name);
    sc_get_curr_simcontext()->add_trace_file(tf);
    the_dumpfile = tf; // To help sc_dumpall()
    return tf;
}

void sc_close_wif_trace_file( sc_trace_file* tf )
{
    wif_trace_file* wif_tf = (wif_trace_file*)tf;
    delete wif_tf;
}


