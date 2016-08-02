// Defines the entry point for the console application.
//

#include <string.h>
#include <stdio.h>
#include <iostream>

#include "ftl/ftl_lp_info.h"
#include "ftl/ftl_lp_ctl.h"
#include "ftl/ftl_lp_wlctl.h"
#include "util_random.h"
#include "ssd.h"

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


	uint64_t byte_per_die = 1024*1024*1024;

	//SSD* ssd = new SSD();
	SSD* ssd = new CompSSD();
	if( !ssd->init(2,2,2, byte_per_die, 1.25) ) {
		ERR_AND_RTN;
	}

	sim_srand(0);
//	printf("%ld, %ld\n", ssd->get_max_lba(), ssd->get_max_lpn());

	for( uint32_t i = 0; i < ssd->get_max_lpn(); i++ )
	//for( uint32_t i = 0; i < 10; i++ )
	{// io test
		uint64_t lp_no;
		lp_no = sim_rand64( ssd->get_max_lpn() );

		//printf("%ld\n", lp_no);

		if(!ssd->write( lp_no * FTL_SECTS_PER_LP, FTL_SECTS_PER_LP) )  {
			ERR_AND_RTN;
		}
	}

//	printf("successfully finished %ld\n", rw_count);

	std::ofstream writing_file;
	writing_file.open("result.txt", std::ios::out);
	ssd->Dump( writing_file);

	delete ssd;

    return 0;
}
