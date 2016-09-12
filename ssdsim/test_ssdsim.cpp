// Defines the entry point for the console application.
//

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <time.h>

#include "ftl/ftl_lp_info.h"
#include "ftl/ftl_lp_ctl.h"
#include "ftl/ftl_lp_wlctl.h"
#include "util_random.h"
#include "ssd.h"

typedef enum {
	PAGE_BASED,
	CA_FTL,
	CC_FTL,
	CC_NOCOMP
} FTL_TYPE;

void HowtoUse(char* bin_name)
{
    printf("Usage: %s [options]", bin_name);
    printf("\n");
//    printf("  --conf_file,-f file_name\n");
//    printf("     set file which discribe test format\n");
//    printf("  --out_dir,-o out_dir\n");
//    printf("     set output directory\n");
//    printf("  --no_ask,-n\n");
//    printf("     no ask at end\n");
//    printf("  --help,-h \n");
//    printf("     print this message\n");
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
	uint64_t byte_per_die = (uint64_t)1*1024*1024*1024;
	double   op_rate = 1.25;
	FTL_TYPE ftl_type = CC_FTL;
	SSD* ssd = NULL;
	bool enable_virtualization = false;
	std::string trace_file = "osdb_comp_trace.txt";
	uint32_t buffered_page_num = 4;

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
                if( strcmp(argv[i], "--op_rate") == 0 || strcmp(argv[i], "-o") ==0 )
                {
                    if( !CheckAdditionalOpt(++i, argc, argv) ) return 0;
                    else op_rate = atof(argv[i]);
                }
                else if( strcmp(argv[i], "--buff_num") == 0 || strcmp(argv[i], "-b") ==0 )
                {
                    if( !CheckAdditionalOpt(++i, argc, argv) ) return 0;
                    else buffered_page_num = atoi(argv[i]);
                }
                else if( strcmp(argv[i], "--ftl") == 0 || strcmp(argv[i], "-f") ==0 )
                {
                    if( !CheckAdditionalOpt(++i, argc, argv) )	return 0;
                    else {
						if( strcmp(argv[i], "pa") == 0 )
							ftl_type = PAGE_BASED;
						else if( strcmp(argv[i], "ca") == 0 )
							ftl_type = CA_FTL;
						else if( strcmp(argv[i], "cc") == 0 )
							ftl_type = CC_FTL;
						else if( strcmp(argv[i], "cc-nocomp") == 0 )
							ftl_type = CC_NOCOMP;
						else {
							printf("Invalid option -- %s\n", argv[i]);
							HowtoUse(argv[0]);
							return 0;
						}
					}
                }
                else if( strcmp(argv[i], "--trace_file") == 0 || strcmp(argv[i], "-t") ==0 )
                {
                    if( !CheckAdditionalOpt(++i, argc, argv) ) return 0;
                    else trace_file = argv[i];
				}
                else if( strcmp(argv[i], "--en_vir") == 0 || strcmp(argv[i], "-v") ==0 )
                {
					enable_virtualization = true;
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
	// print settings
	time_t now = time(NULL);
	struct tm *pnow = localtime(&now);
	std::cout << "#start time: " << pnow->tm_hour << ":" << pnow->tm_min << ":" << pnow->tm_sec << std::endl;

	std::cout << "#" << argv[0] << "," << ftl_type <<","<<enable_virtualization<<","<<trace_file<<","<<op_rate<<std::endl;

	CompEngine cmp_engine;
	if( ftl_type == CC_NOCOMP)
		cmp_engine.init_engine(1.0);
	else
		cmp_engine.init_engine(trace_file.c_str());

	//std::cout << cmp_engine.get_next_ratio() << std::endl;
	//return 0;

	if( ftl_type == CC_FTL || ftl_type == CA_FTL) {
		CompSSD* cssd = new CompSSD();
		if( ftl_type == CC_FTL) {
			if( !cssd->setup_compression( enable_virtualization, &cmp_engine, A_CC_FTL, buffered_page_num ) ) {
				ERR_AND_RTN;
			}
		} else {
			if( !cssd->setup_compression( enable_virtualization, &cmp_engine, A_CA_FTL ,buffered_page_num) ) {
				ERR_AND_RTN;
			}
		}
		ssd = cssd;
	} else
		ssd = new SSD();


	if( !ssd->init(2,2,2, byte_per_die, op_rate) ) {
		ERR_AND_RTN;
	}

	sim_srand(0);
//	printf("%ld, %ld\n", ssd->get_max_lba(), ssd->get_max_lpn());
	std::vector<uint64_t> write_list;

	for( uint32_t i = 0; i < ssd->get_max_lpn(); i++ )
	//for( uint32_t i = 0; i < 12; i++ )
	{// io test
		uint64_t lp_no;
		lp_no = sim_rand64( ssd->get_max_lpn() );
		write_list.push_back(lp_no);

		//printf("%ld\n", lp_no);

		if(!ssd->write( lp_no * FTL_SECTS_PER_LP, FTL_SECTS_PER_LP) )  {
			ERR_AND_RTN;
		}
	}
	ssd->clear_statistics();
	for( uint32_t i = 0; i < ssd->get_max_lpn(); i++ )
	//for( uint32_t i = 0; i < 12; i++ )
	{// io test
		uint64_t lp_no;
		lp_no = sim_rand64( ssd->get_max_lpn() );
		write_list.push_back(lp_no);

		//printf("%ld\n", lp_no);

		if(!ssd->write( lp_no * FTL_SECTS_PER_LP, FTL_SECTS_PER_LP) )  {
			ERR_AND_RTN;
		}
	}

//	for( auto i : write_list) {
//		if(!ssd->read( i * FTL_SECTS_PER_LP, FTL_SECTS_PER_LP) )  {
//			ERR_AND_RTN;
//		}
//	}

//	printf("successfully finished %ld\n", rw_count);

//	std::ofstream writing_file;
//	writing_file.open("result.txt", std::ios::out);
//	ssd->Dump(writing_file);

	ssd->print_statistics();

	delete ssd;

    return 0;
}
