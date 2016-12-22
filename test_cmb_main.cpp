#include <string.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include "sim.h"
#include <time.h>

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
	uint32_t drv_num = 5;
	uint64_t max_drv_bytes = (uint64_t)32*(1024*1024*1024);
	//double   avg_cmp_ratio = 0.5;
	double   ctl_op_ratio  = 1.25;
	uint32_t ctl_cmp_chunk = 16;
	double   ssd_op_ratio  = 1.25;
	uint64_t ttl_io = 32*1024*1024 ;
	// uint64_t ttl_io = 1 ;
	uint32_t io_size_sect = 8;
	COMP_MODE mode = CTL_SIDE;
	std::string trace_file = "osdb_comp_trace.txt";
	bool single_ssd_mode = true;
	bool skip_ssd_mode = false;

    { // analyze input argument
        int i;
        if( argc == 1 )
        {
            printf("#_no_opt:_load_default_parameter\n");
        }
        else
        {
            for( i = 1; i < argc; i++ )
            {
                if( strcmp(argv[i], "--drive_num") == 0 || strcmp(argv[i], "-d") ==0 )
                {
                    if( !CheckAdditionalOpt(++i, argc, argv) ) return 0;
                    else drv_num = atoi(argv[i]);
                }
                else if( strcmp(argv[i], "--max_drive_bytes") == 0 || strcmp(argv[i], "-b") ==0 )
                {
                    if( !CheckAdditionalOpt(++i, argc, argv) )	return 0;
                    else max_drv_bytes = std::stoull(argv[i]);
                }
//				else if( strcmp(argv[i], "--avg_cmp_ratio") == 0 || strcmp(argv[i], "-c") ==0 )
//				{
//					if( !CheckAdditionalOpt(++i, argc, argv) )	return 0;
//					else avg_cmp_ratio = std::stod(argv[i]);
//				}
				else if( strcmp(argv[i], "--comp_mode") == 0 || strcmp(argv[i], "-m") ==0 )
				{
					if( !CheckAdditionalOpt(++i, argc, argv) )	return 0;
					else if( strcmp(argv[i], "c") == 0 )
						mode = CTL_SIDE;
					else if( strcmp(argv[i], "s") == 0 )
						mode = SSD_SIDE;
					else {
						printf("invalid option for mode%s\n", argv[i] );
						return false;
					}
				}
				else if( strcmp(argv[i], "--ctl_op_ratio") == 0 || strcmp(argv[i], "-R") ==0 )
				{
					if( !CheckAdditionalOpt(++i, argc, argv) )	return 0;
					else ctl_op_ratio = std::stod(argv[i]);
                }
				else if( strcmp(argv[i], "--ctl_cmp_chunk") == 0 || strcmp(argv[i], "-C") ==0 )
				{
					if( !CheckAdditionalOpt(++i, argc, argv) )	return 0;
					else ctl_cmp_chunk = std::stol(argv[i]);
				}
				else if( strcmp(argv[i], "--ssd_op_ratio") == 0 || strcmp(argv[i], "-r") ==0 )
				{
					if( !CheckAdditionalOpt(++i, argc, argv) )	return 0;
					else ssd_op_ratio = std::stod(argv[i]);
                }
				//else if( strcmp(argv[i], "--ttl_io_cnt") == 0 || strcmp(argv[i], "-i") ==0 )
				//{
				//	if( !CheckAdditionalOpt(++i, argc, argv) )	return 0;
				//	else ttl_io = std::stoll(argv[i]);
                //}
                else if( strcmp(argv[i], "--trace_file") == 0 || strcmp(argv[i], "-t") ==0 )
                {
                    if( !CheckAdditionalOpt(++i, argc, argv) ) return 0;
                    else trace_file = argv[i];
				}
				else if( strcmp(argv[i], "--skip_ssd") == 0 )
				{
					skip_ssd_mode = true;
				}
                else if( strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0 )
                {
                    HowtoUse(argv[0]);
                    return 0;
                }
                else
                {
                    printf("Invalid option -- %s\n", argv[i]);
                    HowtoUse(argv[0]);
                    return 0;
                }
            }
        }
    }

	Controller* ctl;
	std::vector<SSD*>		ssd_list;
	std::vector<DriveInfo*> drv_list;
	CompEngine				cmp_engine;

	ssd_list.resize(drv_num);
	drv_list.resize(drv_num);

	// initialization
	if( !cmp_engine.init_engine(trace_file.c_str()) )
		ERR_AND_RTN;



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
		if( !c_ctl->init(&cmp_engine, 1 - 1/ctl_op_ratio, ctl_cmp_chunk) )
			ERR_AND_RTN;
		ctl = c_ctl;
	} else
	{ // ssd side compression
		for( uint32_t i = 0; i < drv_num; i++ ) {
			CompSSD* c_ssd = new CompSSD;
			bool enable_virt = true;
			uint32_t buffered_page_num = 4;

			if( !c_ssd->setup_compression(enable_virt, &cmp_engine, A_CC_FTL, buffered_page_num ) )
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

	// sim start
	time_t now = time(NULL);
	struct tm *pnow = localtime(&now);
	std::cout << "#start time: " << pnow->tm_hour << ":" << pnow->tm_min << ":" << pnow->tm_sec << std::endl;
	// print settings
	std::cout << "#settings, mode, dnum, drive_sect, drive_lba, cmp_ratio, ctl_opr, ctl_chunk, ssd_opr, ttl_io, comp_trace, skip_ssd" << std::endl;
	std::cout << "," << (mode == CTL_SIDE ? "C" : "S") << "," << drv_num << ","<< ssd_list[0]->get_phy_sect() << "," << ssd_list[0]->get_max_lba() << ","<< cmp_engine.get_avg_cmp_ratio()
		<< ","<< ctl_op_ratio << ","<< ctl_cmp_chunk << ","<< ssd_op_ratio << ","<< ttl_io << "," << trace_file << "," << ((skip_ssd_mode)? "on":"off") << std::endl;

	// main
	CommandInfo cmd;
	DriveCommandInfo drv_cmd;

	//const uint64_t check_point = ttl_io / 100;
	//uint64_t check_count = 0;

	sim_srand(0);

	uint32_t round = 2;
	ttl_io = (ctl->get_max_lba()/io_size_sect);
	uint64_t io_align = (ctl->get_max_lba()/io_size_sect) - 1;

	//std::cout << "caution: test code included" << std::endl;

	for( uint32_t r = 0; r < round ; r++ )
	{
		for( uint64_t i = 0; i < ttl_io; i++ )
		{
				//if( check_count > check_point ) {
				//	printf(" %ld / %ld, ", i , ttl_io );
				//	fflush(stdout);
				//	check_count = 0;
				//}
				//	check_count++;

			cmd.opcode = IO_WRITE;
			cmd.sector_num = io_size_sect;

			//cmd.lba = sim_rand64(ctl->get_max_lba() - cmd.sector_num );
			cmd.lba = sim_rand64(io_align) * io_size_sect;

			if( !ctl->receive_command( cmd) )
				return false;

			while( ctl->pull_next_command(drv_cmd) )
			{
				CommandInfo& cmd2drv = drv_cmd.cmd_info;

				if( skip_ssd_mode )
					continue;

				if( single_ssd_mode && drv_cmd.tgt_drive != 0 )
					continue;

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
		if( r == 0 ) {
			ctl->clear_statistics();
			for( auto s : ssd_list )
				s->clear_statistics();
		}
	}

	//printf("\n");
	ctl->print_statistics();
	if( single_ssd_mode )
		ssd_list[0]->print_statistics();
	else {
		for( auto s : ssd_list ) {
			s->print_statistics();
		}
	}

	// clean up
	delete ctl;
	for( auto s : ssd_list )
		delete s;
	ssd_list.clear();
	for( auto d : drv_list )
		delete d;

	now = time(NULL);
	pnow = localtime(&now);
	std::cout << "#end time: " << pnow->tm_hour << ":" << pnow->tm_min << ":" << pnow->tm_sec << std::endl;

	return 0;
}
