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
	uint64_t max_gbyte = 64;
	uint32_t write_range = 100; // 0 - 100
	double   op_ratio = 1.25;
	uint64_t ttl_io = 0;

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
                if( strcmp(argv[i], "--gbyte") == 0 || strcmp(argv[i], "-b") ==0 )
                {
                    if( !CheckAdditionalOpt(++i, argc, argv) ) return 0;
                    else max_gbyte = std::stoull(argv[i]);
                }
                else if( strcmp(argv[i], "--write_range") == 0 || strcmp(argv[i], "-w") ==0 )
                {
                    if( !CheckAdditionalOpt(++i, argc, argv) )	return 0;
                    else write_range = std::stoul(argv[i]);
                }
				else if( strcmp(argv[i], "--op_ratio") == 0 || strcmp(argv[i], "-r") ==0 )
				{
					if( !CheckAdditionalOpt(++i, argc, argv) )	return 0;
					else op_ratio = std::stod(argv[i]);
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

	SSD* ssd = new SSD;
	uint64_t lba;
	const uint32_t sect = FTL_SECTS_PER_LP;

	// initialization
	if( !ssd->simple_init(max_gbyte*1024*1024*1024, op_ratio) )
		ERR_AND_RTN;

	uint64_t max_lp = ssd->get_max_lpn();

	for( uint64_t io = 0; io < max_lp; io++ ) {
		//lba = sim_rand64(max_lp) * FTL_SECTS_PER_LP; // random
		lba = io * FTL_SECTS_PER_LP; // sequential
		if( !ssd->write(lba, sect) )
			ERR_AND_RTN;
	}

	ttl_io = ssd->get_max_lpn() * 5;

	// sim start
	time_t now = time(NULL);
	struct tm *pnow = localtime(&now);
	std::cout << "#start time: " << pnow->tm_hour << ":" << pnow->tm_min << ":" << pnow->tm_sec << std::endl;
	// print settings
	std::cout << "#settings, ttl_io, max_gbyte, write_range, op_ratio, lp_num" << std::endl;
	std::cout << "," << ttl_io << "," << max_gbyte << ","<< write_range << "," << op_ratio << "," << max_lp << std::endl;

	// main
	sim_srand(0);

	max_lp = (max_lp /100) * write_range;

	for( uint64_t io = 0; io < ttl_io; io++) {
		lba = sim_rand64(max_lp) * FTL_SECTS_PER_LP;
		if( !ssd->write(lba, sect) )
			ERR_AND_RTN;
	}

	ssd->print_statistics();

	// clean up
	delete ssd;

	now = time(NULL);
	pnow = localtime(&now);
	std::cout << "#end time: " << pnow->tm_hour << ":" << pnow->tm_min << ":" << pnow->tm_sec << std::endl;

	return 0;
}
