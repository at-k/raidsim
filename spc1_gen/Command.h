// -*- C++ -*-
// Read/Write Command

#ifndef COMMAND_H
#define COMMAND_H

class Command 
{
public:
    typedef enum { READ = 0, WRITE } IO_OPCODE;
    int asu;
    unsigned long long int lba;
    int size;
    IO_OPCODE opcode;
    double timestamp;
};

#endif // ifndef COMMAND_H
