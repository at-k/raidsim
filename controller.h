#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <deque>
#include <list>
#include <vector>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/global_fun.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#include "iosrc.h"
#include "common_def.h"

using namespace boost::multi_index;

// 512 sector = 256kB
#define DEF_STRIPE_SECT 512

typedef struct {
	uint64_t	max_lba;
} DriveInfo;

typedef enum {
	RAID5
} RAID_TYPE;

typedef struct {
	std::vector<DriveInfo*> drive_list;
	RAID_TYPE	rg_type;
	uint64_t	max_lba;
	uint32_t	drive_num;
	uint64_t	stripe_width;
	uint32_t	stripe_size;
} RGInfo;

typedef struct {

} RGOption;

typedef struct {
	uint32_t    tgt_drive;
	CommandInfo cmd_info;
} DriveCommandInfo;

typedef struct {
	uint64_t hcmd_count;
	uint64_t hcmd_rd_count;
	uint64_t hcmd_wr_count;
	uint64_t hcmd_rd_sect;
	uint64_t hcmd_wr_sect;

	uint64_t drvcmd_count;
	uint64_t drv_rd_count;
	uint64_t drv_wr_count;
	uint64_t drv_rd_sect;
	uint64_t drv_wr_sect;
} CTL_STATISTICS;

typedef struct {
	uint64_t drv_gc_rd_count;
	uint64_t drv_gc_wr_count;
	uint64_t drv_gc_rd_sect;
	uint64_t drv_gc_wr_sect;
} CTL_COMP_STATISTICS;

class Controller
{
	public:
		Controller();
		virtual ~Controller(){}

		// multi RG isnt supported
		virtual bool build_raid(RAID_TYPE type, std::vector<DriveInfo*>& drive_list, uint32_t stripe_size = DEF_STRIPE_SECT);

		// host to controller
		virtual bool receive_command(const CommandInfo&);
		// controller to drive
		virtual bool pull_next_command(DriveCommandInfo&);

		virtual void print_statistics();
		virtual void clear_statistics();

		virtual uint64_t get_max_lba() { return max_lba;}

	protected:
		std::list<RGInfo>	rg_list;
		std::list<DriveCommandInfo> pending_cmd;
		CTL_STATISTICS		stats;
		uint64_t			max_lba;

		virtual bool raid_translation(const CommandInfo& org_command, std::list<DriveCommandInfo>& cmd_list);
		virtual void record_statis(const CommandInfo& cmd);
		virtual void record_statis(const DriveCommandInfo& cmd);
};

class CompEngine;

class CompController : public Controller
{
	public:
		CompController();
		virtual ~CompController();

		bool init(CompEngine* cmp_engine, double gc_buffer_ratio = 0.8, uint64_t chunk_sector_size = 16);

		virtual uint64_t get_max_lba(){ return virtual_max_lba;}
		// host to controller
		virtual bool receive_command(const CommandInfo&);
		// controller to drive
		virtual bool pull_next_command(DriveCommandInfo&);

		virtual void print_statistics();
		virtual void clear_statistics();

		uint64_t get_lp_num() { return lp_num;}

		void print_map();

		bool do_gc();
	private:
		typedef struct {
			uint32_t lb_id;
			uint32_t ofs;
			uint32_t len;
		} LPMAP;

		typedef struct {
			uint32_t ofs;
			uint64_t lpn;
		} REVMAP;

		typedef enum {
			LB_FREE = 0,
			LB_OPEN = 1,
			LB_CLOSE = 2
		} LOGBLOCK_STATE;

		typedef struct {
			uint32_t			id;
			LOGBLOCK_STATE		state;
			uint32_t			free_sector_num;
			std::deque<REVMAP>	rev_map;
		} LOGBLOCK;

		typedef struct {
			LOGBLOCK*	tgt_lgblock;
			uint32_t	cur_sector;
		} BUFINFO;

		typedef struct {
			uint64_t  lpn;
			bool	  rmw_needed;
		} LPCMD;

		struct free_sector {};

		typedef composite_key<
			LOGBLOCK,
			BOOST_MULTI_INDEX_MEMBER(LOGBLOCK, LOGBLOCK_STATE, state),
			BOOST_MULTI_INDEX_MEMBER(LOGBLOCK, uint32_t, free_sector_num)
		> key_t;
		typedef composite_key_compare <
			    std::greater<uint32_t>,
			    std::greater<uint32_t>
		> key_comp;

		typedef multi_index_container<
			LOGBLOCK*,
			indexed_by<
				random_access<>,
				ordered_non_unique<tag<free_sector>, key_t, key_comp >
			>
		> LB_SORTED_LIST;
		//typedef LB_SORTED_LIST::index<free_sector>::type::iterator LB_ITR;
		CompEngine* cmp_engine;

		BUFINFO		buf;
		LPMAP*		lpt;
		LOGBLOCK*   lbt;
		LB_SORTED_LIST			lb_list;
		std::deque<LOGBLOCK*>	free_list;
		std::deque<LOGBLOCK*>   destage_tgt;

		uint64_t real_data_size;
		uint64_t virtual_max_lba;
		uint64_t lb_num;
		uint64_t lp_num;

		CTL_COMP_STATISTICS cmp_stats;
		// given parameter
		double   real_data_ratio;
		double	 avg_comp_ratio;
		uint64_t gc_threshold;
		uint64_t lb_size;
		uint64_t lp_size;

		// function
		bool extract_hcmd_to_lp(const CommandInfo& host_cmd, std::list<LPCMD>& lp_list);
		bool close_buf_and_open_next();
		bool is_buf_full(BUFINFO& buf, uint32_t next_data_len) { return buf.cur_sector + next_data_len > lb_size;}

		bool lp_read(uint64_t lp_no);
		bool lp_write(uint64_t lp_no);

		uint32_t do_compression(uint32_t org_data_size);
		uint64_t calc_lp_no(uint64_t host_lba);
		uint64_t calc_raid_addr(uint32_t lb_id, uint32_t ofs);
		void update_lb_list(LOGBLOCK* lb);
};

#endif
