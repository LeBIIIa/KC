
#include "systemc.h"
#include "S1.h"

//definition of multdiv method


void S1::mainFunc()
{
	o1.write(X + Y);
	o2.write((X ^ Y) - (X << 1));
} // end of multdiv