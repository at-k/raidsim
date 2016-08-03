#include <string.h>
#include <stdio.h>
#include <vector>

#include "sim.h"

#include "controller.h"
#include "ssdsim/ssd.h"
#include "util_random.h"

typedef enum {
	CTL_SIDE,
	SSD_SIDE
}COMP_MODE;

void HowtoUse(char* bin_name)
{
    printf("Usage: %s [options]", bin_name);
    printf("\n");
	printf("  --help,-h \n");
    printf("     print this message\n");
    printf("\n");
}

bool CheckAdditionalOpt(int i, int argc, char* argv[])
{
    if( i >= argc || argv == NULL) {
        printf("Option Error\nThis option requires an additional argument -- %s\n", argv[i-1]);
        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
	// parameter
	const uint32_t drv_num = 5;
	const uint64_t max_drv_bytes = (uint64_t)32*(1024*1024*1024);
	const double   avg_cmp_ratio = 0.5;
	const double   ctl_op_ratio  = 1.25;
	const uint32_t ctl_cmp_chunk = 16;
	const double   ssd_op_ratio  = 1.25;
	const uint64_t ttl_io = 32*1024*1024 ;
	const uint32_t io_size_sect = 16;
	const COMP_MODE mode = CTL_SIDE;

	Controller* ctl;
	std::vector<SSD*>		ssd_list;
	std::vector<DriveInfo*> drv_list;

	ssd_list.resize(drv_num);
	drv_list.resize(drv_num);

	// initialization
	if( mode == CTL_SIDE )
	{ // controller side compression
		for( uint32_t i = 0; i < drv_num; i++ ) {
			ssd_list[i] = new SSD;
			drv_list[i] = new DriveInfo;

			if( !ssd_list[i]->simple_init( max_drv_bytes, ssd_op_ratio))
				ERR_AND_RTN;
			drv_list[i]->max_lba = ssd_list[i]->get_max_lba();
		}

		CompController* c_ctl = new CompController;

		if( !c_ctl->build_raid(RAID5, drv_list) )
			ERR_AND_RTN;
		if( !c_ctl->init(1 - 1/ctl_op_ratio, avg_cmp_ratio, ctl_cmp_chunk) )
			ERR_AND_RTN;
		ctl = c_ctl;
	} else
	{ // ssd side compression
		for( uint32_t i = 0; i < drv_num; i++ ) {
			CompSSD* c_ssd = new CompSSD;
			if( !c_ssd->setup_cmp_engine(avg_cmp_ratio) )
				ERR_AND_RTN;
			if( !c_ssd->simple_init( max_drv_bytes, ssd_op_ratio ) )
				ERR_AND_RTN;

			ssd_list[i] = c_ssd;
			drv_list[i] = new DriveInfo;
			drv_list[i]->max_lba = ssd_list[i]->get_max_lba();
		}

		ctl = new Controller;
		if( !ctl->build_raid(RAID5, drv_list) )
			ERR_AND_RTN;
	}

	// main
	CommandInfo cmd;
	DriveCommandInfo drv_cmd;

	//const uint64_t check_point = ttl_io / 10;
	//uint64_t check_count;

	sim_srand(0);

	for( uint64_t i = 0; i < ttl_io; i++ )
	{
	//	if( check_count > check_point ) {
	//		printf(" %ld / %ld, ", i , ttl_io );
	//		fflush(stdout);
	//		check_count = 0;
	//	}
//		check_count++;

		cmd.opcode = IO_WRITE;
		cmd.sector_num = io_size_sect;

		cmd.lba = sim_rand64(ctl->get_max_lba() - cmd.sector_num );

		if( !ctl->receive_command( cmd) )
			return false;

		while( ctl->pull_next_command(drv_cmd) ){
			CommandInfo& cmd2drv = drv_cmd.cmd_info;
			if( cmd2drv.opcode == IO_WRITE  ) {
				if( !ssd_list[drv_cmd.tgt_drive]->write(cmd2drv.lba, cmd2drv.sector_num) ) {
					ERR_AND_RTN;
				}
			} else if( cmd2drv.opcode == IO_READ ) {
				if( !ssd_list[drv_cmd.tgt_drive]->read(cmd2drv.lba, cmd2drv.sector_num) ) {
					ERR_AND_RTN;
				}
			}else
				ERR_AND_RTN;
		}
	}

	//printf("\n");
	ctl->print_statistics();
	for( auto s : ssd_list ) {
		s->print_statistics();
	}

	// clean up
	delete ctl;
	for( auto s : ssd_list )
		delete s;
	ssd_list.clear();
	for( auto d : drv_list )
		delete d;

	return 0;
}
