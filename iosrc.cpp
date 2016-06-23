#include <stdlib.h>
#include "util_random.h"
#include "iosrc.h"

bool IoSrc::Init( unsigned int lu_num, unsigned long long int* sector_list )
{
    if ( lu_num == 0 )         return false;
    if ( sector_list == NULL ) return false;

    lu_sector_list.clear();

    for ( unsigned int i = 0; i < lu_num; i ++ )
    {
        lu_sector_list.push_back( sector_list[i] );
    }

    return true;
}

bool GenericIoSrc::CreateIOPattern(
    IO_OPCODE _opcode, unsigned int _tgt_vol,
    unsigned long long int _start_sec, unsigned long long int _end_sec ,
    unsigned int _length, unsigned int _align )
{
    // �p�^�[����������
    pattern.opcode    = _opcode;
    pattern.tgt_vol   = _tgt_vol;
    pattern.start_sec = _start_sec;
    pattern.end_sec   = _end_sec;
    pattern.align     = _align;
    pattern.length    = _length;

    if( pattern.tgt_vol > lu_sector_list.size() )
    {// ���O�w�肳�ꂽLU����葽���ԍ�
        return false;
    }

    if( pattern.align == 0 || pattern.align > 1024 )
    {// �o�E���_�����w��C�܂��͔��ɑ傫���ꍇ
        return false;
    }

    if( pattern.length > 20480 )
    {// �v���T�C�Y�����ɑ傫���ꍇ�i�K�[�h�j
        return false;
    }

    if( pattern.end_sec == 0 )
    {// �I���Z�N�^�ʒu�����w��̏ꍇ
        pattern.end_sec = lu_sector_list[ pattern.tgt_vol ];
    }

    if( pattern.end_sec < pattern.start_sec )
    {// �ςȎw�肪���ꂽ�ꍇ
        return false;
    }

    return true;
}

bool SequentialIO::CreateIOPattern(
    IO_OPCODE _opcode, unsigned int _tgt_vol, unsigned long long int _start_sec ,
    unsigned long long int _end_sec , unsigned int _req_sec, unsigned int _align )
{
    if(! GenericIoSrc::CreateIOPattern(_opcode, _tgt_vol, _start_sec, _end_sec, _req_sec, _align) )
        return false;

    current_lba = pattern.start_sec + pattern.align;

    return true;
}

bool SequentialIO::NextIo( CommandInfo* command )
{
    command->vol        = pattern.tgt_vol;
    command->opcode     = pattern.opcode;
    command->sector_num = pattern.length;
    command->lba        = current_lba;

    current_lba += pattern.length;

    if( current_lba + pattern.length >= pattern.end_sec )
    {// wrapping lba
        current_lba = pattern.start_sec;
    }

    return true;
}

bool  RandomIO::NextIo(CommandInfo* command)
{
    command->vol        = pattern.tgt_vol;
    command->opcode     = pattern.opcode;
    command->sector_num = pattern.length;

    unsigned long long int rand_addr = sim_rand64(pattern.end_sec - pattern.length - pattern.start_sec);

    command->lba = rand_addr + pattern.start_sec;

    if( command->lba  % pattern.align != 0 )
    {// adjusting alignment
        command->lba += (pattern.align - command->lba % pattern.align) ;
    }

    return true;
}
