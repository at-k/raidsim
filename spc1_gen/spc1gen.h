#ifndef SPC1_GEN_H__
#define SPC1_GEN_H__

#include "../iosrc.h"
#include "BSU.h"
#include "Command.h"

class SPC1Generator : public IoSrc
{
public:
	SPC1Generator();
	~SPC1Generator();

	bool Init( uint32_t lu_num, uint64_t * sector_list );
	bool NextIo( CommandInfo* command );

private:
	int      bsu_num;
	int      idx;
	BSU**    bsuArray;
	Command* cmd;


};

#endif
