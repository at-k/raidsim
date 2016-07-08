#include "ftl_lp_info.h"
#include "ftl_lp_ctl.h"
#include "ftl_lp_func.h"
#include "ftl_asyn_ctl.h"


bool FtlAsynReqInterface::Init( LP_INFO* _lp_info, FM_INFO* _fm_info, FtlInterface* _ftl_if )
{
    lp_info = _lp_info;
    fm_info = _fm_info;

    if( lp_info->pb_num == 0 )
        return false;

    // RCM�J�n臒l�̐ݒ�
    rcm_start_th    = (FTL_PB_GADDR)(((lp_info->pb_num*lp_info->op_ratio)/100) * lp_info->rcm_th / 100 );
    if( rcm_start_th <= _ftl_if->GetOpenPG_Num() * FTL_PG_BLOCK_NUM )
    {// �Œ�ł�2�񕪂͊m��
        rcm_start_th = _ftl_if->GetOpenPG_Num() * lp_info->pg_pb_num * 2;
    }

    // pre_free_pb_num = 0;

    return true;
}

// RCM�J�n臒l�ƁC���݂̋󂫁{�����҂��{RCM�\���Q�̍����C�V�KRCM�Ώۃu���b�N�Ƃ���
//
inline int FtlAsynReqInterface::GetNeededBlockNum()
{
    return  rcm_start_th - (
        (lp_info->apg.rcm_cmp_pg_que.size() + lp_info->apg.rcm_pg_que.size()) * lp_info->pg_pb_num
        + lp_info->pool.total_invalid_pb_count + lp_info->pool.total_free_pb_count );
}

bool FtlAsynReqInterface::ReloadRcmTarget(int tgt_num)
{
    PG_INFO* pg;

    if( tgt_num > 0 )
    {// �󂫃u���b�N��RCM�\���u���b�N�̍���臒l�ȉ�
        QUEUE* que = end_item( &lp_info->apg.close_pg_que );
        QUEUE* que_tmp;

        while( tgt_num >= 0 && !is_que_end(que) )
        {// �������K�v����rcm�^�[�Q�b�g�ɒǉ�����

            pg = (PG_INFO*)get_data(que);
            pg->status = FTL_PGS_RCM;
            lp_info->apg.rcm_pg_que.push_back(pg);

            // ���o���Ď��̃A�C�e����
            que_tmp = que_forward( que );
            remove_item( &lp_info->apg.close_pg_que, que );
            que = que_tmp;

            if( pg->vs_num == lp_info->pg_cw_num )
            {// �c�L�������t���CRCM����Ӗ��Ȃ��B���������B
                PrintMessage( LOG_TYPE_ERROR, "Error FtlAsynReqInterface : RCM���N�G�X�g�ُ�\n" );
                return false;
            }

            tgt_num -= lp_info->pg_pb_num;
        }
    }
    return true;
}

//-- ReWrite�Ώۂ��擾
//   ���ł�RCM���{������s��
int FtlAsynReqInterface::GetReWriteReq( REVERSE_INFO req_list[FTL_MAX_REWR_REQ] )
{
    uint32_t req_num = 0;
    int rcm_tgt_new_block = 0;

    PG_INFO* pg;

    // RCM臒l�܂ŕs�����Ă��镪���擾
    rcm_tgt_new_block = GetNeededBlockNum();

    if( !ReloadRcmTarget( rcm_tgt_new_block ) )
        return -1;

    REVERSE_INFO rev;

    while( req_num != FTL_MAX_REWR_REQ && !lp_info->apg.rcm_pg_que.empty() )
    {
        pg = lp_info->apg.rcm_pg_que.front();
        rev = GetNextReWriteLPN( pg );

        if( rev.lpn != FTL_REWR_END )
        {
            req_list[req_num] = rev;
            req_num ++;
        }else
        {// �Ō�܂ł�肫����
            lp_info->apg.rcm_pg_que.pop_front();
            lp_info->apg.rcm_cmp_pg_que.push_back( pg );
        }
    }

    return req_num;
}

// �����Ώێ擾(BUS,CE�ɂ��ă��E���h���r��)
// �擾���ꂽ�u���b�N�͖����L���[����Ƃ肾����C�����\��L���[�ɓ������
int FtlAsynReqInterface::GetEraseReq ( FTL_ERASE_REQ erase_req[FTL_MAX_ERASE_REQ] )
{
    static uint16_t nxt_bus  = 0; // �����J�n����o�X�ԍ��F���E���h���r���Ń`�F�b�N
    static uint16_t nxt_ce   = 0;

    uint32_t   tgt_block_num = 0; // �Ԓl
    uint32_t   req_id = 0;
    PB_INFO*   pb;

    if( lp_info->pool.erase_resv_que.empty() )
    {// �����v���͊��ɏ\���o�Ă���
        return -1;
    }

    for( uint16_t k = 0; k < fm_info->GetCeNumPerBus() + 1; k++ )
    {// �S�Ă�CE�����Ƀ`�F�b�N (+ ����1��)
        for( ; nxt_bus < fm_info->GetBusNum(); nxt_bus++ )
        {// ����CE���C���Ńo�X���`�F�b�N
            if( !lp_info->pool.invalid_block_que[nxt_bus][nxt_ce].empty() &&
                !lp_info->pool.erase_resv_que.empty() )
            {
                // �L���[������o��
                pb = lp_info->pool.invalid_block_que[nxt_bus][nxt_ce].front();
                lp_info->pool.invalid_block_que[nxt_bus][nxt_ce].pop_front();
                lp_info->pool.total_invalid_pb_count --;

                // �����\��
                req_id = lp_info->pool.erase_resv_que.front();
                lp_info->pool.erase_resv_que.pop_front();
                if( lp_info->pool.erase_resv_list[req_id] != NULL )
                {
                    PrintMessage( LOG_TYPE_ERROR, "Error GetEraseReq: ���������s����\n" );
                    return -1;
                }

                lp_info->pool.erase_resv_list[req_id] = pb;
                erase_req[tgt_block_num].pb_id = pb->id;
                erase_req[tgt_block_num].reg_id = req_id;

                tgt_block_num ++;
                if( tgt_block_num == FTL_MAX_ERASE_REQ )
                {
                    nxt_bus = ROUNDROBIN( nxt_bus, fm_info->GetBusNum() );
                    if( nxt_bus == 0 )
                        nxt_ce  = ROUNDROBIN( nxt_ce, fm_info->GetCeNumPerBus() );
                    goto _ERASE_SEARCH_END;
                }
            }
        }

        nxt_bus = 0;
        nxt_ce  = ROUNDROBIN( nxt_ce, fm_info->GetCeNumPerBus() );
    }

_ERASE_SEARCH_END:

    return tgt_block_num;

}


// -- �t�����n
//
// ����ReWrite�Ώۘ_���y�[�W#���擾
inline REVERSE_INFO FtlAsynReqInterface::GetNextReWriteLPN( PG_INFO* pg_info )
{
    FTL_LP_GADDR lpn;
    REVERSE_INFO rev,rev_rtn;
    uint32_t     ofs;
    VPA_INFO     vpa;

    lpn = FTL_REWR_END;
    rev_rtn.lpn = FTL_REWR_END;

    while( pg_info->p2l_tbl.size() > pg_info->next_rewr_index )
    {// �_��=�����ȗL���y�[�W��T��
        rev = pg_info->p2l_tbl[pg_info->next_rewr_index]; // �t�����擾
        ofs = pg_info->next_rewr_pgsect;    // �`�F�b�N�Ώۂ̕����A�h���X�擾
        vpa = LpnToVpa( rev.lpn, lp_info ); // �������擾

        // �C���f�b�N�X��i�߂�
        pg_info->next_rewr_index ++;
        //pg_info->next_rewr_pgsect += ExtractVpaLength(lpn);
        pg_info->next_rewr_pgsect += rev.len;

        if( vpa.pgn == pg_info->id && vpa.ofs == ofs ) // �t�����Ɛ������̓˂����킹
        {
            rev_rtn = rev;
            break;
        }
    }

    return rev_rtn;
}

