#include <stdio.h>
#include <string.h>

#include "iogenerator.h"
#include "iosrc.h"
#include "spc1_gen/spc1gen.h"

#ifdef WIN32
#pragma warning(disable:4996)
#endif

SPC1_IoGenerator::~SPC1_IoGenerator()
{
    if( generator != NULL )
        delete generator;
}

bool SPC1_IoGenerator::GetNextCommand( CommandInfo* command )
{
    current_io_num++;
    return generator->NextIo(command);
}

bool SPC1_IoGenerator::InitGenerator( uint32_t lu_num, uint64_t* sector_list )
{
    if(this->generator != NULL)
        return false;

    generator = new SPC1Generator();
    current_io_num = 0;

    return generator->Init( lu_num, sector_list );
}


bool  Cello_IoGenerator::GetNextCommand(CommandInfo* command)
{
    // 発行IO総数をインクリメント
    current_io_num++;

    int  fail_count = 0;
    int  size_byte  = 0;
    int  ret        = 0;

    while( (fail_count < 100) &&
        (fgets(buffer, sizeof(buffer), fp) != NULL) )
    {
        total_fail_count += fail_count;

        // ファイルから取得したIOパタンを分解
        sscanf(buffer, "%s %s %s %s %s %s %s %s %d",
            op_buffer, dummy_buffer, dummy_buffer, dummy_buffer, dummy_buffer, dummy_buffer_1,
            dummy_buffer, dummy_buffer_2, &size_byte);

        if ((strlen(dummy_buffer_1) > 9) || (strlen(dummy_buffer_2) > 9))
        {
            fail_count++;
            continue;
        }

        ret = sscanf(buffer, "%s %s %s %s %s %du %s %llu %d",
            op_buffer, dummy_buffer, dummy_buffer, dummy_buffer, dummy_buffer, &(command->vol),
            dummy_buffer, &(command->lba), &size_byte);

        if (ret != 9)
        {
            fail_count++;
            continue;
        }

        command->sector_num = ( size_byte + 512 - 1) / 512; // はてどういう計算だったか…

        if( op_buffer[0] == 'R' || op_buffer[0] == 'r' )
            command->opcode = IO_READ;
        else if( op_buffer[0] == 'W' || op_buffer[0] == 'w' )
            command->opcode = IO_WRITE;
        else
            return false;

        return true;
    }

    return false;
}

bool Cello_IoGenerator::InitGenerator(const char* file_name)
{
	if( fp != NULL )
		return false;

	fp = fopen(file_name, "r");
	if( fp == NULL )
		return false;

	total_fail_count = 0;

	return true;
}

Generic_IoGenerator::Generic_IoGenerator()
{
    lu_sector_list = NULL;
    generator.clear();
}

Generic_IoGenerator::~Generic_IoGenerator()
{
    for(gen_itr itr = generator.begin(); itr != generator.end(); itr ++)
        delete (*itr).src;
    generator.clear();

    if( lu_sector_list != NULL )
        delete [] lu_sector_list;
}

bool  Generic_IoGenerator::InitGenerator( uint32_t lu_num, uint64_t * sector_list )
{
    if( !generator.empty() )
        return false;

    if( lu_sector_list != NULL )
        return false;

    lu_sector_list = new (unsigned long long int)(lu_num);
    total_lu_num   = lu_num;

    for ( unsigned int i = 0; i < lu_num; i ++ )
    {
        lu_sector_list[i] = sector_list[i];
    }

    current_io_num = 0;

    return true;
}

bool Generic_IoGenerator::AddIoPattern(
    IO_OPCODE op,        unsigned int tgtvol,
    unsigned long long int  start_sec, unsigned long long int end_sec,
    unsigned int  length,   unsigned int align,   bool is_random, unsigned short ratio )
{
    GenericIoSrc* io;
    if( is_random )
        io = new RandomIO;
    else
        io = new SequentialIO;

    if( !io->Init( total_lu_num, lu_sector_list ) )
        return false;

    if( !io->CreateIOPattern(op, tgtvol, start_sec, end_sec, length, align) )
        return false;

    IoSrcInfo gen;
    gen.src   = io;
    gen.ratio = ratio;
    generator.push_back(gen);

    unsigned short ratio_sum = 0;
    for(gen_itr itr = generator.begin(); itr != generator.end(); itr ++)
        ratio_sum += (*itr).ratio;

    if( ratio_sum > 100 )
        return false;

    return true;
}

bool Generic_IoGenerator::AddIoPattern(IOM_PATTERN* pat, bool is_random, unsigned short ratio)
{
    return AddIoPattern(pat->opcode, pat->tgt_vol,
        pat->start_sec, pat->end_sec, pat->length, pat->align, is_random, ratio);
}

bool  Generic_IoGenerator::GetNextCommand(CommandInfo* command)
{
    if( generator.empty() )
        return false;

    current_io_num++;

    command->io_id = current_io_num;

    unsigned int choise = 0;
    unsigned int ratio_sum   = 0;
    choise = sim_rand32(100);

    gen_itr itr;
    for( itr = generator.begin(); itr != generator.end(); itr ++) {
        ratio_sum += (*itr).ratio;
        if( ratio_sum >= choise )
            break;
    }
    if( itr == generator.end() )
	{
		printf("hoge %lld %d %d\n", current_io_num, choise ,ratio_sum);
		return false;
	}

    return (*itr).src->NextIo(command);
}

