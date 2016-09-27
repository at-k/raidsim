#ifndef SSD_H_
#define SSD_H_

#include "common_def.h"
#include "ftl/ftl_lp_info.h"
#include <vector>

class FM_INFO;
class LP_INFO;
class FtlInterface;
class FtlAsynReqInterface;

typedef struct {
	uint64_t hcmd_count;
	uint64_t hcmd_rd_count;
	uint64_t hcmd_wr_count;
	uint64_t hcmd_rd_sect;
	uint64_t hcmd_wr_sect;

	uint64_t fmcmd_count;
	uint64_t fm_rd_count;
	uint64_t fm_wr_count;
	uint64_t fm_rd_sect;
	uint64_t fm_wr_sect;

	uint64_t fm_gc_rd_count;
	uint64_t fm_gc_wr_count;
	uint64_t fm_gc_rd_sect;
	uint64_t fm_gc_wr_sect;
	uint64_t fm_pb_erase;
} SSD_STATISTICS;


class SSD {
	public:
		SSD();
		virtual ~SSD();

		bool simple_init(uint64_t ttl_phy_bytes, double op_ratio);

		virtual bool init(uint32_t channel_num, uint32_t pkg_per_ch,
				uint32_t die_per_pkg, uint64_t byte_per_die, double op_ratio);

		bool write(uint64_t lba, uint32_t len);
		bool read(uint64_t lba, uint32_t len);

		inline uint64_t get_max_lba() { return max_lba;}
		inline uint64_t get_max_lpn() { return max_lba / FTL_SECTS_PER_LP;}
		inline uint64_t get_phy_sect() { return phy_sects;}

		virtual void Dump( std::ofstream& write_file );
		virtual void print_statistics();
		virtual void clear_statistics();

	protected:
		virtual bool lp_read(uint64_t lp_no, FTL_FMIO_TYPE type);
		virtual bool lp_write(uint64_t lp_no, FTL_FMIO_TYPE type);
		virtual bool do_gc();

		uint64_t max_lba;
		uint64_t phy_sects;
		SSD_STATISTICS stats;
		double op_ratio;

		FM_INFO*      fm_info;
		LP_INFO*      ftl_data;
		FtlInterface* ftl_if;
		FtlAsynReqInterface* ftl_asyn_if;
};

class CompEngine {
	public:
		CompEngine();
		~CompEngine();

		bool init_engine(const char* trace_file_name);
		bool init_engine(double avg_cmp_ratio);
		uint32_t get_next_length();
		double get_next_ratio();

		double get_avg_cmp_ratio() { return avg_comp_ratio;}

	private:
		std::vector<uint32_t> data_list;
		uint32_t	org_chunk_size;
		uint32_t    avg_comp_size;
		double		avg_comp_ratio;
		uint32_t    cur_pointer;
};

typedef enum {
	A_CA_FTL,
	A_CC_FTL
} ALIGN_TYPE;

class CompSSD : public SSD {
	public:
		CompSSD();
		virtual ~CompSSD();

		virtual bool init(uint32_t channel_num, uint32_t pkg_per_ch,
				uint32_t die_per_pkg, uint64_t byte_per_die, double op_ratio);
		bool setup_compression(bool enable_virtualization, double avg_cmp_ratio, double avg_dev = 1, ALIGN_TYPE a_type = A_CC_FTL, uint32_t b_page_num = 4);
		bool setup_compression(bool _enable_virtualization, CompEngine* _cmp_engine, ALIGN_TYPE a_type, uint32_t b_page_num ) {
			enable_virtualization = _enable_virtualization;
			cmp_engine = _cmp_engine;
			align_type = a_type;
			buffered_page_num = b_page_num;
			return true;}
		virtual void print_statistics();

	protected:
		virtual bool lp_read(uint64_t lp_no, FTL_FMIO_TYPE type);
		virtual bool lp_write(uint64_t lp_no, FTL_FMIO_TYPE type);

	private:
		CompEngine* cmp_engine;
		double avg_cmp_ratio;
		double avg_dev;
		bool enable_virtualization;
		uint32_t buffered_page_num;

		ALIGN_TYPE align_type;

		std::vector<uint64_t>  write_buffer;
		std::vector<uint64_t>  rcm_write_buffer;
		std::vector<uint64_t>  rcm_read_buffer;

		std::vector<uint32_t>	read_penalty;

		bool buffered_data_write(FTL_FMIO_TYPE iotype, std::vector<uint64_t>& write_buffer);

};


#endif
