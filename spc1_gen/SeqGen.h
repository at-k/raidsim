// -*- C++ -*-
// Sequential Read/Write

#ifndef SEQGEN_H
#define SEQGEN_H

#include "IOGen.h"
#include "Command.h"

class SeqGen : public IOGen {
public:
    SeqGen(int asu, double read_frac, unsigned long long int vol_size, double start, double startvar,
        int stride, double length);
    ~SeqGen();
    virtual int generate(Command *cmd);

private:
    unsigned long long int get_start_addr(int transfer_size);
    int get_smix(void);
    int asu;
    double read_frac;
    unsigned long long int vol_size;
    double start;
    double startvar;
    int stride;
    double length;
    unsigned long long int max_addr;
    unsigned long long int last_addr;
    int last_transfer_size;
};

#endif // ifndef SEQGEN_H

