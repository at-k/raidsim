// Defines the entry point for the console application.
//

#include <string.h>
#include <stdio.h>
#include <iostream>

#include "ftl/ftl_lp_info.h"
#include "ftl/ftl_lp_ctl.h"
#include "ftl/ftl_lp_wlctl.h"
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
    const char*     conf_file = "testconf.ini";
    const char*     out_dir   = "result";

    bool no_ask = true;

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
                if( strcmp(argv[i], "--conf_file") == 0 || strcmp(argv[i], "-f") ==0 )
                {
                    if( !CheckAdditionalOpt(++i, argc, argv) ) return 0;
                    else conf_file = argv[i];
                }
                else if( strcmp(argv[i], "--out_dir") == 0 || strcmp(argv[i], "-o") ==0 )
                {
                    if( !CheckAdditionalOpt(++i, argc, argv) )	return 0;
                    else out_dir = argv[i];
                }
                else if( strcmp(argv[i], "--no_ask") == 0 || strcmp(argv[i], "-n") ==0 )
                {
                    no_ask = true;
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

    FM_INFO*      fm_info;
    LP_INFO*      ftl_data;
    FtlInterface* ftl_if;
    FtlAsynReqInterface* ftl_asyn_if;

	fm_info = new FM_INFO();
    ftl_data = new LP_INFO();
    ftl_if = new FtlInterface();
    ftl_asyn_if = new FtlAsynReqInterface();
//    ftl_if = new FtlInterfaceWL();
//    ftl_asyn_if = new FtlAsynReqInterfaceWL();

	uint64_t bytes_per_chip = 2;
	bytes_per_chip *= (1024*1024*1024);

	if( !fm_info->InitFlashModule( 2, 2, 2, bytes_per_chip, 2, 2) ) {
		ERR_AND_RTN;
	}

	uint64_t usr_area_sector;
	usr_area_sector = fm_info->GetTotalSector() * 0.8;
	usr_area_sector += ( (usr_area_sector % SECTS_PER_PP) == 0 ? 0:(SECTS_PER_PP - usr_area_sector % SECTS_PER_PP ) ); // ページで割り切れるように調製

	if( !ftl_data->InitFTL( fm_info, usr_area_sector, ftl_if, ftl_asyn_if, NULL) ) {
		ERR_AND_RTN;
	}

	if( ftl_if->Format() == false ) {
		ERR_AND_RTN;
	}
	printf("success format\n");

	std::cout<< ftl_data->lp_num << "\n";

	sim_srand(0);

	for( uint32_t i = 0; i < ftl_data->lp_num; i++ )
	{// io test
		uint64_t lp_no;
		lp_no = sim_rand64( ftl_data->lp_num );

		FTL_PG_GADDR pg;
        pg = ftl_if->GetOpenPG( FMT_HOST_WR, lp_no);
        if( pg == FTL_PG_INVALID_ID ) {
			ERR_AND_RTN;
		}

		if( !ftl_if->L2P_Update( FMT_HOST_WR, lp_no, FTL_SECTS_PER_LP, pg) ) {
			ERR_AND_RTN;
		}

		{ // rcm and erase part
            int req_num, erase_req_num;
            REVERSE_INFO  req_list[FTL_MAX_REWR_REQ];
            FTL_ERASE_REQ erase_req[FTL_MAX_ERASE_REQ];

            while(1)
            {
                req_num = ftl_asyn_if->GetReWriteReq( req_list );
                if( req_num < 0 )
                {
					ERR_AND_RTN;
                }

                for( int i = 0; i < req_num; i++ )
                {
                    pg = ftl_if->GetOpenPG( FMT_RCM_RWR, req_list[i].lpn );
                    if( pg == FTL_PG_INVALID_ID )
                    {
						ERR_AND_RTN;
                    }
                    if( ftl_if->L2P_Update( FMT_RCM_RWR, req_list[i].lpn, req_list[i].len, pg ) == false )
                    {
						ERR_AND_RTN;
                    }
                }

                erase_req_num = ftl_asyn_if->GetEraseReq( erase_req );
                if( erase_req_num < 0 )
                {
					ERR_AND_RTN;
                }

                for( int i = 0; i < erase_req_num; i++ )
                {
                    if( ftl_if->ErasePB( erase_req[i] ) == false )
                    {
						ERR_AND_RTN;
                    }

                }

                if( req_num == 0 && erase_req_num == 0 )
                {
                    break;
                }
            }
        }
	}

	printf("successfully finished\n");

	std::ofstream writing_file;
	writing_file.open("result.txt", std::ios::out);

	ftl_if->Dump(writing_file);

	delete fm_info;
    delete ftl_data;
    delete ftl_if;
    delete ftl_asyn_if ;

    return 0;
}
