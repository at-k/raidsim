#include "spc1gen.h"

// Ese SPC-1 trace generator

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//#include <unistd.h>
#include "BSU.h"
#include "Command.h"
#include "../iogenerator.h"

SPC1Generator::SPC1Generator(){
	bsuArray = NULL;
	bsu_num = 0;
}

SPC1Generator::~SPC1Generator() {
	if(bsuArray != NULL) {
		for(int i = 0; i < bsu_num ; i++)
			delete bsuArray[i];
		delete[] bsuArray;
	}
	if(cmd != NULL) delete cmd;
}

bool SPC1Generator::Init( uint32_t lu_num, uint64_t* sector_list )
{
	uint64_t asu1_size, asu2_size, asu3_size;

	if( bsuArray != NULL )    return false;
    if( lu_num != 3 )         return false;
    if( sector_list == NULL ) return false;

	bsu_num = 20;

	bsuArray = new BSU*[bsu_num];

    asu1_size = sector_list[0];
    asu2_size = sector_list[1];
    asu3_size = sector_list[2];

    for (int i = 0; i < bsu_num; i++)
    {
		bsuArray[i] = new BSU(asu1_size, asu2_size, asu3_size);
	}

	cmd = new Command;
	cmd->timestamp = 0;
	idx = 0;

	return true;
}

bool SPC1Generator::NextIo(CommandInfo* entry)
{
	bsuArray[idx]->generate(cmd);
	//    printf("%d,%ld,%d,%c,%.6f\n", cmd.asu, cmd.lba, cmd.size * 512,
	//	   cmd.opcode ? 'W' : 'R', cmd.timestamp);
	idx++;
	if (idx > bsu_num - 1) {
		idx = 0;
	}
	cmd->timestamp += 0.02 / bsu_num;
	entry->vol = cmd->asu; entry->lba = cmd->lba;
	entry->sector_num = cmd->size;
	entry->timestamp = cmd->timestamp;
	entry->opcode = cmd->opcode ? IO_WRITE : IO_READ;

	return true;
}
