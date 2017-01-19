#ifndef ASYN_IO_MNG_H
#define ASYN_IO_MNG_H

//-- �񓯊�I/O�iRCM & RF�j...�Ƃ肠����RF�͕ۗ�
//   ��ԑJ�ڌn�͂��Ȃ��CRCM�Ώ�PB/(LC)�̌���ƁC�y�[�W�R�s�[�R�}���h�����̂�
//   �Ƃ�������{�I�Ƀp�����[�^�̕ύX�͂��Ȃ�

#include "../phy/fm_arch_info.h"
#include "ftl_lp_info.h"

// �����ERCM�Ȃǂ̔񓯊��v���֌W
class FtlAsynReqInterface
{
public:
	virtual ~FtlAsynReqInterface(){}

    virtual bool Init( LP_INFO* _lp_info, FM_INFO* _fm_info, FtlInterface* _ftl_if );

    //-- RCM�ɂ��IO�v����������
    //   RCM���{����ꍇ�͂�������u���b�N��I�ԁB���ۂ̃R�s�[�͏�ɂ��C��
    //   �Ԃ�l�͗v����
    int GetReWriteReq( REVERSE_INFO req_list[FTL_MAX_REWR_REQ] );

    //-- Erase�v��������
    //   ������Ȃ��Ƌ󂫕s���Ŏ���
    //   �Ԃ�l�͗v����
    int GetEraseReq( FTL_ERASE_REQ erase_req[FTL_MAX_ERASE_REQ] );

protected:
    LP_INFO* lp_info;
    FM_INFO* fm_info;

    FTL_PB_GADDR rcm_start_th;
    //FTL_PB_GADDR pre_free_pb_num;

    int GetNeededBlockNum();

    //-- Close�L���[����RCM�Ώ�PG��T����
    //   RCM�ΏۃL���[�ɑJ�ڂ�����
    virtual bool ReloadRcmTarget( int tgt_num );

    REVERSE_INFO GetNextReWriteLPN( PG_INFO* pg_info );

};


#endif
