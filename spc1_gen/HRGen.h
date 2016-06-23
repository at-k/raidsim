// -*- C++ -*-
// The Hierarchical Reuse Random Walk Command Generator

#ifndef HRGEN_H
#define HRGEN_H

#include "IOGen.h"
#include "Command.h"

class HRGen : public IOGen
{
 public:
     HRGen(int asu, double read_frac, int transfer_size, unsigned long long int vol_size,
         double lower_limit, double upper_limit);
     ~HRGen();
     int      select_leaf(int last_leaf);
     unsigned long long int select_addr(Command::IO_OPCODE);
     virtual int generate(Command *cmd);

private:
    static const int h_start = 7;
    static const int leaf_size = 32 / 4; // [block]
    static const int block_size = 4096 / 512; // [sector]
#ifndef WIN32
    constexpr static const double climb_ratio = 0.44;
#else
    static const double climb_ratio;
#endif
    int      asu;
    double   read_frac;
    int      transfer_size;
    int      h_max;
    unsigned long long int offset;
    int      node_num;
    int    *last_read;
    unsigned long long int last_written_addr;
    int      leaf;
};

#ifdef WIN32
__declspec( selectany ) const double HRGen::climb_ratio( 0.44);
#endif

#endif // ifndef HRGEN_H
