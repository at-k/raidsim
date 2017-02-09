// Defines the entry point for the console application.
//

#include <string.h>
#include <stdio.h>
#include "sim.h"
#include "controller.h"
#include "util_random.h"

void HowtoUse(char* bin_name)
{
    printf("Usage: %s [options]", bin_name);
    printf("\n");
    printf("  --conf_file,-f file_name\n");
    printf("     set file which discribe test format\n");
    printf("  --out_dir,-o out_dir\n");
    printf("     set output directory\n");
    printf("  --no_ask,-n\n");
    printf("     no ask at end\n");
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
//    const char*     conf_file = "testconf.ini";
//    const char*     out_dir   = "result";
//
//    bool no_ask = true;
//
//    { // analyze input argument
//        int i;
//        if( argc == 1 )
//        {
//            printf("#_no_opt:_load_default_parameter\n");
//        }
//        else
//        {
//            for( i = 1; i < argc; i++ )
//            {
//                if( strcmp(argv[i], "--conf_file") == 0 || strcmp(argv[i], "-f") ==0 )
//                {
//                    if( !CheckAdditionalOpt(++i, argc, argv) ) return 0;
//                    else conf_file = argv[i];
//                }
//                else if( strcmp(argv[i], "--out_dir") == 0 || strcmp(argv[i], "-o") ==0 )
//                {
//                    if( !CheckAdditionalOpt(++i, argc, argv) )	return 0;
//                    else out_dir = argv[i];
//                }
//                else if( strcmp(argv[i], "--no_ask") == 0 || strcmp(argv[i], "-n") ==0 )
//                {
//                    no_ask = true;
//                }
//                else if( strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0 )
//                {
//                    HowtoUse(argv[0]);
//                    return 0;
//                }
//                else
//                {
//                    printf("Invalid option -- %s\n", argv[i]);
//                    HowtoUse(argv[0]);
//                    return 0;
//                }
//            }
//        }
//    }

	std::list<Controller*> ctl_list;
	std::list<Controller*>::iterator itr;

	Controller      ctl_base;

	uint32_t		ctl_num = 4;
	CompController  ctl[ctl_num];

	ctl_list.push_back(&ctl_base);
	for( uint32_t i =0; i < ctl_num; i++)
		ctl_list.push_back(&ctl[i]);

	const uint64_t drv_sect = 50*1024*1024 / 512;
	const uint32_t drv_num = 5;

	DriveInfo drv[drv_num];
	std::vector<DriveInfo*> drv_list;
	for( uint32_t i = 0; i < drv_num; i++ ) {
		drv[i].max_lba = drv_sect;
		drv_list.push_back( &drv[i] );
	}

	for( itr = ctl_list.begin(); itr != ctl_list.end(); itr++ ) {
		if( !(*itr)->build_raid(RAID5, drv_list) )
			return false;
	}

	ctl[0].init(0.2, 0.5, 16);
	ctl[1].init(0.2, 0.5, 32);
	ctl[2].init(0.2, 0.5, 64);
	ctl[3].init(0.2, 0.5, 128);
	//ctl[3].init(0.1, 0.5, 32);
	//ctl[1].init(0.2, 0.6);
	//ctl[2].init(0.2, 0.7);
	//ctl[3].init(0.2, 0.8);

	CommandInfo cmd;
	DriveCommandInfo drv_cmd;

	const uint64_t ttl_io = 1024*1024;
	const uint64_t check_point = ttl_io / 10;
	uint64_t check_count;

	sim_srand(0);

	for( uint64_t i = 0; i < ttl_io; i++ )
	{
		if( check_count > check_point ) {
			printf(" %ld / %ld, ", i , ttl_io );
			fflush(stdout);
			check_count = 0;
		}
		check_count++;

		cmd.opcode = IO_WRITE;
		cmd.sector_num = 16;

		for( itr = ctl_list.begin(); itr != ctl_list.end(); itr++ ) {
			cmd.lba = sim_rand64((*itr)->get_max_lba() - cmd.sector_num );

			if( !(*itr)->receive_command( cmd) )
				return false;

			while( (*itr)->pull_next_command( drv_cmd) ){

			}
		}
	}
	printf("\n");
	for( itr = ctl_list.begin(); itr != ctl_list.end(); itr++ ) {
		(*itr)->print_statistics();
	}

	return 0;

//    SimCore sim;
//    if( !sim.Initialize(conf_file, out_dir) )
//        goto _SIM_END_;
//
//    while(0)
//    {
//        if( !sim.IsSimEnd() )
//            sim.RunStep();
//        else
//        {// ending simulation
//            if( !no_ask )
//            {// continue?
//                printf("continue?[y/n]\n");
//                fflush( stdout );
//
//                int y_n;
//                do {
//                    y_n = getchar();
//				} while( y_n == 0x0a );
//
//                if( y_n == 'y' ) // reset io/timer counter and continue simulator
//                    sim.ResetEndCounter();
//                else if( y_n == 'n' ) // end simulator
//                    break;
//            }
//            else
//                break;
//        }
//    }
//
//_SIM_END_:
//    sim.Close();

    return 0;
}
