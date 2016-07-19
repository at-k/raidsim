#ifndef SSD_H_
#define SSD_H_

#include "common_def.h"
#include "ftl/ftl_lp_info.h"

class FM_INFO;
class LP_INFO;
class FtlInterface;
class FtlAsynReqInterface;

class SSD {
	public:
		SSD();
		virtual ~SSD();

		virtual bool init(uint32_t channel_num, uint32_t pkg_per_ch,
				uint32_t die_per_pkg, uint64_t byte_per_die, double op_ratio);

		bool write(uint64_t lba, uint32_t len);
		bool read(uint64_t lba, uint32_t len);

		inline uint64_t get_max_lba() { return max_lba;}
		inline uint64_t get_max_lpn() { return max_lba / FTL_SECTS_PER_LP;}

		virtual void Dump( std::ofstream& write_file );

	protected:
		virtual bool lp_read(uint64_t lp_no);
		virtual bool lp_write(uint64_t lp_no, FTL_FMIO_TYPE type);
		virtual bool do_gc();

		uint64_t max_lba;

		FM_INFO*      fm_info;
		LP_INFO*      ftl_data;
		FtlInterface* ftl_if;
		FtlAsynReqInterface* ftl_asyn_if;
};

class CompSSD : public SSD{
	public:
		CompSSD();
		virtual ~CompSSD();

		virtual bool init(uint32_t channel_num, uint32_t pkg_per_ch,
				uint32_t die_per_pkg, uint64_t byte_per_die, double op_ratio);
		bool setup_cmp_engine(double avg_cmp_ratio, double avg_dev);

	protected:
		virtual bool lp_read(uint64_t lp_no);
		virtual bool lp_write(uint64_t lp_no, FTL_FMIO_TYPE type);

	private:
		double avg_cmp_ratio;
		double avg_dev;
};

#endif
