#ifndef SIM_CORE___H
#define SIM_CORE___H

#include <string>
#include "util/common_def.h"

#define CONF_FILE_VERSION "3.1"

class IoGenerator;
class FM_INFO;
class FTL_LP_INFO;

enum SIM_TYPE
{
	IO_TEST_MODE,
	SIM_MODE
};

enum OUTPUT_TRIGER_TYPE
{
	OTT_IO_KILO_COUNT,
	OTT_SIM_TIME_MS
};

typedef struct
{
	SIM_TYPE  sim_type; // �V�~�����[�V�����^�C�v
	uint64_t  sim_time; // ���݂̃V�~�����[�V��������
	uint64_t  io_count; // ���݂�IO�J�E���g��

	uint32_t  io_tag;   // ��x�ɕ�����IO���i�^�O�j

	OUTPUT_TRIGER_TYPE out_type; // �o�͌_�@���I���_�@�̎��
	uint64_t  end_count;         // �I���\��(���� or �J�E���g��)
	uint32_t  out_interval;      // ���ʏo�͊Ԋu

	uint64_t  trace_on_io_count; // Trace_ON��������IO�J�E���g
	uint64_t  trace_off_io_count;// Trace_OFF��������IO�J�E���g

	uint32_t  rand_seed;// seed for random function
} SIM_INFO;

class SimCore
{
	public:
		SimCore();
		~SimCore();

		bool Initialize( const char* conffile, const char* outdir );
		void Close();

		// step execution interface
		void RunStep();

		inline bool IsSimEnd() { return is_sim_end;}

		// reset simulater conclusion condition
		void ResetEndCounter();

		void PrintHeader();
		void PrintMidResult();
		void PrintFinalResult();

	private:
		SIM_INFO	  sim_info;
		std::string   conf_file_name;

		IoGenerator*   io_gen;
		//FM_INFO*      fm_info;
		//LP_INFO*      ftl_data;
		//FtlInterface* ftl_if;
		//FtlAsynReqInterface* ftl_asyn_if;

		uint64_t io_count;
		uint64_t end_time_org;

		uint64_t cum_copy_count;
		uint64_t cum_host_count;

		bool     is_sim_end;
};

#endif
