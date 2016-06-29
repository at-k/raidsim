
#include "ftl_lp_info.h"
#include "ftl_lp_ctl.h"
#include "ftl_lp_func.h"
#include "ftl_asyn_ctl.h"


//#define MAX_PB_TGT_NUM_LC 10
//#define MAX_REQ_PAGE_NUM   4

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

#if 0

bool GetRcmRequest( LP_INFO* l2p_info, uint32_t tgtlp_list[MAX_RCM_IO_CREATE_NUM], uint16_t* io_count )
{
    static uint16_t   next_pool = 0;  // �|�[�����O���J�n����v�[���ԍ�
    static uint32_t   tgtlp_list_tmp[MAX_REQ_PAGE_NUM];

    POOL_INFO* pool_info;
    uint16_t   pool_no;

    PB_INFO*   pb_info;
    QUEUE*     pb_que;

    *io_count = 0;  // �v������������

    for ( pool_no = 0; pool_no < l2p_info->pool_num; pool_no++ )
    {
        // �|�[�����O�Ώۂ̃v�[����I��
        pool_info = &l2p_info->pool_list[next_pool];

        UpdatePoolRcmRequest( l2p_info, pool_info );

        // �S�Ă̓��Y�v�[����RCM�Ώۂɂ��ăT�[�`

        pb_que = begin_item( &pool_info->rcm_block_que );
        while ( !is_que_end( pb_que ) )
        {
            uint32_t needed_count;

            pb_info = (PB_INFO*)pb_que->item;

            needed_count = MAX_RCM_IO_CREATE_NUM - *io_count; // �K�v���擾

            needed_count = SearchRcmTargetPage( l2p_info, pb_info, tgtlp_list_tmp, needed_count ); // �Ώۃy�[�W���擾

            for ( uint32_t i = 0; i < needed_count ; i ++ )
            {
                tgtlp_list[*io_count] = tgtlp_list_tmp[i];  // ���ʊi�[
                (*io_count) = (*io_count) + 1;              // �C���N�������g
            }

            if ( *io_count == MAX_RCM_IO_CREATE_NUM )
                goto __RCM_REQ_END;          // �I���

            pb_que = next_back( pb_que ); // ���̃L���[��
        }

        //-- ���E���h���r��
        next_pool ++;
        if ( next_pool == l2p_info->pool_num )  next_pool = 0;
    }
__RCM_REQ_END:

    return true;
}

//-- �u���b�N�����邽�тɃR�[��
void UpdatePoolRcmRequest( LP_INFO* l2p_info, POOL_INFO* pool_info )
{
    int tgt_pb_num = 0;
    uint32_t rank = 0;

    if ( pool_info->total_free_pb_count < l2p_info->pool_rc_th )
    {// �s��������
        //-- �K�v�����v�Z
        // 臒l�ɓ͂��Ă��Ȃ����̂����CRCM�o�^�ς݂̂��́C���ɖ��������ꂽ���́i�̔����j������
        tgt_pb_num = (l2p_info->pool_rc_th - pool_info->total_free_pb_count) -
            (pool_info->rcm_block_que.count + (pool_info->total_invalid_pb_count / 2));
    }

    if ( tgt_pb_num <= 0 )
    {// ���ɏ\���̃u���b�N���o�^�ς�
        return;
    }

    if ( tgt_pb_num <= 0 )
    {// ���ɏ\���̃u���b�N���o�^�ς�
        return;
    }

    // �s�����������L���O��̃`�����N����₤
    SearchRcmTargetPB ( l2p_info, pool_info, tgt_pb_num );

    return;
}

uint32_t SearchRcmTargetPB ( LP_INFO* l2p_info, POOL_INFO* pool_info, uint32_t needed_count )
{
    uint32_t   pb_num;
    uint16_t   rank;
    QUEUE*     pb_que;

    pb_num = 0;

    // �Ώۃu���b�N�T�[�`
    for( rank = 0; rank < CLOSE_BLOCK_RANK; rank++ )
    {
        while( pool_info->close_block_que[rank].count != 0 )
        {
            // �ΏۃA�C�e���擾
            pb_que = begin_item( &pool_info->close_block_que[rank] );

            // RCM�Ώۃu���b�N�Ƃ��ēo�^
            SetPBasRcmTarget( l2p_info, ((PB_INFO*)(pb_que->item))->pb_id );

            pb_num ++; // �^�[�Q�b�g�ڕW���X�V

            if( pb_num == needed_count )
            {// �\�����܂����̂ŏI��

                // ���YLC�ɂ��Ă͗\�蕪���܂����̂Ŕ�����
                goto __SEARCH_END;
            }
        }
    }
__SEARCH_END:

    return pb_num;
}

uint32_t SearchRcmTargetPage ( LP_INFO* l2p_info, PB_INFO* pb_info, uint32_t lptgt_list[MAX_REQ_PAGE_NUM], uint32_t needed_count )
{
    uint16_t ppo      = 0;
    uchar    bmap     = 0;
    uint32_t io_count = 0;

    uint16_t start_byte = (pb_info->rcm_search_pos) / ((sizeof(uchar)*8)); // �����J�n�ʒu�o�C�g��
    uint16_t start_offs = (pb_info->rcm_search_pos) % ((sizeof(uchar)*8)); // �����J�n�ʒu�I�t�Z�b�g

    uint16_t offs_count = start_offs;

    if ( needed_count > MAX_REQ_PAGE_NUM )
        needed_count = MAX_REQ_PAGE_NUM;

    for( uint16_t i = start_byte; i < VP_BITMAP_BYTES; i ++ )
    {
        bmap = pb_info->vp_bitmap[i]; // �����Ώۃr�b�g�}�b�v�擾
        ppo  = i * 8;                 // �J�n�y�[�W�I�t�Z�b�g�ԍ�

        while( bmap != 0 && io_count < needed_count )
        {

            // �ŏ��̃r�b�g�}�b�v�̈ʒu���킹�B
            if ( offs_count )
            {
                offs_count --;
                bmap = bmap >> 1;
                ppo++;
                continue;
            }

            uint32_t lpn = 0;

            if( bmap & 0x01 )
            {

                if( !GetLPNfromPPO_PBN( ppo, pb_info->pb_id, &lpn, l2p_info ) )  // �`�����N���I�t�Z�b�g�v�Z
                    return false;

                lptgt_list[io_count] = lpn;       // ���ʊi�[

                pb_info->rcm_search_pos = ppo + 1;// ���񌟍��J�n�ʒu

                io_count++;

            }
            bmap = bmap >> 1;
            ppo++;

        }

        offs_count = 0;

        // ���]�̐������擾�ł���΃u���C�N�B
        if( io_count == needed_count )
        {
            break;
        }
    }

    return io_count;
}

#endif
