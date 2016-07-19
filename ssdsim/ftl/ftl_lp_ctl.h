#ifndef L2PTBL_MNG_H
#define L2PTBL_MNG_H

#include <fstream>

#include "common_def.h"
#include "ftl_lp_info.h"
#include "ftl_asyn_ctl.h"

// �_���X�V�����ތn�̏����i�y�[�W���C�g�E�����֌W�jIF
class FtlInterface
{
	public:
		FtlInterface();
		virtual ~FtlInterface();

		//bool Init( LP_INFO* _lp_info, FM_INFO* _fm_info, FtlAsynReqInterface* _asyn_ctl ) {
		virtual bool Init( LP_INFO* _lp_info, FM_INFO* _fm_info ) {
			lp_info = _lp_info;
			fm_info = _fm_info;
			//asyn_ctl = _asyn_ctl;
			return true;
		}

		//-- �������ݐ�p���e�B�O���[�v�擾�֐�
		//   �����F���C�g�^�[�Q�b�g�^�C�v�ihost / rc / rf)
		//         �擾�p���e�B�O���[�v�ԍ�
		//   �Ԓl�F�������ݐ�p���e�B�O���[�v�����݂��Ȃ���Ζ���ID��Ԃ�
		virtual FTL_PG_GADDR GetOpenPG ( FTL_FMIO_TYPE type, FTL_LP_GADDR lp_no );

		virtual uint32_t GetOpenPG_Num(){ return FTL_OPENPG_NUM; }

		//-- �_���X�V�֐�
		//   �����FIO���
		//         �ύX�Ώۂ̘_���y�[�W
		//         �������ݐ�̕���PG�ԍ�
		//   �Ԓl�F���s����΁C�����蓖�Ă�Ԃ�
		virtual bool L2P_Update ( FTL_FMIO_TYPE type, FTL_LP_GADDR lp_no, uchar len, FTL_PG_GADDR pg_no );

		//-- �u���b�N�����֐�
		//   �����F�����Ώۂ̕����u���b�N�ԍ�
		//   �Ԓl�F�Ώۃu���b�N�̏�Ԃ��ُ�ł����false��Ԃ�
		bool ErasePB ( FTL_ERASE_REQ pb_no );

		//-- �u���b�N�����ǉ�
		//    ���́F�ǉ���v�[��
		//          �ǉ��Ώە����u���b�N
		bool InitialAdd ( POOL_INFO* pool_info, PB_INFO* pb_info );

		//-- FM�̈揉����
		//   �V�[�P���V�����ɑS�̈�1�񃉃C�g
		bool Format(bool enable_comp = false, double comp_ratio = 1.0);

		//-- FTL�̒��g�_���v
		virtual void Dump( std::ofstream& write_file );

	protected:
		LP_INFO* lp_info;
		FM_INFO* fm_info;

		// �����F�󂫃u���b�N��[�Ώۂ̃v�[���C��[�Ώۃu���b�N���
		virtual bool InvalidToFree ( PB_INFO* pb_info );

		// �u���b�N��[�֐�
		// �����F��[�Ώۃu���b�N���, �o�^�ԍ�
		virtual PG_INFO* BuildPG ( FTL_FMIO_TYPE type, uint32_t id );

		// �󂫃u���b�N�I���֐��C�p�����[�^�̍X�V���s��
		// �����F���
		virtual PB_INFO* GetFreePB ( FTL_FMIO_TYPE type );

		// �u���b�N��ԑJ�ڊ֐� �N���[�Y������
		// �����FPG���
		virtual bool CloseToInvalid ( PG_INFO* pg_info );

		// �u���b�N��ԑJ�ڊ֐� �I�[�v�����N���[�Y
		// �����FPG���
		virtual bool OpenToClose ( PG_INFO* pg_info );

		// �N���[�Y�L���[���̃u���b�N�����L���O�X�V
		virtual bool UpdateCloseRanking ( PG_INFO* pg_info );
};

#endif
