#include <string.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <time.h>

#include "sim.h"
#include "iogenerator.h"
#include "controller.h"
#include "ssdsim/ssd.h"
#include "util/util_random.h"

std::string file_list [] = {
	"~/panfs/trace/BuildServers/BuildServer00/BuildServer/Traces/24.hour.BuildServer.11-28-2007.07-24-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer00/BuildServer/Traces/24.hour.BuildServer.11-28-2007.07-39-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer00/BuildServer/Traces/24.hour.BuildServer.11-28-2007.07-55-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer01/BuildServer/Traces/24.hour.BuildServer.11-28-2007.08-10-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer01/BuildServer/Traces/24.hour.BuildServer.11-28-2007.08-25-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer01/BuildServer/Traces/24.hour.BuildServer.11-28-2007.08-40-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer01/BuildServer/Traces/24.hour.BuildServer.11-28-2007.08-55-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer02/BuildServer/Traces/24.hour.BuildServer.11-28-2007.09-10-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer02/BuildServer/Traces/24.hour.BuildServer.11-28-2007.09-26-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer02/BuildServer/Traces/24.hour.BuildServer.11-28-2007.09-41-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer02/BuildServer/Traces/24.hour.BuildServer.11-28-2007.09-56-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer03/BuildServer/Traces/24.hour.BuildServer.11-28-2007.10-11-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer03/BuildServer/Traces/24.hour.BuildServer.11-28-2007.10-26-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer03/BuildServer/Traces/24.hour.BuildServer.11-28-2007.10-42-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer03/BuildServer/Traces/24.hour.BuildServer.11-28-2007.10-57-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer04/BuildServer/Traces/24.hour.BuildServer.11-28-2007.11-12-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer04/BuildServer/Traces/24.hour.BuildServer.11-28-2007.11-27-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer04/BuildServer/Traces/24.hour.BuildServer.11-28-2007.11-43-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer04/BuildServer/Traces/24.hour.BuildServer.11-28-2007.11-58-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer05/BuildServer/Traces/24.hour.BuildServer.11-29-2007.12-13-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer05/BuildServer/Traces/24.hour.BuildServer.11-29-2007.12-28-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer05/BuildServer/Traces/24.hour.BuildServer.11-29-2007.12-44-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer05/BuildServer/Traces/24.hour.BuildServer.11-29-2007.12-59-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer06/BuildServer/Traces/24.hour.BuildServer.11-29-2007.01-14-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer06/BuildServer/Traces/24.hour.BuildServer.11-29-2007.01-29-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer06/BuildServer/Traces/24.hour.BuildServer.11-29-2007.01-44-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer06/BuildServer/Traces/24.hour.BuildServer.11-29-2007.01-59-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.01-05-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.01-20-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.01-35-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.01-50-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.02-05-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.02-14-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.02-20-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.02-30-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.02-36-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.02-45-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.02-51-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.03-00-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.03-06-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.03-15-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.03-21-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.03-30-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.03-36-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.03-45-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.03-51-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.04-00-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.04-06-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.04-15-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.04-21-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.04-31-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.04-36-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.04-46-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.04-52-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.05-01-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.05-07-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.05-16-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.05-22-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.05-31-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.05-37-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.05-46-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.05-52-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.06-01-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.06-07-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.06-16-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.06-22-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.06-32-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.06-38-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.06-47-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.06-53-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.07-02-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.07-08-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.07-17-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.07-23-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.07-32-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.07-47-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.08-02-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.08-17-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.08-33-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.08-48-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.09-03-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.09-18-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.09-33-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.09-48-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.10-03-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.10-18-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.10-34-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.10-49-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.11-04-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.11-19-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.11-34-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.11-49-AM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.12-04-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.12-19-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.12-35-PM.trace.csv",
	"~/panfs/trace/BuildServers/BuildServer07/BuildServer/Traces/24.hour.BuildServer.11-29-2007.12-50-PM.trace.csv"
};

uint64_t max_lba_list [] = {
	314572800,
	314572800,
	314572800,
	314572800,
	314572800,
	314572800,
	314572800,
	314572800
};
//487575632,
//487598944,
//487598936,
//487598984,
//487598936,
//487598936,
//487598936,
//487598936,
//886942706,
//487598936,
//487598936,
//487598936,
//487598936,
//487575632,
//487598936,
//487598936,
//28525584 ,
//71654624 ,
//0        ,
//95533124
//};

//378588288 ,
//378601642 ,
//378600698 ,
//378599402 ,
//378601178 ,
//378600250 ,
//378599274 ,
//378598522 ,
//476292577 ,
//378601034 ,
//378598810 ,
//378599706 ,
//378597306 ,
//378589152 ,
//378601626 ,
//378601210 ,
//19487423  ,
//95593616  ,
//0         ,
//95533124
//};
//


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
	std::string cmp_trace_file = "osdb_comp_trace.txt";
	std::string io_trace_file = "";
	bool io_trace_analysis_mode = false;
	bool single_ssd_mode = true;
	bool skip_ssd_mode = false;

	uint32_t tgt_vol = 0;

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
                else if( strcmp(argv[i], "--cmp_trace_file") == 0 || strcmp(argv[i], "-c") ==0 )
                {
                    if( !CheckAdditionalOpt(++i, argc, argv) ) return 0;
                    else cmp_trace_file = argv[i];
				}
				else if( strcmp(argv[i], "--io_trace_file") == 0 || strcmp(argv[i], "-t") ==0 )
				{
					if( !CheckAdditionalOpt(++i, argc, argv) ) return 0;
					else io_trace_file = argv[i];
				}
				else if( strcmp(argv[i], "--tgt_vol") == 0 || strcmp(argv[i], "-v") ==0 )
				{
					if( !CheckAdditionalOpt(++i, argc, argv) ) return 0;
					else tgt_vol = std::stol(argv[i]);
				}
				else if( strcmp(argv[i], "--io_trace_analysis") == 0 || strcmp(argv[i], "-ia") ==0 )
				{
					io_trace_analysis_mode = true;
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
	IoGenerator*			iogen;

	ssd_list.resize(drv_num);
	drv_list.resize(drv_num);

//	TPCC_IoGenerator iogen_tpcc;
//
//	if( !io_trace_file.empty() ) {
//		if( !iogen_tpcc.InitGenerator( io_trace_file.c_str() ) ) {
//			printf("error\n");
//		}
//		iogen = &iogen_tpcc;
//	}else {
//		std::cout <<  ": io trace file is required in this version" << std::endl;
//		return false;
//	}

	if( io_trace_analysis_mode )
	{
		CommandInfo cmd_kari;
		std::vector<uint64_t> max_lba;
		max_lba.clear();

		while( iogen->GetNextCommand( &cmd_kari ) ) {
			//std::cout << cmd_kari.vol << "," << cmd_kari.lba << std::endl;
			if( cmd_kari.vol >= max_lba.size() ) {
				max_lba.resize(cmd_kari.vol+1);
			}
			if( max_lba[cmd_kari.vol] < cmd_kari.lba + cmd_kari.sector_num )
				max_lba[cmd_kari.vol] = cmd_kari.lba + cmd_kari.sector_num;
		}
		for(uint32_t i = 0; i < max_lba.size(); i++) {
			std::cout << i << "," << max_lba[i] << std::endl;
 		}
	} else
	{
		TPCC_IoGenerator* iogen_tpcc;
		HdpController* hdp_con = new HdpController;

		std::cout << "lu initializing start" << std::endl;
		uint32_t la_vol = 0;
		for( auto max_lba : max_lba_list ) {
			if( !hdp_con->create_lu( max_lba, la_vol)) {
				std::cout << "fail to create lu" << std::endl;
				return false;
			}
			la_vol ++;
		}
		std::cout << "done" << std::endl;

		uint32_t counter = 0;

		for( auto t_file : file_list) {
			iogen_tpcc = new TPCC_IoGenerator();
			if( !iogen_tpcc->InitGenerator( t_file.c_str() ) ) {
				printf("error\n");
				return false;
			}

			CommandInfo cmd_kari;

			std::cout << "start:" << t_file << std::endl;

			while( iogen_tpcc->GetNextCommand( &cmd_kari ) ) {
				if( cmd_kari.vol != tgt_vol )
					continue;

				if( !hdp_con->receive_command( cmd_kari ) ) {
					std::cout << "fail to process IO command: command no = "
						<< iogen_tpcc->GetCurrentIoCount() << std::endl;
					return false;
				}

				if( iogen_tpcc->GetCurrentIoCount() > 2 )
					break;
			}

			std::string result_file = std::to_string(counter) + "_result.txt";
			FILE* fp = freopen(result_file.c_str(), "w", stdout);
			hdp_con->print_statistics();
			fclose(fp);
			fp = freopen ("/dev/tty", "a", stdout);

			counter ++;
			delete iogen_tpcc;
		}
	}

	return 0;

	// initialization
	if( !cmp_engine.init_engine(cmp_trace_file.c_str()) )
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
		<< ","<< ctl_op_ratio << ","<< ctl_cmp_chunk << ","<< ssd_op_ratio << ","<< ttl_io << "," << cmp_trace_file << "," << ((skip_ssd_mode)? "on":"off") << std::endl;

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