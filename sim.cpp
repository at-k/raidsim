#include "sim.h"

#include <time.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <sstream>

#include "common_def.h"

//#include "ftl/ftl_lp_info.h"
//#include "ftl/ftl_lp_ctl.h"
//#include "ftl/ftl_lp_wlctl.h"

//#include "sim_version.h"
#include "inilib/ini.h"
//#include "../util/Logger.h"
#include "iogenerator.h"
#include "iosrc.h"
#include "logger.h"
#include "util_random.h"

//#pragma comment(lib, "Util.lib")
//#pragma comment(lib, "IoGen.lib")
//#pragma comment(lib, "gsl.lib")
//#pragma comment(lib, "cblas.lib")

//#pragma warning( disable : 4996 )

//const char* SimCore::GetVersion()     { return version_info;}
//const char* SimCore::GetVersionShort(){ return version_info_short;}


STATIS_INFO  g_statis;
STATIS_INFO  g_ttl_statis;

#ifndef WIN32
inline uint64_t _atoi64(const char* str)
{
	uint64_t val;
	std::istringstream ss(str);
	if (!(ss >> val))
		std::cout << "failed" << std::endl;
	return val;
}
#endif

SimCore::SimCore()
{
    io_gen = NULL;
    //fm_info   = NULL;
    //ftl_data  = NULL;
    //ftl_if    = NULL;
    //ftl_asyn_if = NULL;

    cum_host_count = 0;
    cum_copy_count = 0;
}


SimCore::~SimCore()
{
    if ( io_gen != NULL )
        delete io_gen;
    //if ( fm_info != NULL )
    //    delete fm_info;
    //if ( ftl_data != NULL )
    //    delete ftl_data;
    //if ( ftl_if != NULL )
    //    delete ftl_if;
    //if ( ftl_asyn_if != NULL )
    //    delete ftl_asyn_if;
}

//-- Initialize
//
bool SimCore::Initialize( const char* conf_file, const char* outdir )
{
    // initialize logger
    if( InitLogger( outdir ) == false )
    {
        printf( HERE "# Intialize Error -- Fail to Init Logger\n");
        return false;
    }

    // initialize statistics
    memset( &g_statis, 0, sizeof( STATIS_INFO ) );
    memset( &g_ttl_statis, 0, sizeof( STATIS_INFO ) );

    AddLogType( LOG_TYPE_ERROR, "w", true );
    AddLogType( LOG_TYPE_RESULT, "w" , true );
    AddLogType( LOG_TYPE_LOG, "w" );
    AddLogType( LOG_TYPE_TRACE, "w" );
    AddLogType( LOG_TYPE_WL, "w" );

    //-- create object
    //if ( io_gen != NULL || fm_info != NULL || ftl_data != NULL )
    //{
    //    PrintMessage( LOG_TYPE_ERROR, "Error at SimCore::Initialize -- double initializing\n" );
    //    return false;
    //}

    //fm_info = new FM_INFO();
    //ftl_data = new LP_INFO();
    //ftl_if = new FtlInterfaceWL();
    //ftl_asyn_if = new FtlAsynReqInterfaceWL();

    // 0クリア
    memset(&sim_info, 0, sizeof(SIM_INFO) );

    //-- size check
    if ( sizeof( uint16_t ) != 2 || sizeof( uint32_t ) != 4 || sizeof( uint64_t ) != 8 ) {
        PrintMessage( LOG_TYPE_ERROR, HERE "Error at SimCore::Initialize -- sizeof check error\n");
        return false;
    }

    //-- open ini file
    std::ifstream fst;

    fst.open( conf_file, std::ios::in );
    if ( !fst.is_open() ) {
        PrintMessage( LOG_TYPE_ERROR, HERE "Error at SimCore::Initialize -- fail to load setting file\n" );
        return false;
    }
    conf_file_name = conf_file;

    //-- load configuration
    ini_document doc;
    doc.fromInputStream(fst);  // iniオブジェクトへファイルの読み込み

    if( doc["CONF_FILE_INFO"]["VERSION"] != CONF_FILE_VERSION ) {
        PrintMessage( LOG_TYPE_ERROR, HERE "Error at SimCore::Initialize -- conffile version missmatch. current version is %s\n", CONF_FILE_VERSION );
        return false;
    }

    std::string sec;   // section...iniファイル上の[xx]←これ
    std::string label; // label
    std::string value; // value

    if( doc["TEST_MODE"]["IO_MODE"] == "TRUE" )
        sim_info.sim_type = IO_TEST_MODE;
    else
        sim_info.sim_type = SIM_MODE;

    //-- FM構造(バス/チップ/ダイ)の設定
    //{
    //    FMADDR arch;

    //    sec = "FM_ARCH_MODEL";
    //    arch.bus = atoi( doc[sec]["BUS_NUM"].c_str() );

    //    arch.chip = atoi( doc[sec]["CHIP_PER_BUS"].c_str() );

    //    arch.die = atoi( doc[sec]["DIE_PER_CHIP"].c_str() );

    //    uint64_t byte_per_chip;
    //    if ( doc[sec]["BYTE_PER_CHIP"] != "" )
    //    {// Byte単位
    //        byte_per_chip = atoi( doc[sec]["BYTE_PER_CHIP"].c_str() );
    //    }
    //    else if ( !doc[sec]["MBYTE_PER_CHIP"].empty() )
    //    {// MByte単位
    //        byte_per_chip = (1024*1024) * atoi( doc[sec]["MBYTE_PER_CHIP"].c_str() );
    //    }
    //    else if ( !doc[sec]["GBYTE_PER_CHIP"].empty() )
    //    {// GByte単位
    //        byte_per_chip =  atoi( doc[sec]["GBYTE_PER_CHIP"].c_str() );
    //        byte_per_chip = byte_per_chip * 1024 * 1024 * 1024;
    //    }

    //    uint16_t dma_num;
    //    dma_num = atoi( doc[sec]["DMA_NUM"].c_str() );

    //    uint16_t ce_per_bus;
    //    ce_per_bus = atoi ( doc[sec]["CE_PER_BUS"].c_str() );

    //    if( fm_info->InitFlashModule( arch.bus, arch.chip, arch.die, byte_per_chip, dma_num, ce_per_bus ) == false )
    //    {// モジュール初期化\n
    //        PrintMessage( LOG_TYPE_ERROR, "Error at SimCore::Initialize -- FMモジュールの初期化失敗\n" );
    //        return false;
    //    }
    //}

    //-- FM制御パラメータ設定
    //FTL_EXT_OPT ftl_option;
    //{
    //    ftl_option.enable_pg_composition = false;
    //    ftl_option.enable_rcm_th = false;

    //    sec = "FTL_OPTION";
    //    if( !doc[sec]["FTL_PARITY"].empty() )
    //    {
    //        uint32_t pos = doc[sec]["FTL_PARITY"].find(":");
    //        if( pos == std::string::npos )
    //        {
    //            PrintMessage( LOG_TYPE_ERROR, "Error at SimCore::Initialize -- PARITY設定記述ミス\n" );
    //            return false;
    //        }

    //        ftl_option.pg_pb_num     = atoi(doc[sec]["FTL_PARITY"].substr(0,pos).c_str());
    //        ftl_option.pg_parity_num = atoi(doc[sec]["FTL_PARITY"].substr(pos+1,doc[sec]["FTL_PARITY"].size() - pos -1 ).c_str());

    //        ftl_option.enable_pg_composition = true;
    //    }

    //    if( !doc[sec]["FTL_RCM_TH"].empty() )
    //    {
    //        ftl_option.rcm_th = atoi(doc[sec]["FTL_RCM_TH"].c_str());
    //        ftl_option.enable_rcm_th = true;
    //    }
    //}

    //-- ユーザエリア領域の設定
    //double   usr_area_ratio;
    uint64_t usr_area_sector;
    {
        // 当面単一LUのみを考える
        sec = "USR_AREA_MODEL";
        //usr_area_ratio  = atof( doc[sec]["USR_AREA_RATIO"].c_str() );

        //-- ユーザ領域のセクタ数を計算
        //usr_area_sector = (uint64_t)(fm_info->GetTotalSector() * usr_area_ratio);
        //usr_area_sector += ( (usr_area_sector % SECTS_PER_PP) == 0 ? 0:(SECTS_PER_PP - usr_area_sector % SECTS_PER_PP ) ); // ページで割り切れるように調製

        //if( ftl_data->InitFTL( fm_info, usr_area_sector, ftl_if, ftl_asyn_if, &ftl_option ) == false )
        //{
        //    PrintMessage( LOG_TYPE_ERROR, "Error at SimCore::Initialize -- FTLデータの初期化失敗\n" );
        //    return false;
        //}

		// test code
		usr_area_sector = 10000;
    }

    //-- IO pattern settings
    {
        sec = "HOST_IO";

        // set io depth
        value = doc[sec]["TAG"];
        if( !value.empty() ) {
            sim_info.io_tag = atoi( value.c_str() );
        } else {
            sim_info.io_tag = 1;
        }

        value = doc[sec]["TYPE"];
        if( value != "NORMAL" ) {
            PrintMessage( LOG_TYPE_ERROR, HERE "Error at SimCore::Initialize -- suppor type is NORMAL only\n" );
            return false;
        }
        else
        {// 汎用I/Oパタン設定
			Generic_IoGenerator* generator = new Generic_IoGenerator();
            generator->InitGenerator( 1, &usr_area_sector );
            this->io_gen = generator;

            IOM_PATTERN io_pattern;       // I/O pattern
            bool        is_random;        // random or sequential
            uint16_t    ratio;            // I/O pattern ratio
            uint16_t    total_ratio;      // must be 100
            char        pattn_name[256];  // name

            total_ratio = 0;

            for( uint32_t i = 0; ; i++ )
            {// loop until total_ratio == 100
                if( total_ratio == 100 )  break;

                sprintf( pattn_name, "A%d", i );  // set pattern name

                if( doc[pattn_name]["TARGET_VOL"].empty() ) {
                    PrintMessage( LOG_TYPE_ERROR, HERE "Error at SimCore::Initialize -- total ratio must be 100\n" );
                    return false;
                }

                io_pattern.tgt_vol   = atoi(doc[pattn_name]["TARGET_VOL"].c_str());
                io_pattern.length    = atoi(doc[pattn_name]["LENGTH"].c_str());
                io_pattern.align     = atoi(doc[pattn_name]["ALIGN_SEC"].c_str());

                if( doc[pattn_name]["START_SEC"].back() == 'R' )
                {// use ratio rule when the end character is `R`
                    std::string tmp = doc[pattn_name]["START_SEC"].erase(doc[pattn_name]["START_SEC"].size()-1 ,1);
                    io_pattern.start_sec = usr_area_sector * atoi( tmp.c_str() ) / 100;
                }else
                    io_pattern.start_sec = atoi(doc[pattn_name]["START_SEC"].c_str());

                if( doc[pattn_name]["END_SEC"].back() == 'R' )
                {// use ratio rule when the end character is `R`
                    std::string tmp = doc[pattn_name]["END_SEC"].erase(doc[pattn_name]["END_SEC"].size() - 1);
                    io_pattern.end_sec = usr_area_sector * atoi( tmp.c_str() ) / 100;
                }else
                    io_pattern.end_sec = atoi(doc[pattn_name]["END_SEC"].c_str());

                // write or read
                if( doc[pattn_name]["OP_CODE"] == "WRITE" )      io_pattern.opcode = IO_WRITE;
                else if( doc[pattn_name]["OP_CODE"] == "READ" )  io_pattern.opcode = IO_READ;
                else {
                    PrintMessage( LOG_TYPE_ERROR, HERE "Error at SimCore::Initialize -- undefined opecode : %s\n", doc[pattn_name]["OP_CODE"].c_str() );
                    return false;
                }

                // rand or seq
                if( doc[pattn_name]["PATTERN"]      == "RAN" ) is_random = true;
                else if( doc[pattn_name]["PATTERN"] == "SEQ" ) is_random = false;
                else {
                    PrintMessage( LOG_TYPE_ERROR, HERE "Error at SimCore::Initialize -- undefined pattern : %s\n", doc[pattn_name]["PATTERN"].c_str() );
                    return false;
                }
                ratio = atoi( doc[pattn_name]["RATIO"].c_str() );

                if( generator->AddIoPattern( &io_pattern, is_random, ratio ) == false ) {
                    PrintMessage( LOG_TYPE_ERROR, HERE "Error at SimCore::Initialize -- fail to add pattern\n" );
                    return false;
                }

                total_ratio += ratio; // ratioの更新
            }
        }
    }

    //-- output controll
    {
        sec = "RESULT_FORMAT";

        // output triger
        value = doc[sec]["OUTPUT_TRIGER"];

        if( !value.empty() )
        {
            if( value == "KILO_IO" )      sim_info.out_type = OTT_IO_KILO_COUNT;
            //else if( value == "TIME_MS" ) sim_info.out_type = OTT_SIM_TIME_MS;
            else
            {// TIME is now invalid
                PrintMessage( LOG_TYPE_ERROR, HERE "Error at SimCore::Initialize -- invalid option %s\n", value.c_str() );
            }
        }
        else
        {// load default
            sim_info.out_type = OTT_IO_KILO_COUNT;
        }

        // output interval
        value = doc[sec]["OUTPUT_INTERVAL"];

        if( !value.empty() )
            sim_info.out_interval = std::stoull(value) * 1000;
        else
            sim_info.out_interval = 10 * 1000; // 10,000 IO

        // simulation end
        value = doc[sec]["TOTAL_COUNT"];
        if( !value.empty() )
            sim_info.end_count = _atoi64( value.c_str() ) * 1000;
        else
            sim_info.end_count = 1000 * 1000; // 1sec or 1M IO
    }

    //-- for debug
    {
        sec = "DEBUG";

        value = doc[sec]["RAND_SEED"];
        if( value.empty() )
            sim_info.rand_seed = (uint32_t)time( NULL );
        else
            sim_info.rand_seed = atoi( value.c_str() );

        sim_srand( sim_info.rand_seed );

        sim_info.trace_on_io_count = sim_info.trace_off_io_count = 0;

        if( !doc[sec]["TRACE_ON"].empty() )
            sim_info.trace_on_io_count = _atoi64( doc[sec]["TRACE_ON"].c_str() );

        if( !doc[sec]["TRACE_OFF"].empty() )
            sim_info.trace_off_io_count = _atoi64( doc[sec]["TRACE_OFF"].c_str() );

        if( sim_info.trace_on_io_count > sim_info.trace_off_io_count )
            sim_info.trace_on_io_count = sim_info.trace_off_io_count = 0;
    }

    is_sim_end = false;

    {// format.. write sequentially to whole area
        //if( ftl_if->Format() == false ) {
        //    PrintMessage( LOG_TYPE_ERROR, "Error at SimCore::Initialize -- FM領域フォーマット失敗\n" );
        //    return false;
        //}
    }

    // ヘッダ出力
    PrintHeader();

    return true;
}

void SimCore::Close()
{
    PrintFinalResult();
    CloseLogger();
}

void SimCore::ResetEndCounter()
{
    sim_info.sim_time = 0;
    sim_info.io_count = 0;
}

void SimCore::PrintHeader()
{
//    PrintMessage( LOG_TYPE_RESULT, "#-------------------------------------------------------------------------------------#\n");
//    PrintMessage( LOG_TYPE_RESULT, "#.ConfFile...%s\n", conf_file_name.c_str() );
//    PrintMessage( LOG_TYPE_RESULT, "#.TotalFmPage...%d.Page,TotalUsrPage...%d.Page\n", ftl_data->pp_num, ftl_data->lp_num );
//    PrintMessage( LOG_TYPE_RESULT, "#.OP_Ratio...%.1f,REAL_OP_RATIO...%.1f,%dD%dP\n", ftl_data->op_ratio, ftl_data->real_op_ratio, (ftl_data->pg_pb_num-ftl_data->pg_parity_num), ftl_data->pg_parity_num );
//    PrintMessage( LOG_TYPE_RESULT, "#.DMA_NUM...%d,BUS_NUM...%d,CE_NUM_PER_BUS...%d,BUS_NUM_PER_DMA...%d,PACKAGE_SIZE(GB)...%d\n",
//        fm_info->GetDMANum(), fm_info->GetBusNum(), fm_info->GetCeNumPerBus(), fm_info->GetBusNumPerCe(), fm_info->GetFMSizeGB() );
//    PrintMessage( LOG_TYPE_RESULT, "#.RAND_SEED.....%d\n", sim_info.rand_seed );
//    PrintMessage( LOG_TYPE_RESULT, "#-------------------------------------------------------------------------------------#\n");
//
//    // 1.
//    //PrintMessage( LOG_TYPE_RESULT, "Time(us),TotalIO(kIO),Write,Read,WriteSect,ReadSect,\t,");
//    PrintMessage( LOG_TYPE_RESULT, "IOCount,Write,Read,WriteSect,ReadSect,\t,");
//    // 2.
//    PrintMessage( LOG_TYPE_RESULT, "FMIO,FMPageWrite,FMPageRead,ErasePB,AssignedPB(Host),AssignedPB(RC),\t,");
//    // 3.
//    PrintMessage( LOG_TYPE_RESULT, "AveEC,MaxEC,MinEC,SumEC,SumFB,SumIB,\t,");
//    // 4.
//    PrintMessage( LOG_TYPE_RESULT, "WA(L-P),WA(H-F),\t,");
//    // 5.
//    /*PrintMessage( LOG_TYPE_RESULT, "WorstRespTimeWrite,AveRespTimeWrite,\t," );
//    PrintMessage( LOG_TYPE_RESULT, "WorstRespTimeRead,AveRespTimeRead,\t," );
//    // 6.
//    PrintMessage( LOG_TYPE_RESULT, "MBPS(Write),MBPS(Read),IOPS(Write),IOPS(Read),\t,");
//    PrintMessage( LOG_TYPE_RESULT, "MBPS(BE-Write),MBPS(BE-Read),\t,");
//    // 7.
//    PrintMessage( LOG_TYPE_RESULT, "BusUtilRatio,CeUtilRatio,CeMultiCount,CeMulti(RCM)\t,");
//    // 8.
//    PrintMessage( LOG_TYPE_RESULT, "CheckSum(forDebug)");*/
//
//    // end
//    PrintMessage( LOG_TYPE_RESULT, "\n" );
}

void SimCore::PrintMidResult()
{
    // 時間更新
    //uint64_t elapsed_time = g_statis.cur_sim_time;

    // g_statisは間隔の値，g_ttl_statisは累計値
    g_ttl_statis.cur_sim_time  += g_statis.cur_sim_time;
    sim_info.sim_time = g_ttl_statis.cur_sim_time;

    g_ttl_statis.host_io_count += g_statis.host_io_count;
    g_ttl_statis.host_rd_count += g_statis.host_rd_count;
    g_ttl_statis.host_wr_count += g_statis.host_wr_count;
    g_ttl_statis.host_rd_sect  += g_statis.host_rd_sect;
    g_ttl_statis.host_wr_sect  += g_statis.host_wr_sect;

    /*g_ttl_statis.host_io_comp_count += g_statis.host_io_comp_count;
    g_ttl_statis.host_rd_comp_count += g_statis.host_rd_comp_count;
    g_ttl_statis.host_wr_comp_count += g_statis.host_wr_comp_count;
    g_ttl_statis.host_io_resp_time  += g_statis.host_io_resp_time;
    g_ttl_statis.host_rd_resp_time  += g_statis.host_rd_resp_time;
    g_ttl_statis.host_wr_resp_time  += g_statis.host_wr_resp_time;

    g_ttl_statis.host_io_worst_resp_time = g_statis.host_io_worst_resp_time;
    g_ttl_statis.host_rd_worst_resp_time = g_statis.host_rd_worst_resp_time;
    g_ttl_statis.host_wr_worst_resp_time = g_statis.host_wr_worst_resp_time;*/

    g_ttl_statis.fm_io_count        += g_statis.fm_io_count;
    g_ttl_statis.fm_page_rd_count   += g_statis.fm_page_rd_count;
    g_ttl_statis.fm_page_wr_count   += g_statis.fm_page_wr_count;
    g_ttl_statis.fm_page_copy_count += g_statis.fm_page_copy_count;
    g_ttl_statis.fm_erase_count     += g_statis.fm_erase_count;

    g_ttl_statis.fm_host_fb_cons_count += g_statis.fm_host_fb_cons_count;
    g_ttl_statis.fm_rc_fb_cons_count   += g_statis.fm_rc_fb_cons_count;

    g_ttl_statis.fb_count = g_statis.fb_count;
    g_ttl_statis.ib_count = g_statis.ib_count;

    /*
    g_ttl_statis.ce_util_time += g_statis.ce_util_time;
    g_ttl_statis.bus_util_time += g_statis.bus_util_time;
    g_ttl_statis.ce_multi_count += g_statis.ce_multi_count;
    g_ttl_statis.rewrite_ce_multi_count += g_statis.rewrite_ce_multi_count;
    */

    // 結果出力
    // 1.
    PrintMessage( LOG_TYPE_RESULT, "%lld,%lld,%lld,%lld,%lld,\t,",
                  g_statis.host_io_count/1000,
                  g_statis.host_wr_count, g_statis.host_rd_count,
                  g_statis.host_wr_sect,  g_statis.host_rd_sect );
    // 2.
    PrintMessage( LOG_TYPE_RESULT, "%lld,%lld,%lld,%lld,%lld,%lld,\t,",
                  g_statis.fm_io_count,
                  g_statis.fm_page_wr_count,           g_statis.fm_page_rd_count,
                  g_statis.fm_erase_count,
                  g_statis.fm_host_fb_cons_count,  g_statis.fm_rc_fb_cons_count );
    // 3.
    PrintMessage( LOG_TYPE_RESULT, "-,-,-,%lld,%lld,%lld,\t,",
                  g_statis.fm_erase_count,
                  g_statis.fb_count,    g_statis.ib_count);
    // 4.
    //double wa_lp   = 0;
    //double wa_htof = 0;
    //if( g_statis.fm_page_wr_count != 0  )
    //{
    //    wa_lp = (double)( (long double)(g_statis.fm_page_wr_count) /
    //                      (long double)(g_statis.fm_page_wr_count - g_statis.fm_page_copy_count) );
    //}
    //if( g_statis.host_wr_sect != 0 )
    //{
    //    wa_htof = (double)( (long double)(g_statis.fm_page_wr_count * SECTS_PER_PP) /
    //                        (long double)(g_statis.host_wr_sect) );
    //}

    //PrintMessage( LOG_TYPE_RESULT, "%.2f,%.2f,\t,", wa_lp, wa_htof );
    /*
    // 5.
    uint64_t wr_resp = 0;
    uint64_t rd_resp = 0;
    if( g_statis.host_wr_count != 0 )
    {
        wr_resp = g_statis.host_wr_resp_time / g_statis.host_wr_count;
    }

    if( g_statis.host_rd_count != 0 )
    {
        rd_resp = g_statis.host_rd_resp_time / g_statis.host_rd_count;
    }
    PrintMessage( LOG_TYPE_RESULT, "%lld,%lld,\t,", g_statis.host_wr_worst_resp_time, wr_resp );
    PrintMessage( LOG_TYPE_RESULT, "%lld,%lld,\t,", g_statis.host_rd_worst_resp_time, rd_resp );
    // 6.
    double MBPS_rd, MBPS_wr;
    double IOPS_rd, IOPS_wr;
    double MBPS_rd_back, MBPS_wr_back;
    if( elapsed_time != 0)
    {
        MBPS_wr = (double)((long double)SECTOR2BYTE(g_statis.host_wr_sect)/(long double)elapsed_time);
        MBPS_rd = (double)((long double)SECTOR2BYTE(g_statis.host_rd_sect)/(long double)elapsed_time);

        IOPS_wr = (double)((long double)(g_statis.host_wr_count*1000*1000) / (long double)elapsed_time);
        IOPS_rd = (double)((long double)(g_statis.host_rd_count*1000*1000) / (long double)elapsed_time);
        // host
        PrintMessage( LOG_TYPE_RESULT, "%.2f,%.2f,%.2f,%.2f,\t,",
                      MBPS_wr, MBPS_rd, IOPS_wr, IOPS_rd);
        // backend
        MBPS_rd_back = (double)((long double)(BYTES_PER_PP*g_statis.fm_page_rd_count) / (long double)elapsed_time);
        MBPS_wr_back = (double)((long double)(BYTES_PER_PP*g_statis.fm_page_wr_count) / (long double)elapsed_time);

        PrintMessage( LOG_TYPE_RESULT, " %.2f,%.2f,\t,",
                      MBPS_wr_back, MBPS_rd_back);
    }
    // 7.
    double ce_util_ratio  = 0;
    double bus_util_ratio = 0;
    if( g_statis.ce_util_time != 0 )
    {
        ce_util_ratio = (double)( (long double)(100*g_statis.ce_util_time) / (long double)(elapsed_time) );
        ce_util_ratio = ce_util_ratio / (double)fm_info->GetTotalCENum();
    }
    if( g_statis.bus_util_time != 0 )
    {
        bus_util_ratio = (double)( (long double)(100*g_statis.bus_util_time) / (long double)(elapsed_time) );
        bus_util_ratio = bus_util_ratio / (double)fm_info->GetBusNum();
    }
    PrintMessage( LOG_TYPE_RESULT, "%.2f,%.2f,%d,%d,\t,",
                  bus_util_ratio, ce_util_ratio, g_statis.ce_multi_count, g_statis.rewrite_ce_multi_count );

    // ext.
    //PrintMessage( LOG_TYPE_RESULT, "%d,%d,", ftl_data->l2p_info.pool_list[0].initial_vp_count, ftl_data->l2p_info.pool_list[0].rcm_tgt_pb_num);

    // 8.
    PrintMessage( LOG_TYPE_RESULT, "-");
    */

    // end
    PrintMessage( LOG_TYPE_RESULT, "\n" );

    // 引き継ぎ情報以外はリセット
    memset( &g_statis, 0, sizeof( STATIS_INFO ) );

    //g_statis.cur_sim_time      = g_ttl_statis.cur_sim_time;
    //g_statis.pre_sim_time      = g_ttl_statis.cur_sim_time;
    //g_statis.host_io_count     = g_ttl_statis.host_io_count;
    //g_statis.pre_host_io_count = g_ttl_statis.host_io_count;
    g_statis.fb_count          = g_ttl_statis.fb_count;
    g_statis.ib_count          = g_ttl_statis.ib_count;

    return;
}

void SimCore::PrintFinalResult()
{
    PrintMidResult();
    PrintMessage( LOG_TYPE_RESULT, "#SimulationEnd\n");

    double wa_lp   = 0;
    if( cum_host_count != 0 && cum_copy_count != 0 )
    {
        wa_lp = (double)( (long double)(cum_host_count + cum_copy_count) /
                          (long double)(cum_host_count) );
    }

    PrintMessage( LOG_TYPE_RESULT, "%lld,%lld,%.2f\n",cum_host_count, cum_copy_count, wa_lp );
}

void SimCore::RunStep()
{
    /*{
        PrintMessage( LOG_TYPE_RESULT, "テストモード。" );
        FtlTestFunc( ftl_data, fm_info );
        //g_is_trace_on = false;
        return;
    }*/

    CommandInfo next_io;

    // 時間は現バージョンでは無効
    // g_statis.cur_sim_time = scheduler.GetCurTime();

    // 詳細トレース取得ON/OFF切り替え
    if( sim_info.trace_on_io_count != sim_info.trace_off_io_count )
    {
        if( g_ttl_statis.host_io_count > sim_info.trace_off_io_count )
        {
            g_is_trace_on = false;
        }
        else if( g_ttl_statis.host_io_count >= sim_info.trace_on_io_count )
        {
            g_is_trace_on = true;
        }
    }

    if( (sim_info.out_type == OTT_IO_KILO_COUNT && sim_info.io_count < sim_info.end_count) )
        //|| (sim_info.out_type == OTT_SIM_TIME_MS && sim_info.sim_time < sim_info.end_count) )
    {// シミュレーション条件の最後まではIOを生成する。
        if( io_gen->GetNextCommand( &next_io ) == false )
        {
            PrintMessage( LOG_TYPE_ERROR, HERE "Error at SimCore::RunStep -- fail to get next io\n" );
            is_sim_end = true;
            return;
        }
//		else if( next_io.sector_num != FTL_SECTS_PER_LP || next_io.lba % FTL_SECTS_PER_LP != 0 )
//        {
//            PrintMessage( LOG_TYPE_ERROR, "Error at SimCore::RunStep -- IO種類制限中\n" );
//            is_sim_end = true;
//            return;
//        }

//        uint32_t start_lp, end_lp;
//        bool     start_rmw_flag, end_rmw_flag;
//        HostIoToFMIo( next_io.lba, next_io.sector_num, &start_lp, &end_lp, &start_rmw_flag, &end_rmw_flag );
//
//        if( start_lp + 1  != end_lp )
//        {// 今のところ8kBライト以外はアウト。
//            PrintMessage( LOG_TYPE_ERROR, "Error at SimCore::RunStep -- 未サポートIOパタン\n");
//            is_sim_end = true;
//            return;
//        }
//
        sim_info.io_count ++;
//        g_statis.host_io_count++;
//        g_statis.fm_io_count++;
//        g_statis.fm_page_wr_count++;
//
//        if( cum_copy_count != 0 )
//            cum_host_count ++;
//
//        /* io part */
//        FTL_PG_GADDR pg;
//        pg = ftl_if->GetOpenPG( FMT_HOST_WR, start_lp );
//        if( pg == FTL_PG_INVALID_ID )
//        {
//            PrintMessage( LOG_TYPE_ERROR, "Error at SimCore::RunStep -- runtime 1 \n" );
//            is_sim_end = true;
//            return;
//        }
//        if( ftl_if->L2P_Update( FMT_HOST_WR, start_lp, FTL_SECTS_PER_LP, pg ) == false )
//        {
//            PrintMessage( LOG_TYPE_ERROR, "Error at SimCore::RunStep -- runtime 2 \n" );
//            is_sim_end = true;
//            return;
//        }
//
//        { // rcm and erase part
//            int req_num, erase_req_num;
//            REVERSE_INFO  req_list[FTL_MAX_REWR_REQ];
//            FTL_ERASE_REQ erase_req[FTL_MAX_ERASE_REQ];
//
//            while(1)
//            {
//                req_num = ftl_asyn_if->GetReWriteReq( req_list );
//                if( req_num < 0 )
//                {
//                    is_sim_end = true;
//                    return;
//                }
//
//                for( int i = 0; i < req_num; i++ )
//                {
//                    pg = ftl_if->GetOpenPG( FMT_RCM_RWR, req_list[i].lpn );
//                    if( pg == FTL_PG_INVALID_ID )
//                    {
//                        PrintMessage( LOG_TYPE_ERROR, "Error at SimCore::RunStep -- runtime 3 \n" );
//                        is_sim_end = true;
//                        return;
//                    }
//                    if( ftl_if->L2P_Update( FMT_RCM_RWR, req_list[i].lpn, req_list[i].len, pg ) == false )
//                    {
//                        PrintMessage( LOG_TYPE_ERROR, "Error at SimCore::RunStep -- runtime 4 \n" );
//                        is_sim_end = true;
//                        return;
//                    }
//
//                    g_statis.fm_io_count++;
//                    g_statis.fm_page_wr_count++;
//                    g_statis.fm_page_copy_count++;
//
//                    cum_copy_count++;
//                }
//
//                erase_req_num = ftl_asyn_if->GetEraseReq( erase_req );
//                if( erase_req_num < 0 )
//                {
//                    is_sim_end = true;
//                    return;
//                }
//
//                for( int i = 0; i < erase_req_num; i++ )
//                {
//                    if( ftl_if->ErasePB( erase_req[i] ) == false )
//                    {
//                        PrintMessage( LOG_TYPE_ERROR, "Error at SimCore::RunStep -- runtime 5 \n" );
//                        is_sim_end = true;
//                        return;
//                    }
//
//                    g_statis.fm_erase_count++;
//                }
//
//                if( req_num == 0 && erase_req_num == 0 )
//                {
//                    break;
//                }
//            }
//
//            g_statis.fb_count = ftl_data->pool.total_free_pb_count;
//            g_statis.ib_count = ftl_data->pool.total_invalid_pb_count;
//        }
    }
    else
    {
        is_sim_end = true;
    }

    /*if( scheduler.MainProc() == false )
    {
        PrintMessage( LOG_TYPE_ERROR, "Error at SimCore::RunStep -- スケジューラメインプロシージャ失敗\n" );
        is_sim_end = true;
        return;
    }*/

    // 出力。
    if( (sim_info.out_type == OTT_IO_KILO_COUNT &&
         g_statis.host_io_count >= sim_info.out_interval ) )
//        ||(sim_info.out_type == OTT_SIM_TIME_MS &&
//         g_statis.host_io_count >= sim_info.out_interval ))
    {

        PrintMidResult();
    }

    {
        static uint32_t count = 0;

        count++;

//        if( count > ftl_data->lp_num * 10 )
//        {// 10周に１回記録
//            ftl_if->Dump( LOG_TYPE_WL );
//            count = 0;
//        }
    }

    return;
}

