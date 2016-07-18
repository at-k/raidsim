// -*- C++ -*-
// I/O Generator

#ifndef IOGEN_H
#define IOGEN_H

#include <stdlib.h>
#include "Command.h"
#include "util_random.h"

class IOGen
{
public:
    virtual ~IOGen() {};
    virtual int generate(Command *cmd) = 0;
    double get_random(void) { return (double(sim_rand32(RAND_MAX)) / (RAND_MAX + 1.0)); }
    static const int alignment = 8;
};

#endif // ifndef IOGEN_H
