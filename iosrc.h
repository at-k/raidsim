#ifndef IOSRC__H
#define IOSRC__H

#include <string.h>
#include <vector>
//#include "iogenerator.h"

// OP type definition
typedef enum
{
    IO_READ,
    IO_WRITE,

} IO_OPCODE;

// Command information
typedef struct
{
    IO_OPCODE                 opcode;
    unsigned int              vol;
    unsigned long long int    lba;
    unsigned int              sector_num;
    double                    timestamp;
    unsigned long long int    sim_timestamp;
    unsigned long long int    io_id;
} CommandInfo;

typedef struct
{
    IO_OPCODE				opcode;
    unsigned int			tgt_vol;
    unsigned long long int  start_sec;
    unsigned long long int  end_sec;
    unsigned int			align;
    unsigned int			length;
} IOM_PATTERN;

class IoSrc
{
public:
    IoSrc(){}
    virtual ~IoSrc(){}

    virtual bool Init( unsigned int lu_num, unsigned long long int* sector_list );
    virtual bool NextIo(CommandInfo* command) = 0;
    virtual void PrintSettings() {}

protected:
    std::vector<unsigned long long int> lu_sector_list;
};

class GenericIoSrc : public IoSrc
{
public:
    GenericIoSrc():IoSrc(){ memset(&pattern, 0, sizeof(IOM_PATTERN)); }
    ~GenericIoSrc(){}

    virtual bool CreateIOPattern(
        IO_OPCODE op,                         unsigned int tgtvol,
        unsigned long long int start_sec = 0, unsigned long long int end_sec = 0,
        unsigned int length = 1,              unsigned int align = 1);

    bool CreateIOPattern( IOM_PATTERN* p )
    {
        return CreateIOPattern(p->opcode, p->tgt_vol, p->start_sec, p->end_sec, p->length, p->align);
    }
    bool NextIo(CommandInfo* command) = 0;

protected:
    IOM_PATTERN pattern;
};

class SequentialIO : public GenericIoSrc
{
public:
    SequentialIO() : GenericIoSrc(){current_lba = 0;}
    ~SequentialIO(){}

    bool CreateIOPattern (
        IO_OPCODE op,                         unsigned int tgtvol,
        unsigned long long int start_sec = 0, unsigned long long int end_sec = 0,
        unsigned int req_sec = 1,             unsigned int align = 1);

    bool NextIo ( CommandInfo* command );

//    void PrintSettings();

private:
    unsigned long long int current_lba;
};

class RandomIO : public GenericIoSrc
{
public:
    RandomIO() : GenericIoSrc(){ }
    ~RandomIO(){}

    bool NextIo( CommandInfo* command );
//    void PrintSettings();

private:

};

#endif
