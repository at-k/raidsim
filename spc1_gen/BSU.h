// -*- C++ -*-
// Ese SPC-1 BSU

#ifndef BSU_H
#define BSU_H

#include "IOGen.h"
#include "Command.h"

class BSU {
 public:
     BSU( unsigned long long int asu1_size, unsigned long long int asu2_size, unsigned long long int asu3_size);
     ~BSU();
     int generate(Command *cmd);
     static const int IOGEN_NUM = 8;
     
private:
    IOGen *iogen[IOGEN_NUM];
    double intensity[IOGEN_NUM];
};

#endif // ifndef BSU_H
