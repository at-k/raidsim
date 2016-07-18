#ifndef FTL_LP_WLCTL_H
#define FTL_LP_WLCTL_H

#include "ftl_lp_ctl.h"
#include "ftl_asyn_ctl.h"

enum WL_TYPE{
    WL_PROPOSE = 0,
	WL_STEPPING = 1,
	WL_RCM_FIX = 2,

	WL_ROUNDROBIN = 10

};

// �_���X�V�����ތn�̏����i�y�[�W���C�g�E�����֌W�jIF
class FtlInterfaceWL : public FtlInterface
{
	public:
		FtlInterfaceWL();
		virtual ~FtlInterfaceWL();

		virtual bool Init( LP_INFO* _lp_info, FM_INFO* _fm_info );

		//-- �������ݐ�p���e�B�O���[�v�擾�֐�
		//   �����F���C�g�^�[�Q�b�g�^�C�v�ihost / rc / rf)
		//         �擾�p���e�B�O���[�v�ԍ�
		//   �Ԓl�F�������ݐ�p���e�B�O���[�v�����݂��Ȃ���Ζ���ID��Ԃ�
		virtual FTL_PG_GADDR GetOpenPG ( FTL_FMIO_TYPE type, FTL_LP_GADDR lp_no );

		virtual uint32_t GetOpenPG_Num();

		//-- FTL�̒��g�_���v
		//   �f���o��
		virtual void Dump( std::ofstream& write_file );

	protected:

		typedef struct {// LP�����v���Ǘ��\����
			uchar     attr;   // ����W�p�x�N���X
			uint32_t  w_count; // ���ۂ̏�ʂ���̃��C�g��
			uint32_t  rcm_count; // ReWrite��
			uint32_t  attr_sum; // ����W�p�x�N���X�̘a
		} LP_WL_INFO;

		WL_TYPE wl_type;
		uint32_t ttl_w_count;

		// �u���b�N��ԑJ�ڊ֐� �I�[�v�����N���[�Y
		// �����FPG���
		virtual bool OpenToClose ( PG_INFO* pg_info );

		LP_WL_INFO* lp_wl_info;

};

class FtlAsynReqInterfaceWL : public FtlAsynReqInterface
{
	public:
		virtual bool Init( LP_INFO* _lp_info, FM_INFO* _fm_info, FtlInterface* _ftl_if );

	protected:
		//-- Close�L���[����RCM�Ώ�PG��T����
		//   RCM�ΏۃL���[�ɑJ�ڂ�����
		virtual bool ReloadRcmTarget( int tgt_num );


};

#endif
