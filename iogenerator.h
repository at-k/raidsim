#ifndef IOGENERATOR_H
#define IOGENERATOR_H

#include <stdio.h>
#include <list>

#include "common_def.h"
#include "iosrc.h"

class IoSrc;
class SPC1Generator;

class IoGenerator
{
public:
    IoGenerator(){}
    virtual ~IoGenerator(){ }

    virtual bool GetNextCommand(CommandInfo* command) = 0;
    virtual bool InitGenerator( uint32_t lu_num, uint64_t* sector_list ) {return false;}
	virtual bool InitGenerator( const char* file_name ){return false;}

protected:
    unsigned long long int current_io_num;
};

class SPC1_IoGenerator : public IoGenerator
{
public:
    SPC1_IoGenerator() {generator = NULL;}
    ~SPC1_IoGenerator();

    bool GetNextCommand( CommandInfo* command);
    bool InitGenerator( uint32_t lu_num, uint64_t* sector_list );

private:
	SPC1Generator* generator;
    char*		   buffer;

};

class Cello_IoGenerator : public IoGenerator
{
public:
    Cello_IoGenerator() {fp = NULL;}
    ~Cello_IoGenerator(){ if(fp!=NULL) fclose(fp); }

    bool GetNextCommand(CommandInfo* command);
	bool InitGenerator(const char* file_name);

private:
    char buffer[1024];
    char op_buffer[256];
    char dummy_buffer[256];
    char dummy_buffer_1[256];
    char dummy_buffer_2[256];

	FILE* fp;
	unsigned int total_fail_count;
};

typedef struct
{
    unsigned short  ratio;
    IoSrc*    src;
} IoSrcInfo;

class Generic_IoGenerator: public IoGenerator
{
public:
    Generic_IoGenerator();
    ~Generic_IoGenerator();

    bool GetNextCommand(CommandInfo* command);

    bool InitGenerator( uint32_t lu_num, uint64_t* sector_list );

    bool AddIoPattern( IO_OPCODE op, unsigned int tgtvol,
        unsigned long long int start_sec, unsigned long long int end_sec,
        unsigned int length, unsigned int align,  bool is_random, unsigned short ratio = 10);

    bool AddIoPattern( IOM_PATTERN* pat, bool is_random, unsigned short ratio = 10);

private:
    typedef std::list<IoSrcInfo>            gen_list;
    typedef std::list<IoSrcInfo>::iterator  gen_itr;

    gen_list                 generator;
    unsigned long long int*  lu_sector_list;
    unsigned int             total_lu_num;
};

#endif
