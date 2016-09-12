#include "controller.h"
#include "common_def.h"
#include "ssdsim/ssd.h"

#include <string.h>
#include <stdio.h>

#include <iostream>
#include <algorithm>

#include <boost/foreach.hpp>

Controller::Controller() {
	memset(&stats, 0, sizeof(CTL_STATISTICS) );
	rg_list.clear();
	pending_cmd.clear();
	max_lba = 0;
}

bool Controller::build_raid(RAID_TYPE type, std::vector<DriveInfo*>& drive_list, uint32_t stripe_size)
{
	if( drive_list.size() < 2 )
		ERR_AND_RTN;

	if( type != RAID5 || rg_list.size() == 1 ) {
		printf( HERE "error : multi rg and any RAID type except for RAID5 is not supported\n");
		return false;
	}

	RGInfo rg;
	rg.max_lba = 0;
	rg.rg_type = type;
	rg.drive_num = drive_list.size();
	rg.stripe_size = stripe_size;

	uint64_t drv_lba = drive_list.front()->max_lba;
	if( stripe_size > drv_lba )
		ERR_AND_RTN;

	DriveInfo* drv;
	for( uint32_t i = 0; i < drive_list.size(); i++ )
	{
		drv = drive_list[i];
		if( drv->max_lba != drv_lba )
			ERR_AND_RTN;

		rg.drive_list.push_back(drv);
	}


	if( drv_lba % stripe_size != 0 ) {
		printf("# round up to stripe %d,%ld\n", stripe_size, drv_lba);
		drv_lba -= drv_lba % stripe_size;
	}

	if( type == RAID5 ) {
		// considering parity capacity
		rg.max_lba = drv_lba * (rg.drive_num - 1 );
		rg.stripe_width = rg.stripe_size * (rg.drive_num - 1);
	}

	max_lba += rg.max_lba;
	rg_list.push_back(rg);

	return true;
}

bool Controller::raid_translation(const CommandInfo& tgt_cmd, std::list<DriveCommandInfo>& cmd_list)
{
	uint64_t ofs_in_rg;

	// the number of RG is restricted to one
	RGInfo& rg = rg_list.front();
	ofs_in_rg  = tgt_cmd.lba;

	if( ofs_in_rg + tgt_cmd.sector_num > rg.max_lba )
		ERR_AND_RTN;

	if( rg.rg_type == RAID5 )
	{
		uint64_t ttl_length = tgt_cmd.sector_num;
		bool  multi_drive = false;
		bool  parity_update = false;

		while(ttl_length != 0)
		{
			uint64_t stripe_row_no  = ofs_in_rg / rg.stripe_width;
			uint64_t ofs_in_stripe  = ofs_in_rg % rg.stripe_width;
			uint64_t ofs_in_str_drv = ofs_in_stripe % rg.stripe_size;
			uint32_t parity_drive = (uint32_t)(stripe_row_no % rg.drive_num);
			uint32_t tgt_drive = ofs_in_stripe / rg.stripe_size;

			if( tgt_drive >= parity_drive ) {
				tgt_drive += 1;
				if( tgt_drive == rg.drive_num )
					tgt_drive = 0;
			}

			DriveCommandInfo cmd;
			cmd.tgt_drive = tgt_drive;

			// lba
			cmd.cmd_info.lba = stripe_row_no * rg.stripe_size + ofs_in_str_drv;

			// length
			if( ofs_in_str_drv + ttl_length <= rg.stripe_size )
				cmd.cmd_info.sector_num = ttl_length;
			else {
				cmd.cmd_info.sector_num = rg.stripe_size - ofs_in_str_drv;
				multi_drive = true;
			}

			cmd.cmd_info.opcode = tgt_cmd.opcode;

			// old data read for parity update
			if( tgt_cmd.opcode == IO_WRITE )
			{
				DriveCommandInfo rmw = cmd;
				rmw.cmd_info.opcode = IO_READ;

				cmd_list.push_back(rmw);

				// parity update
				if( !parity_update )
				{
					parity_update = true;

					DriveCommandInfo p_read;
					p_read.cmd_info.opcode = IO_READ;
					p_read.tgt_drive = parity_drive;

					if( multi_drive ) {
						p_read.cmd_info.sector_num = rg.stripe_size;
						p_read.cmd_info.lba = stripe_row_no * rg.stripe_size;
					} else {
						p_read.cmd_info.sector_num = rmw.cmd_info.sector_num;
						p_read.cmd_info.lba = rmw.cmd_info.lba;
					}

					DriveCommandInfo p_write = p_read;
					p_write.cmd_info.opcode = IO_WRITE;

					cmd_list.push_back(p_read);
					cmd_list.push_back(p_write);
				}
			}

			ttl_length -= cmd.cmd_info.sector_num;
			ofs_in_rg  += cmd.cmd_info.sector_num;
			cmd_list.push_back(cmd);

			// next stripe
			if(  ofs_in_stripe + cmd.cmd_info.sector_num == rg.stripe_width ) {
				multi_drive = false;
				parity_update = false;
			}
		}

	}else
		return false;

	return true;
}

void Controller::record_statis(const CommandInfo& cmd)
{
	stats.hcmd_count ++;
	if(cmd.opcode == IO_READ) {
		stats.hcmd_rd_count++;
		stats.hcmd_rd_sect += cmd.sector_num;
	}else if(cmd.opcode == IO_WRITE) {
		stats.hcmd_wr_count++;
		stats.hcmd_wr_sect += cmd.sector_num;
	}
}
void Controller::record_statis(const DriveCommandInfo& cmd)
{
	stats.drvcmd_count ++;
	if(cmd.cmd_info.opcode == IO_READ) {
		stats.drv_rd_count++;
		stats.drv_rd_sect += cmd.cmd_info.sector_num;
	}else if(cmd.cmd_info.opcode == IO_WRITE) {
		stats.drv_wr_count++;
		stats.drv_wr_sect += cmd.cmd_info.sector_num;
	}
}
void Controller::print_statistics()
{
	std::cout << "#controller statistics, max_lba, hcmd_cnt, hcmd_rd_cnt, hcmd_wr_cnt, hcmd_rd_sect, hcmd_wr_sect, dcmd_cnt, dcmd_rd_cnt, dcmd_wr_cnt, dcmd_rd_sect, dcmd_wr_sect " << std::endl;
	std::cout << "," << get_max_lba() << "," << stats.hcmd_count <<","<<stats.hcmd_rd_count <<","<<stats.hcmd_wr_count << ","
		<< stats.hcmd_rd_sect << "," << stats.hcmd_wr_sect << ","<<stats.drvcmd_count << ","
		<< stats.drv_rd_count<<","<<stats.drv_wr_count<<","<<stats.drv_rd_sect<<","<<stats.drv_wr_sect<<"\n";
}
void Controller::clear_statistics()
{
	memset(&stats, 0, sizeof(CTL_STATISTICS) );
}

bool Controller::receive_command(const CommandInfo& cmd)
{
	record_statis(cmd);
	if( !raid_translation(cmd, pending_cmd) )
		ERR_AND_RTN;
	return  true;
}

bool Controller::pull_next_command(DriveCommandInfo& next_cmd)
{
	if( pending_cmd.empty() )
		return false;

	//memcpy( &next_cmd, &pending_cmd.front(), sizeof(DriveCommandInfo) );
	next_cmd = pending_cmd.front();
	pending_cmd.pop_front();

	record_statis(next_cmd);

	return true;
}

// ---- compression controller implementation
//
CompController::CompController()
{
	lb_list.clear();
	free_list.clear();
	destage_tgt.clear();
	lpt = NULL; lbt = NULL;

	memset(&cmp_stats, 0, sizeof(CTL_COMP_STATISTICS) );
}

CompController::~CompController()
{
	if( lpt != NULL ) delete[] lpt;
	if( lbt != NULL ) delete[] lbt;
}

bool CompController::init(CompEngine* _cmp_engine, double gc_buffer_ratio, uint64_t chunk_sector_size)
{
	if( lpt != NULL || lbt != NULL )
		ERR_AND_RTN;

	// test parameter
	real_data_ratio = 1 - gc_buffer_ratio;
	cmp_engine = _cmp_engine;
	avg_comp_ratio = cmp_engine->get_avg_cmp_ratio();
	lb_size = rg_list.front().stripe_width;
	lp_size = chunk_sector_size;

	// parameter check
	if( ( rg_list.size() != 1 ) ||
		( lb_size < lp_size || lb_size % lp_size != 0 ) ||
		( rg_list.front().max_lba % lb_size != 0 ) ) {
		printf( HERE "error : parameter violation %ld, %ld, %ld\n", lp_size, lb_size, rg_list.front().max_lba);
		return false;
	}

	// set data size ... aligned to log block size
	real_data_size = rg_list.front().max_lba * real_data_ratio;
	real_data_size += ( (real_data_size % lb_size) == 0 ? 0 : (lb_size - (real_data_size % lb_size)) );

	virtual_max_lba = real_data_size / avg_comp_ratio;
	virtual_max_lba += ( (virtual_max_lba % lb_size) == 0 ? 0 : (lb_size - (virtual_max_lba % lb_size)) );

	// init log block
	lb_num = rg_list.front().max_lba / lb_size;
	gc_threshold = (uint64_t)lb_num * (0.05);

	lbt = new LOGBLOCK[lb_num];
	for( uint64_t i = 0; i < lb_num; i++ ) {
		lbt[i].id = i; lbt[i].state = LB_FREE; lbt[i].free_sector_num = lb_size;
		free_list.push_back( &(lbt[i]) );
		lb_list.push_back( &(lbt[i]) );
	}

	lp_num = virtual_max_lba / lp_size;
	lpt = new LPMAP[lp_num];

	// set first open block
	LOGBLOCK* lb = free_list.front();
	free_list.pop_front();
	buf.tgt_lgblock = lb;
	buf.cur_sector = 0;

	//-- write initial data
	for( uint64_t i = 0; i < lp_num; i++ ) {
		lpt[i].len = lp_size *  avg_comp_ratio;

		if( is_buf_full(buf, lpt[i].len) && !close_buf_and_open_next() )
			ERR_AND_RTN;

		// update map
		lpt[i].lb_id = buf.tgt_lgblock->id;
		lpt[i].ofs = buf.cur_sector;

		// add reverse map
		REVMAP revmap = { buf.cur_sector, i};
		buf.tgt_lgblock->rev_map.push_back(revmap);

		// update buffer information
		buf.cur_sector += lpt[i].len;
		buf.tgt_lgblock->free_sector_num -= lpt[i].len;

	}

	destage_tgt.clear();

	return true;
}

bool CompController::receive_command(const CommandInfo& host_cmd)
{
	std::list<LPCMD> tmp_cmd_list;

	extract_hcmd_to_lp(host_cmd, tmp_cmd_list);

	// for test
	//std::for_each(tmp_cmd_list.begin(), tmp_cmd_list.end(), [](LPCMD& lp) {
	//	std::cout<< lp.lpn << "," << lp.rmw_needed << "\n";
	//});

	if( host_cmd.opcode == IO_READ )
	{
		std::list<LPCMD>::iterator itr;
		for( itr = tmp_cmd_list.begin(); itr != tmp_cmd_list.end(); itr++ )
			if( !lp_read( (*itr).lpn ) )
				ERR_AND_RTN;
	}
	else if( host_cmd.opcode == IO_WRITE )
	{
		std::list<LPCMD>::iterator itr;
		for( itr = tmp_cmd_list.begin(); itr != tmp_cmd_list.end(); itr++ ) {
			if( (*itr).rmw_needed && !lp_read((*itr).lpn) )
				ERR_AND_RTN;
			if( !lp_write((*itr).lpn) )
				ERR_AND_RTN;
		}
	}else
		return false;

	if( free_list.size() < gc_threshold && !do_gc() )
		ERR_AND_RTN;

	record_statis( host_cmd );

	return true;
}

bool CompController::pull_next_command(DriveCommandInfo& next_cmd)
{
	if( pending_cmd.empty() )
		return false;

	next_cmd = pending_cmd.front();
	pending_cmd.pop_front();

	record_statis( next_cmd );

	return true;
}

bool CompController::extract_hcmd_to_lp(const CommandInfo& host_cmd, std::list<LPCMD>& lp_list)
{
	uint64_t len  = host_cmd.sector_num;
	uint64_t sect = host_cmd.lba;

	LPCMD cmd;

	while(len != 0 )
	{
		cmd.rmw_needed = false;
		cmd.lpn = calc_lp_no(sect);
		if( sect % lp_size != 0 )
			cmd.rmw_needed = true;

		if( cmd.lpn != calc_lp_no(sect + len) ) {
			uint64_t next_sect = (cmd.lpn + 1) * lp_size;
			len = len - (next_sect - sect);
			sect = next_sect;
		}else {
			len = 0;
			cmd.rmw_needed = true;
		}

		lp_list.push_back(cmd);
	}

	return true;
}

bool CompController::close_buf_and_open_next()
{
	if( buf.tgt_lgblock == NULL )
		return false;
	if( free_list.empty() )
		ERR_AND_RTN;

	buf.tgt_lgblock->state = LB_CLOSE;
	destage_tgt.push_back(buf.tgt_lgblock);

	update_lb_list( buf.tgt_lgblock );

	LOGBLOCK* lb;
	lb = free_list.front();
	free_list.pop_front();

	lb->state = LB_OPEN;
	lb->free_sector_num = lb_size;
	lb->rev_map.clear();

	buf.tgt_lgblock = lb;
	buf.cur_sector = 0;

	update_lb_list( buf.tgt_lgblock );

	return true;
}

bool CompController::lp_read(uint64_t lp_no)
{
	if( lp_no > lp_num )
		ERR_AND_RTN;

	LPMAP lp_map = lpt[lp_no];
	CommandInfo cmd;
	cmd.lba = calc_raid_addr(lp_map.lb_id, lp_map.ofs);
	cmd.sector_num = lp_map.len;
	cmd.opcode = IO_READ;

	if( !raid_translation(cmd, pending_cmd) )
		ERR_AND_RTN;

	return true;
}

bool CompController::lp_write(uint64_t lp_no)
{
	if( lp_no > lp_num )
		ERR_AND_RTN;

	LPMAP old_lp_map = lpt[lp_no];

	// data compression
	uint32_t length = do_compression( lp_size );

	// get next write destination
	if( is_buf_full(buf, length) && !close_buf_and_open_next() )
		ERR_AND_RTN;

	// update map
	lpt[lp_no].lb_id = buf.tgt_lgblock->id;
	lpt[lp_no].ofs = buf.cur_sector;

	// update reverse map
	REVMAP revmap = { buf.cur_sector , lp_no };
	buf.tgt_lgblock->rev_map.push_back(revmap);

	// invalidate old data
	lbt[old_lp_map.lb_id].free_sector_num += old_lp_map.len;
	if( lbt[old_lp_map.lb_id].free_sector_num > lb_size )
		ERR_AND_RTN;

	if( lbt[old_lp_map.lb_id].free_sector_num == lb_size ) {
		lbt[old_lp_map.lb_id].state = LB_FREE;
		free_list.push_back( &lbt[old_lp_map.lb_id] );
	}
	update_lb_list( &lbt[old_lp_map.lb_id] );

	// update buffer information
	buf.cur_sector += length;
	buf.tgt_lgblock->free_sector_num -= length;
	update_lb_list( buf.tgt_lgblock );

	// destaging
	while( !destage_tgt.empty() ) {
		LOGBLOCK* lg = destage_tgt.front();
		destage_tgt.pop_front();

		CommandInfo cmd;
		cmd.lba = calc_raid_addr( lg->id, 0 );
		cmd.sector_num = lb_size;
		cmd.opcode = IO_WRITE;

		if( !raid_translation(cmd, pending_cmd) )
			ERR_AND_RTN;
	}

	return true;
}

bool CompController::do_gc()
{
	LB_SORTED_LIST::index<free_sector>::type& fs = lb_list.get<free_sector>();

	BOOST_FOREACH( LOGBLOCK* lb, fs) {
		if( lb->state != LB_CLOSE || lb->free_sector_num == 0 )
			ERR_AND_RTN;

		std::deque<REVMAP>::iterator itr;
		for( itr = lb->rev_map.begin(); itr != lb->rev_map.end(); itr++ ) {
			LPMAP& lp_map = lpt[(*itr).lpn];
			if( lp_map.lb_id == lb->id && lp_map.ofs == (*itr).ofs ) {
				if( !lp_read((*itr).lpn) )
					ERR_AND_RTN;
				if( !lp_write((*itr).lpn) )
					ERR_AND_RTN;
				cmp_stats.drv_gc_rd_count ++;
				cmp_stats.drv_gc_wr_count ++;
				cmp_stats.drv_gc_rd_sect += lp_map.len;
				cmp_stats.drv_gc_wr_sect += lp_map.len;
			}
		}

		if( free_list.size() >= gc_threshold )
			return true;
	}

	return true;
}
uint32_t CompController::do_compression( uint32_t org_data_size )
{
	return org_data_size *  cmp_engine->get_next_ratio();
}

inline uint64_t CompController::calc_lp_no(uint64_t host_lba)
{
	return host_lba / lp_size;
}

inline uint64_t CompController::calc_raid_addr(uint32_t lb_id, uint32_t ofs)
{
	return lb_id * lb_size + ofs;
}

inline void CompController::update_lb_list(LOGBLOCK* lb)
{
	lb_list.replace( lb_list.begin() + lb->id, lb );
}

void CompController::print_statistics()
{
	std::cout << "#comp_controller statistics, virutal_max_lba, gc_rd_cnt, gc_wr_cnt, gc_rd_sect, gc_wr_sect" << std::endl;
	std::cout << "," << virtual_max_lba << "," << cmp_stats.drv_gc_rd_count << "," << cmp_stats.drv_gc_wr_count << ","
		<< cmp_stats.drv_gc_rd_sect << "," << cmp_stats.drv_gc_wr_sect << std::endl;
	Controller::print_statistics();
}
void CompController::clear_statistics()
{
	memset(&cmp_stats, 0, sizeof(CTL_COMP_STATISTICS) );
	Controller::clear_statistics();
}


// for debug
void CompController::print_map()
{
	printf("%ld, %ld, %ld, %ld, %ld, %ld\n",
			rg_list.front().max_lba, real_data_size, virtual_max_lba, lb_num, lp_num, free_list.size() );

	printf("--lpmap\n");
	for( uint64_t i = 0; i < lp_num; i++ ) {
		printf("%d, %d, %d \n", lpt[i].lb_id, lpt[i].ofs, lpt[i].len );
	}

	printf("--log block info\n");
	for( uint64_t i = 0; i < lb_num; i++ ) {
		printf("%d, %d, %d ->", lbt[i].id, lbt[i].state, lbt[i].free_sector_num );
		for( uint64_t k = 0; k < lbt[i].rev_map.size(); k++ )
			printf("(%d, %ld)", lbt[i].rev_map[k].ofs, lbt[i].rev_map[k].lpn );
		printf("\n");
	}

	printf("--log block info free sector rank\n");
	LB_SORTED_LIST::index<free_sector>::type& c0 = lb_list.get<free_sector>();
	BOOST_FOREACH( LOGBLOCK* blk, c0 ) {
		printf("%d, %d, %d \n", blk->id, blk->state, blk->free_sector_num );
	}
	printf("\n");
}
