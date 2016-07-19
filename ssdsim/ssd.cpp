#include "ssd.h"

#include "phy/fm_arch_info.h"
#include "ftl/ftl_lp_info.h"
#include "ftl/ftl_lp_ctl.h"
#include "ftl/ftl_lp_wlctl.h"
#include "ftl/ftl_lp_func.h"


#define SAFE_DELETE(x) {if(x != NULL) delete x; x = NULL;}

SSD::SSD()
{
	fm_info  = NULL;
	ftl_data = NULL;
	ftl_if = NULL;
	ftl_asyn_if = NULL;
}
SSD::~SSD()
{
	SAFE_DELETE(fm_info);
	SAFE_DELETE(ftl_data);
	SAFE_DELETE(ftl_if);
	SAFE_DELETE(ftl_asyn_if);
}

bool SSD::init(uint32_t channel_num, uint32_t pkg_per_ch, uint32_t die_per_pkg, uint64_t byte_per_die, double op_ratio)
{
	fm_info = new FM_INFO();
    ftl_data = new LP_INFO();
    ftl_if = new FtlInterface();
    ftl_asyn_if = new FtlAsynReqInterface();

	uint64_t bytes_per_pkg = die_per_pkg * byte_per_die;

	if( !fm_info->InitFlashModule( channel_num, pkg_per_ch, die_per_pkg, bytes_per_pkg, channel_num, 2) ) {
		ERR_AND_RTN;
	}

	uint64_t usr_area_sector;
	usr_area_sector = fm_info->GetTotalSector() / op_ratio ;
	usr_area_sector += ( (usr_area_sector % SECTS_PER_PP) == 0 ? 0:(SECTS_PER_PP - usr_area_sector % SECTS_PER_PP ) ); // ページで割り切れるように調製

	FTL_EXT_OPT ftl_opt;
	ftl_opt.enable_lp_virtualization= false;
	ftl_opt.enable_rcm_th = false;
	ftl_opt.enable_pg_composition = true;
	ftl_opt.pg_pb_num = 2;
	ftl_opt.pg_parity_num = 0;

	if( !ftl_data->InitFTL( fm_info, usr_area_sector, ftl_if, ftl_asyn_if, &ftl_opt) ) {
		ERR_AND_RTN;
	}
	if( ftl_if->Format() == false ) {
		ERR_AND_RTN;
	}

	max_lba = usr_area_sector;
	return true;
}

bool SSD::write(uint64_t lba, uint32_t len)
{
	uint32_t start_lp, end_lp;
	bool rmw_start, rmw_end;

	HostIoToFMIo(lba, len, &start_lp, &end_lp, &rmw_start, &rmw_end);
	//printf("%ld, %ld\n", start_lp, end_lp );
	for( uint32_t lp_no = start_lp; lp_no < end_lp; lp_no++ )
	{
		if( lp_no == start_lp && rmw_start && !lp_read(lp_no) ) {
			ERR_AND_RTN;
		}

		if(  !lp_write(lp_no, FMT_HOST_WR) ) {
			ERR_AND_RTN;
		}

		if( !do_gc() ) {
			ERR_AND_RTN;
		}
	}

	// for read modify write
	if( start_lp != end_lp && rmw_end && !lp_read(end_lp) &&!lp_write(end_lp, FMT_HOST_WR) ) {
		ERR_AND_RTN;
	}

	return true;
}

bool SSD::read(uint64_t lba, uint32_t len)
{
	uint32_t start_lp, end_lp;
	bool rmw_start, rmw_end;

	HostIoToFMIo(lba, len, &start_lp, &end_lp, &rmw_start, &rmw_end);

	for( uint32_t lp_no = start_lp; lp_no < end_lp; lp_no++ )
	{
		if( !lp_read(lp_no )) return false;
	}
	if( start_lp != end_lp && rmw_end && !lp_read(end_lp) ) {
		ERR_AND_RTN;
	}

	return true;
}

bool SSD::lp_write(uint64_t lp_no, FTL_FMIO_TYPE type)
{
	FTL_PG_GADDR pg;
	pg = ftl_if->GetOpenPG( type, lp_no);
	if( pg == FTL_PG_INVALID_ID ) {
		ERR_AND_RTN;
	}
	if( !ftl_if->L2P_Update( type, lp_no, FTL_SECTS_PER_LP, pg) ) {
		ERR_AND_RTN;
	}
	return true;
}

bool SSD::lp_read(uint64_t lp_no)
{
	return true;
}

bool SSD::do_gc()
{
	int req_num, erase_req_num;
	REVERSE_INFO  req_list[FTL_MAX_REWR_REQ];
	FTL_ERASE_REQ erase_req[FTL_MAX_ERASE_REQ];

	while(1)
	{
		req_num = ftl_asyn_if->GetReWriteReq( req_list );
		if( req_num < 0 ){
			ERR_AND_RTN;
		}

		for( int i = 0; i < req_num; i++ ) {
			uint64_t lp_no = req_list[i].lpn;
			if( !lp_write(lp_no, FMT_RCM_RWR )) {
				ERR_AND_RTN;
			}
		}

		erase_req_num = ftl_asyn_if->GetEraseReq( erase_req );
		if( erase_req_num < 0 ) {
			ERR_AND_RTN;
		}

		for( int i = 0; i < erase_req_num; i++ ) {
			if( ftl_if->ErasePB( erase_req[i] ) == false ) {
				ERR_AND_RTN;
			}
		}

		if( req_num == 0 && erase_req_num == 0 ) {
			break;
		}
	}
	return true;
}

void SSD::Dump( std::ofstream& write_file ) {
	ftl_if->Dump( write_file );
}

CompSSD::CompSSD()
{
	avg_cmp_ratio = 0.5;
	SSD();
}

CompSSD::~CompSSD()
{
}

bool CompSSD::init(uint32_t channel_num, uint32_t pkg_per_ch, uint32_t die_per_pkg, uint64_t byte_per_die, double op_ratio)
{
	fm_info = new FM_INFO();
    ftl_data = new LP_INFO();
    ftl_if = new FtlInterface();
    ftl_asyn_if = new FtlAsynReqInterface();

	uint64_t bytes_per_pkg = die_per_pkg * byte_per_die;

	if( !fm_info->InitFlashModule( channel_num, pkg_per_ch, die_per_pkg, bytes_per_pkg, channel_num, 2) ) {
		ERR_AND_RTN;
	}

	uint64_t usr_area_sector;
	usr_area_sector = fm_info->GetTotalSector() / op_ratio ;
	usr_area_sector += ( (usr_area_sector % SECTS_PER_PP) == 0 ? 0:(SECTS_PER_PP - usr_area_sector % SECTS_PER_PP ) ); // ページで割り切れるように調製

	FTL_EXT_OPT ftl_opt;
	ftl_opt.enable_rcm_th = false;
	ftl_opt.enable_pg_composition = true;
	ftl_opt.pg_pb_num = 32;
	ftl_opt.pg_parity_num = 0;
	ftl_opt.enable_lp_virtualization= true;
	ftl_opt.lp_multiple_rate = 1.0 / avg_cmp_ratio;

	if( !ftl_data->InitFTL( fm_info, usr_area_sector, ftl_if, ftl_asyn_if, &ftl_opt) ) {
		ERR_AND_RTN;
	}
	if( ftl_if->Format(true , avg_cmp_ratio) == false ) {
		ERR_AND_RTN;
	}

	max_lba = ftl_data->lp_num * FTL_SECTS_PER_LP;

	return true;
}


bool CompSSD::setup_cmp_engine(double _avg_cmp_ratio, double _avg_dev)
{
	if( ftl_data != NULL )
		return false;
	avg_cmp_ratio = _avg_cmp_ratio;
	avg_dev = _avg_dev;
	return true;
}

bool CompSSD::lp_write(uint64_t lp_no, FTL_FMIO_TYPE type)
{
	FTL_PG_GADDR pg;
	pg = ftl_if->GetOpenPG( type, lp_no);
	if( pg == FTL_PG_INVALID_ID ) {
		ERR_AND_RTN;
	}

	uint32_t len = FTL_SECTS_PER_LP;

	if( type == FMT_HOST_WR ) {
		len = FTL_SECTS_PER_LP * avg_cmp_ratio;
	}else  {
		len = ftl_data->l2p_tbl[lp_no].len;
	}

	if( !ftl_if->L2P_Update( type, lp_no, len, pg) ) {
		ERR_AND_RTN;
	}
	return true;
}

bool CompSSD::lp_read(uint64_t lp_no)
{
	return true;
}
