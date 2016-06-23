// -*- C++ -*-
// I/O Stream

#ifndef IOSTREAM_H
#define IOSTREAM_H

#include <stdlib.h>
#include "Command.h"

class IOStream {
public:
	virtual int generate(Command *cmd) = 0;
	double get_random(void) { return (double(rand32(RAND_MAX)) / (RAND_MAX + 1.0)); }
	static const int alignment = 8;
};

#endif // ifndef IOSTREAM_H
