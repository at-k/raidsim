// -*- C++ -*-
// Uniform Random Read/Write

#ifndef UNIFORMGEN_H
#define UNIFORMGEN_H

#include "IOGen.h"
#include "Command.h"

class UniformGen : public IOGen {
public:
    UniformGen(int asu, double read_frac, int transfer_size, unsigned long long int vol_size,
        double lower_limit, double upper_limit);
    ~UniformGen();
    virtual int generate(Command *cmd);

private:
    int                    asu;
    double                 read_frac;
    int                    transfer_size;
    unsigned long long int vol_size;
    double                 lower_limit;
    double                 upper_limit;
    unsigned long long int max_addr;
};

#endif // ifndef UNIFORMGEN_H

