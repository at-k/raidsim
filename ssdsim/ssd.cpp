#include <string.h>
#include <iostream>

#include <string>
#include <fstream>
#include <sstream>

#include "ssd.h"

#include "phy/fm_arch_info.h"
#include "ftl/ftl_lp_info.h"
#include "ftl/ftl_lp_ctl.h"
#include "ftl/ftl_lp_wlctl.h"
#include "ftl/ftl_lp_func.h"


#define SAFE_DELETE(x) {if(x != NULL) delete x; x = NULL;}

SSD::SSD()
{
	memset(&stats, 0, sizeof(SSD_STATISTICS));
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

bool SSD::simple_init(uint64_t ttl_phy_bytes, double _op_ratio)
{
	const uint32_t def_channel_num = 4;
	const uint32_t def_pkgnum_per_ch = 2;
	const uint32_t def_dienum_per_pkg = 4;

	op_ratio = _op_ratio;

	uint64_t die_num = def_channel_num * def_pkgnum_per_ch * def_dienum_per_pkg;
	if( ttl_phy_bytes % die_num != 0 ) {
		ERR_AND_RTN;
	}
	return init(def_channel_num, def_pkgnum_per_ch, def_dienum_per_pkg, ttl_phy_bytes / die_num, op_ratio);
}

bool SSD::init(uint32_t channel_num, uint32_t pkg_per_ch, uint32_t die_per_pkg, uint64_t byte_per_die, double _op_ratio)
{
	fm_info = new FM_INFO();
    ftl_data = new LP_INFO();
    ftl_if = new FtlInterface();
    ftl_asyn_if = new FtlAsynReqInterface();

	op_ratio = _op_ratio;

	uint64_t bytes_per_pkg = die_per_pkg * byte_per_die;
	//printf("hoge, %d, %d, %d, %ld\n", channel_num, pkg_per_ch, die_per_pkg, byte_per_die);
	if( !fm_info->InitFlashModule( channel_num, pkg_per_ch, die_per_pkg, bytes_per_pkg, channel_num, 2) ) {
		ERR_AND_RTN;
	}

	uint64_t usr_area_sector;
	usr_area_sector = fm_info->GetTotalSector() / op_ratio ;
	usr_area_sector += ( (usr_area_sector % SECTS_PER_PP) == 0 ? 0:(SECTS_PER_PP - usr_area_sector % SECTS_PER_PP ) ); // ページで割り切れるように調製

	phy_sects = fm_info->GetTotalSector();

	FTL_EXT_OPT ftl_opt;
	ftl_opt.enable_lp_virtualization= false;
	ftl_opt.enable_rcm_th = false;
	ftl_opt.enable_pg_composition = true;
	ftl_opt.pg_pb_num = 1;
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
		if( lp_no == start_lp && rmw_start && !lp_read(lp_no, FMT_HOST_WR) ) {
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
	if( start_lp != end_lp && rmw_end ) {
		if( !lp_read(end_lp, FMT_HOST_WR) &&!lp_write(end_lp, FMT_HOST_WR) ) {
			ERR_AND_RTN;
		}
	}

	stats.hcmd_wr_sect += len;

	return true;
}

bool SSD::read(uint64_t lba, uint32_t len)
{
	uint32_t start_lp, end_lp;
	bool rmw_start, rmw_end;

	HostIoToFMIo(lba, len, &start_lp, &end_lp, &rmw_start, &rmw_end);

	for( uint32_t lp_no = start_lp; lp_no < end_lp; lp_no++ )
	{
		if( !lp_read(lp_no, FMT_HOST_RD )) return false;
	}
	if( start_lp != end_lp && rmw_end && !lp_read(end_lp, FMT_HOST_RD) ) {
		ERR_AND_RTN;
	}

	stats.hcmd_rd_sect += len;

	return true;
}

bool SSD::lp_write(uint64_t lp_no, FTL_FMIO_TYPE type)
{
	FTL_PG_GADDR pg;
	pg = ftl_if->GetOpenPG( type, lp_no);
	if( pg == FTL_PG_INVALID_ID ) {
		ERR_AND_RTN;
	}
	uint32_t written_size = 0;
	if( !ftl_if->L2P_Update( type, lp_no, FTL_SECTS_PER_LP, pg, written_size) ) {
		ERR_AND_RTN;
	}

	stats.fm_wr_count++;

	stats.fm_wr_sect += written_size;
	if( type == FMT_RCM_RWR ) {
		stats.fm_gc_wr_count++;
		stats.fm_gc_wr_sect += written_size;
	}

	return true;
}

bool SSD::lp_read(uint64_t lp_no, FTL_FMIO_TYPE type)
{
	stats.fm_rd_count++;
	stats.fm_rd_sect += FTL_SECTS_PER_LP;
	if( type == FMT_RCM_RWR ) {
		stats.fm_gc_rd_count++;
		stats.fm_gc_rd_sect += FTL_SECTS_PER_LP;
	}

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

		//std::cout << ftl_data->pool.total_free_pb_count <<","<<ftl_data->pool.total_invalid_pb_count << std::endl;
		if( req_num != 0 )
			//std::cout<< "req:"<< req_num <<",free:" << ftl_data->pool.total_free_pb_count << std::endl;
		for( int i = 0; i < req_num; i++ ) {
			uint64_t lp_no = req_list[i].lpn;
			if( !lp_read(lp_no, FMT_RCM_RWR )) {
				ERR_AND_RTN;
			}

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
			stats.fm_pb_erase ++;
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

void SSD::print_statistics() {
	std::cout << "#ssd statistics, max_lba, phy_sects, op_ratio, hcmd_wr_sect, hcmd_rd_sect, fm_wr_sect, fm_gc_wr_sect, fm_rd_sect, fm_gc_rd_sect, fm_gc_wr_count, fm_gc_rd_count, wa, ra"  << std::endl;
	std::cout << "," << max_lba << "," << fm_info->GetTotalSector() << "," << op_ratio << "," << stats.hcmd_wr_sect << ","  << stats.hcmd_rd_sect << ","
		<< stats.fm_wr_sect << "," << stats.fm_gc_wr_sect << "," << stats.fm_rd_sect << "," << stats.fm_gc_rd_sect << "," << stats.fm_gc_wr_count << ","
		<< stats.fm_gc_rd_count << "," <<
		(stats.hcmd_wr_sect != 0 ? ((double)stats.fm_wr_sect / stats.hcmd_wr_sect):0) << "," <<
		(stats.hcmd_rd_sect != 0 ? ((double)stats.fm_rd_sect / stats.hcmd_rd_sect):0) << std::endl;

}

void SSD::clear_statistics(){
	memset(&stats, 0, sizeof(SSD_STATISTICS));
}
CompEngine::CompEngine(){
	data_list.clear();
}

CompEngine::~CompEngine(){
	data_list.clear();
}

bool CompEngine::init_engine(const char* trace_file_name)
{
	std::ifstream reading_file;
    reading_file.open(trace_file_name, std::ios::in);
	if( !reading_file ) {
		ERR_AND_RTN;
	}

	// load header
	std::string line_buffer;
	std::getline(reading_file, line_buffer);
	std::istringstream ss(line_buffer);
	std::string token;
	while(std::getline(ss, token, ',') ) {
		size_t pos = token.find("bs");
		if( pos != std::string::npos ) {
			std::istringstream is(token);
			std::string mem;
			std::getline(is, mem, '=');
			std::getline(is, mem, '=');
			org_chunk_size = atoi(mem.c_str());
			break;
		}
	}

	//std::cout << org_chunk_size << std::endl;
	uint32_t sum = 0;

	while(!reading_file.eof()) {
		std::getline(reading_file, line_buffer);
		uint32_t val = atoi(line_buffer.c_str());
		if( val == 0 ) continue;
		//std::cout << val << std::endl;
		data_list.push_back(val);
		sum += val;
	}

	avg_comp_size = sum / data_list.size();
	avg_comp_ratio = (double)avg_comp_size / org_chunk_size;

	cur_pointer = 0;

	return true;
}

bool CompEngine::init_engine(double _avg_comp_ratio) {
	if( !data_list.empty() )
		return false;

	avg_comp_ratio = _avg_comp_ratio;
	return true;
}

uint32_t CompEngine::get_next_length()
{
	if( data_list.empty() ) {
		return avg_comp_ratio * org_chunk_size;
	}
	uint32_t ret_val = data_list[cur_pointer];
	cur_pointer++;
	if( cur_pointer == data_list.size() ) cur_pointer = 0;
	return ret_val;
}

double CompEngine::get_next_ratio()
{
	if( data_list.empty() ) {
		return avg_comp_ratio;
	}
	double ret_val = (double)data_list[cur_pointer] / org_chunk_size;
	cur_pointer++;
	if( cur_pointer == data_list.size() ) cur_pointer = 0;
	return ret_val;
}

CompSSD::CompSSD()
{
	avg_cmp_ratio = 0.5;
	cmp_engine = NULL;
	enable_virtualization = NULL;
	buffered_page_num = 4;
	SSD();
}

CompSSD::~CompSSD()
{
}

void CompSSD::print_statistics() {
	SSD::print_statistics();
//	for( auto p : read_penalty) {
//		std::cout << p << std::endl;
//	}
}

bool CompSSD::init(uint32_t channel_num, uint32_t pkg_per_ch, uint32_t die_per_pkg, uint64_t byte_per_die, double _op_ratio )
{
	fm_info = new FM_INFO();
    ftl_data = new LP_INFO();
    ftl_if = new FtlInterface();
    ftl_asyn_if = new FtlAsynReqInterface();

	op_ratio = _op_ratio;
	uint64_t bytes_per_pkg = die_per_pkg * byte_per_die;

	if( !fm_info->InitFlashModule( channel_num, pkg_per_ch, die_per_pkg, bytes_per_pkg, channel_num, 2) ) {
		ERR_AND_RTN;
	}

	uint64_t usr_area_sector;
	usr_area_sector = fm_info->GetTotalSector() / op_ratio ;
	usr_area_sector += ( (usr_area_sector % SECTS_PER_PP) == 0 ? 0:(SECTS_PER_PP - usr_area_sector % SECTS_PER_PP ) ); // ページで割り切れるように調製

	phy_sects = fm_info->GetTotalSector();

	if( cmp_engine == NULL )
		ERR_AND_RTN;

	avg_cmp_ratio = cmp_engine->get_avg_cmp_ratio();

	FTL_EXT_OPT ftl_opt;
	ftl_opt.enable_rcm_th = false;
	ftl_opt.enable_pg_composition = true;
	ftl_opt.pg_pb_num = 8;
	ftl_opt.pg_parity_num = 0;
	if( !enable_virtualization ) {
		ftl_opt.enable_lp_virtualization= false;
	}else {
		ftl_opt.enable_lp_virtualization= true;
		ftl_opt.lp_multiple_rate = 1.0 / avg_cmp_ratio;
	}

	if( !ftl_data->InitFTL( fm_info, usr_area_sector, ftl_if, ftl_asyn_if, &ftl_opt) ) {
		ERR_AND_RTN;
	}
	//std::cout << avg_cmp_ratio << std::endl;
	if( ftl_if->Format(true , avg_cmp_ratio) == false ) {
		ERR_AND_RTN;
	}

	max_lba = ftl_data->lp_num * FTL_SECTS_PER_LP;

	write_buffer.clear();
	rcm_write_buffer.clear();
	rcm_read_buffer.clear();

	read_penalty.clear();
	read_penalty.resize(ftl_data->lp_num);
	for( auto &p : read_penalty )
		p = 0;

	//std::cout << max_lba <<"," <<  phy_sects << std::endl;

	return true;
}


bool CompSSD::setup_compression(bool _enable_virtualization, double _avg_cmp_ratio, double _avg_dev,
		ALIGN_TYPE a_type, uint32_t b_page_num)
{
	if( ftl_data != NULL )
		return false;
	enable_virtualization = _enable_virtualization;
	avg_cmp_ratio = _avg_cmp_ratio;
	avg_dev = _avg_dev;
	align_type = a_type;
	buffered_page_num = b_page_num;
	return true;
}

bool CompSSD::lp_write(uint64_t lp_no, FTL_FMIO_TYPE type)
{
	// 修正予定
	// HOST_WRITEの場合
	//  CaFTLであれば、Nページ目のライトは物理ページアライメントに揃える。
	//  かつ、そのNページを、どうにかしてまとめて、格納した物理ページ数に対応付ける。
	//  CCFTLであれば、特に何も。
	// RCM_WRITEの場合
	//  lengthはそのまま。アライメントに関しては上と同じ。
	//FTL_PG_GADDR pg;

	stats.fm_wr_count++;
	if( type == FMT_HOST_WR) {
		write_buffer.push_back(lp_no);
		if( write_buffer.size() == buffered_page_num) {
			if( !buffered_data_write( type, write_buffer ) ) {
				ERR_AND_RTN;
			}
		}
	} else {
		stats.fm_gc_wr_count++;
		rcm_write_buffer.push_back(lp_no);
		if( rcm_write_buffer.size() == buffered_page_num) {
			if( !buffered_data_write( type, rcm_write_buffer )) {
				ERR_AND_RTN;
			}
		}
	}
//	}

	return true;
}

bool CompSSD::lp_read(uint64_t lp_no, FTL_FMIO_TYPE type)
{
	// 修正予定
	// HOST_READの場合
	//  CCFTLの場合
	//   物理ページ跨がりの場合2ページリード
	//   そうでない場合は1ページリード
	//  CaFTLの場合
	//   圧縮データの
	// RCM_READの場合
	//  複数の要求をためて、物理ページリードをまとめる
	stats.fm_rd_count++;
	if( type == FMT_HOST_RD ) {
		stats.fm_rd_sect += read_penalty[lp_no];
//		std::cout << "rd (" << lp_no << "," << read_penalty[lp_no] << ")" << std::endl;
	} else {
		stats.fm_gc_rd_count++;
		rcm_read_buffer.push_back(lp_no);
		if( rcm_read_buffer.size() >= 10 )
		{
			// read req for rcm dispatched at the same time
			VPA_INFO vp_start = ftl_data->l2p_tbl[rcm_read_buffer.front()];;
			VPA_INFO vp_end = vp_start;
			uint32_t pp_num = 0;
			for( uint32_t i = 1; i < rcm_read_buffer.size(); i++ ) {
				VPA_INFO vp = ftl_data->l2p_tbl[rcm_read_buffer[i]];
				if( vp_end.pgn == vp.pgn && ((vp_end.ofs+vp_end.len)/SECTS_PER_PP) == (vp.ofs/SECTS_PER_PP) ) {
					vp_end = vp;
				}else {
					pp_num += (1 + ((vp_end.ofs+vp_end.len) / SECTS_PER_PP) - (vp_start.ofs / SECTS_PER_PP));
					if( pp_num / rcm_read_buffer.size() > 10 ) {
						std::cout << vp_start.pgn << "," << vp_start.ofs << "," << vp_end.pgn << "," << vp_end.ofs << std::endl;
					}
					vp_start = vp;
					vp_end = vp_start;
				}
			}
			VPA_INFO vp_back = ftl_data->l2p_tbl[rcm_read_buffer.back()];
			pp_num += (1 + ((vp_back.ofs+vp_back.len) / SECTS_PER_PP) - (vp_start.ofs / SECTS_PER_PP));

			if( pp_num / rcm_read_buffer.size() > 10 ) {
				std::cout <<" aho--\n";
				//std::cout << grp_vp.pgn << "," << grp_vp.ofs << "," << grp_vp_start.pgn << "," << grp_vp_start.ofs << std::endl;
				//std::cout << pp_num << "," << rcm_read_buffer.size() << std::endl;
				//for( auto lp : rcm_read_buffer ) {
				//	std::cout << lp <<","<< ftl_data->l2p_tbl[lp].pgn <<","<< ftl_data->l2p_tbl[lp].ofs << ",";
				//}
				//std::cout << std::endl;

			}
			//static uint32_t i = 0;
			//std::cout << pp_num << std::endl;
			//if( ++i > 1000)
			//	return false;

			stats.fm_rd_sect += (pp_num * SECTS_PER_PP);
			stats.fm_gc_rd_sect += (pp_num * SECTS_PER_PP);

			//				std::cout << "rd(" ;
			//				for( auto lp : rcm_read_buffer ) {
			//					std::cout << lp << " ";
			//				}
			//				std::cout << "--" << rcm_read_buffer.size() << "," << pp_num * SECTS_PER_PP << ")" << std::endl;

			rcm_read_buffer.clear();
		}
	}

	return true;
}

bool CompSSD::buffered_data_write(FTL_FMIO_TYPE iotype, std::vector<uint64_t>& l_write_buffer)
{
	std::vector<uint32_t> len_list;
	uint32_t ttl_len = 0;
	uint32_t written_size = 0;
	FTL_PG_GADDR pg;
	// set length
	for( auto &lp : l_write_buffer ){
		if( iotype == FMT_HOST_WR ) {
			len_list.push_back( ceil((double)FTL_SECTS_PER_LP * cmp_engine->get_next_ratio()));
		}else {
			len_list.push_back( ftl_data->l2p_tbl[lp].len);
		}
		//ttl_len += len_list.back();
	}
	// write data except last one
	for( uint32_t i = 0; i < l_write_buffer.size() - 1; i++ ) {
		pg = ftl_if->GetOpenPG(iotype, l_write_buffer[i]);
		if( pg == FTL_PG_INVALID_ID ) {
			ERR_AND_RTN;
		}
		if( !ftl_if->L2P_Update(iotype, l_write_buffer[i], len_list[i], pg, written_size) ) {
			ERR_AND_RTN;
		}
		ttl_len += written_size;
	}
	// write last page
	pg = ftl_if->GetOpenPG(iotype, l_write_buffer.back());
	if( pg == FTL_PG_INVALID_ID ) {
		ERR_AND_RTN;
	}
	if( align_type == A_CA_FTL) {
		if( !ftl_if->L2P_Update(iotype, l_write_buffer.back(),
					len_list.back(), pg, written_size, true) ) {
					//len_list.back(), pg, written_size) ) {
			ERR_AND_RTN;
		}
	}else {
		if( !ftl_if->L2P_Update(iotype, l_write_buffer.back(),
					len_list.back(), pg, written_size) ) {
			ERR_AND_RTN;
		}
	}
	ttl_len += written_size;
	stats.fm_wr_sect += ttl_len;
	if( iotype == FMT_RCM_RWR )
		stats.fm_gc_wr_sect += ttl_len;

	// set read penalty
	uint32_t  r_penalty;
	if( align_type == A_CA_FTL ) {
		r_penalty = ttl_len;
	}else {
		FTL_PG_OADDR start = ftl_data->l2p_tbl[l_write_buffer[0]].ofs;
		FTL_PG_OADDR end  = ftl_data->l2p_tbl[l_write_buffer[0]].ofs + ttl_len;

		if( start / SECTS_PER_PP <= end / SECTS_PER_PP ) {
			r_penalty = SECTS_PER_PP * (1 + (end/SECTS_PER_PP - start/SECTS_PER_PP));
		}else { // spanning virtual block ... it's not correct value(smaller than actual length), but corner case.
			r_penalty = ttl_len;
		}
	}
	for( auto &lp : l_write_buffer ) {
		read_penalty[lp] = r_penalty;
	}
	//for( auto &lp : l_write_buffer ) {
	//	std::cout << "(" << lp << "," << ftl_data->l2p_tbl[lp].pgn  << "," << ftl_data->l2p_tbl[lp].ofs << "," << static_cast<int>(ftl_data->l2p_tbl[lp].len) << ","  << r_penalty << "), " ;
	//}
	//std::cout << std::endl;

	l_write_buffer.clear();

	return true;
}

