
#include "systemc.h"
#include "S2.h"

//definition of multdiv method
void S2::mainFunc()
{
	long a, b;
	a = in1.read();
	b = in2.read();
	o1.write(a ^ b);
	o2.write(~(a ^ b));
} // end of multdiv