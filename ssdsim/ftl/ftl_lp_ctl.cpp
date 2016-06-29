
#include <nf_common.h>

#include "ftl_lp_ctl.h"
#include "ftl_lp_info.h"
#include "ftl_lp_func.h"
//#include "ftl_asyn_ctl.h"
#include "phy/fm_arch_info.h"

#include <algorithm>

FtlInterface::FtlInterface()
{
}
FtlInterface::~FtlInterface()
{
}

//--�I�[�v��PG�擾�֐�
//  ���ӓ_�F�擾�����OpenPG�͋󂫂��s�\����������Ȃ��iLength�Ɋւ��錟�؂����Ă��Ȃ��j
//          Length���s���(�Ⴆ�Έ��k�Ȃ�)�P�[�X�𓱓�����ꍇ�C�����C���K�v�B
FTL_PG_GADDR FtlInterface::GetOpenPG ( FTL_FMIO_TYPE type, FTL_LP_GADDR lp_no )
{
    static uchar hostpg_index = 0;
    static uchar rcmpg_index  = 0;

    FTL_PG_GADDR pg_no;

    if( type == FMT_HOST_WR )
    {// �z�X�g���C�g
        if( lp_info->apg.host_openpg[ hostpg_index ] == NULL )
        {
            lp_info->apg.host_openpg[ hostpg_index ] = BuildPG( type, hostpg_index );
            if( lp_info->apg.host_openpg[ hostpg_index ] == NULL ) // �擾���s
                return FTL_PG_INVALID_ID;
        }
        pg_no = lp_info->apg.host_openpg[ hostpg_index ]->id; // ���蓖��
        hostpg_index = ROUNDROBIN( hostpg_index, FTL_OPENPG_NUM );
    }
    else if( type == FMT_RCM_RWR )
    {// RCM�ɂ��ReWrite
        if( lp_info->apg.rcm_openpg[ rcmpg_index ] == NULL )
        {
            lp_info->apg.rcm_openpg[ rcmpg_index ] = BuildPG( type, rcmpg_index );
            if( lp_info->apg.rcm_openpg[ rcmpg_index ] == NULL ) // �擾���s
                return FTL_PG_INVALID_ID;
        }
        pg_no = lp_info->apg.rcm_openpg[ rcmpg_index ]->id; // ���蓖��
        rcmpg_index = ROUNDROBIN( rcmpg_index, FTL_OPENPG_NUM );
    }
    else
    {// RF�͖��T�|�[�g
        return FTL_PG_INVALID_ID;
    }

    return pg_no;
}


//-- �_���X�V
bool FtlInterface::L2P_Update ( FTL_FMIO_TYPE type, FTL_LP_GADDR lp_no, uchar len, FTL_PG_GADDR pg_no )
{
    PG_INFO* pg_dst_info;   // ������p���e�B�O���[�v
    PG_INFO* pg_src_info;   // �O�̃p���e�B�O���[�v
    //VPA_INFO vpa_dst;       // ����t���扼�z�A�h���X
    VPA_INFO vpa_src;       // �O�̉��z�A�h���X
    REVERSE_INFO rev_info;  // �t�����o�^���

    vpa_src = LpnToVpa( lp_no, lp_info );   // ���A�h���X�擾
    pg_dst_info = &lp_info->pg_list[pg_no]; // �������PG���擾

    if( vpa_src.pgn != FTL_PG_INVALID_ID )
    {// ���Ɋ��蓖�čς�
        pg_src_info = &lp_info->pg_list[vpa_src.pgn]; // ��PG���擾
    }else
    {
        pg_src_info = NULL;
    }

    // ---- �G���[����
    if( pg_dst_info->status != FTL_PGS_OPEN )
    {// Open���Ă��Ȃ�PG���w��
        PrintMessage( LOG_TYPE_ERROR, "Error L2P_Update: PG��Ԉُ�\n" );
        return false;
    } else if( pg_dst_info->next_ofs + len > lp_info->pg_cw_num )
    {// �������ޗ]�n�����u���b�N���w��
        PrintMessage( LOG_TYPE_ERROR, "Error L2P_Update: �󂫎c�ʕs�\��\n" );
        return false;
    } else if( pg_src_info != NULL && pg_src_info->vs_num < vpa_src.len )
    {// ��PG�̏�񂪃}�b�`���Ȃ�
        PrintMessage( LOG_TYPE_ERROR, "Error L2P_Update: ���������s����\n" );
        return false;
    }

    // ---- ������X�V
    lp_info->l2p_tbl[ lp_no ].pgn = pg_no;
    lp_info->l2p_tbl[ lp_no ].len = len;
    lp_info->l2p_tbl[ lp_no ].ofs = pg_dst_info->next_ofs;

    // �t�����쐬
    rev_info.lpn = lp_no;
    rev_info.len = len;

    // ������PG�̏��X�V
    pg_dst_info->p2l_tbl.push_back( rev_info ); // �t�����o�^
    pg_dst_info->next_ofs += len;
    pg_dst_info->vs_num += len; // �L���Z�N�^��
    pg_dst_info->lp_num ++;     // �Q�Ƙ_���y�[�W��

    if( pg_dst_info->next_ofs == lp_info->pg_cw_num )
    {// �Ō�܂ŏ������݊���
        if( OpenToClose( pg_dst_info ) == false )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error L2P_Update: �u���b�N���C�g���J�ڎ��s\n" );
            return false;
        }
    }

    // ---- �������X�V
    if( pg_src_info != NULL )
    {
        pg_src_info->vs_num -= vpa_src.len;
        pg_src_info->lp_num --;

        if( pg_src_info->vs_num != 0 )
        {
            if( pg_src_info->status == FTL_PGS_CLOSE )
            {// close PG
                if( UpdateCloseRanking( pg_src_info ) == false )
                {
                    PrintMessage( LOG_TYPE_ERROR, "Error L2p_Update: close�L���[�X�V���s\n");
                    return false;
                }
            }
        }else
        {
            if( pg_src_info->status != FTL_PGS_OPEN )
            {// �S������
                if( CloseToInvalid( pg_src_info ) == false )
                {
                    PrintMessage( LOG_TYPE_ERROR, "Error L2P_Update: close->invalid���s\n");
                    return false;
                }
            }
        }
    }


    /* �r�b�g�}�b�v�Ǘ��͍��͂��Ȃ� */

    return true;
}

bool FtlInterface::ErasePB ( FTL_ERASE_REQ erase_req )
{
    PB_INFO*   pb_info;   // �Ώۃu���b�N���

    pb_info = &lp_info->pb_list[erase_req.pb_id];

    if( pb_info != lp_info->pool.erase_resv_list[erase_req.reg_id] )
    {
        PrintMessage( LOG_TYPE_ERROR, "Error ErasePB : ���������s����\n" );
        return false;
    }

    // �������
    lp_info->pool.erase_resv_list[erase_req.reg_id] = NULL;
    lp_info->pool.erase_resv_que.push_back( erase_req.reg_id );

    if( pb_info->pg_no != FTL_PG_INVALID_ID )
    {// PG�ɏ������Ă���PB���I�΂�Ă���
        PrintMessage( LOG_TYPE_ERROR, "Error ErasePB: �s�K�ȃu���b�N\n" );
        return false;
    }

    // ���������X�V
    pb_info->erase_count++;
    // pb_info->last_erase_time =

    if( InvalidToFree( pb_info ) == false )
    {// �Ȃ񂩎��s
        return false;
    }

    return true;
}

bool FtlInterface::InvalidToFree ( PB_INFO* pb_info )
{
    if( pb_info->status != FTL_PBS_INVALID )
    {// ��Ԃ���������
        PrintMessage( LOG_TYPE_ERROR, "Error InvalidToFree: �u���b�N��Ԉُ�\n" );
        return false;
    }

    pb_info->status = FTL_PBS_FREE;

    // �t���[�L���[�փG���L���[, �Ƃ肠����FIFO...�����Ȃ񂩒m��Ȃ��B
    lp_info->pool.free_block_que[pb_info->bus_no][pb_info->ce_no].push_back(pb_info);

    pb_info->status = FTL_PBS_FREE;       // ��ԕύX
    lp_info->pool.total_free_pb_count ++; // �S�󂫃u���b�N�����X�V

    return true;
}

// �󂫃u���b�N����K�v�����擾���ăp���e�B�O���[�v���\��
// type�͍��̂Ƃ��떢�g�p
//
PG_INFO* FtlInterface::BuildPG ( FTL_FMIO_TYPE type, uint32_t id )
{
    PG_INFO* pg;
    PB_INFO* pb;

    if( lp_info->free_pg_que.empty() )
    {// ��PG������
        PrintMessage( LOG_TYPE_ERROR, "Error BuildPG: ��PG�͊�\n" );
        return false;
    }
    // �󂫂����o�� & �폜
    pg = lp_info->free_pg_que.front();
    lp_info->free_pg_que.pop_front(); // �L���[����폜
    pg->status = FTL_PGS_OPEN;        // ��ԕύX
    pg->opg_info.id = id;             // �I�[�v�����ǉ�
    pg->opg_info.type = type;

    // �u���b�N��肾��
    for( uint32_t i = 0; i < lp_info->pg_pb_num; i++ )
    {
        static int aho = 0;
        if( type == FMT_RCM_RWR )
            aho ++;
        if( aho == 8 )
            aho = 0;

        pb = GetFreePB( type );
        if( pb == NULL )
        {
            if( lp_info->pool.total_free_pb_count == 0 )
            {// �󂫃u���b�N�����C�u���b�N�͊�
                PrintMessage( LOG_TYPE_ERROR, "Error BuildPG: �󂫃u���b�N�͊�\n" );
            }else
            {
                PrintMessage( LOG_TYPE_ERROR, "Error BuildPG: �u���b�N�擾���s\n" );
            }
            return false;
        }
        if( pb->pg_no != FTL_PG_INVALID_ID || pb->status != FTL_PBS_FREE )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error BuildPG: �u���b�N��ԕs����\n");
            return false;
        }

        pg->pb_list[i] = pb;
        pb->pg_no = pg->id;
        pb->status = FTL_PBS_BIND;

    }

    return pg;
}

// �󂫃u���b�N��1�擾 �o�X�����Ƀ��E���h���r��
// type�͍��̂Ƃ��떢�g�p
PB_INFO* FtlInterface::GetFreePB( FTL_FMIO_TYPE type )
{
    static uint16_t nxt_bus = 0;
    static uint16_t nxt_ce  = 0;

    PB_INFO* pb_info = NULL;

    // �����J�n
    for( uint16_t k = 0; k < fm_info->GetCeNumPerBus() + 1; k++ )
    {// �S�Ă�CE�����Ƀ`�F�b�N( + �Ō�ɂ���1��)
        for( ; nxt_bus < fm_info->GetBusNum(); nxt_bus++ )
        {// CE���C���Ńo�X���`�F�b�N
            if( !lp_info->pool.free_block_que[nxt_bus][nxt_ce].empty() )
            {// �u���b�N����������

                // �u���b�N�̎��o�� �擪�u���b�N����擾
                pb_info = lp_info->pool.free_block_que[nxt_bus][nxt_ce].front();
                lp_info->pool.free_block_que[nxt_bus][nxt_ce].pop_front();

                lp_info->pool.total_free_pb_count --;

                nxt_bus = ROUNDROBIN( nxt_bus, fm_info->GetBusNum() );
                if( nxt_bus == 0 )
                    nxt_ce = ROUNDROBIN( nxt_ce, fm_info->GetCeNumPerBus() );
                goto __SEL_PB_END;
            }
        }

        nxt_bus = 0;
        nxt_ce = ROUNDROBIN( nxt_ce, fm_info->GetCeNumPerBus() );
    }

__SEL_PB_END:

    return pb_info;
}


bool FtlInterface::OpenToClose ( PG_INFO* pg_info )
{
    if( pg_info->status != FTL_PGS_OPEN )
    {
        PrintMessage( LOG_TYPE_ERROR, "Error OpenToClose: PG��ԕs����\n" );
        return false;
    }

    if( pg_info->opg_info.type == FMT_HOST_WR )
    {// �z�X�g���C�g�p
        if( lp_info->apg.host_openpg[pg_info->opg_info.id] != pg_info )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error OpenToClose: �Ώ�Host PG���s����\n" );
            return false;
        }

        // �I�[�v��PG�o�^����
        lp_info->apg.host_openpg[pg_info->opg_info.id] = NULL;
    }
    else if( pg_info->opg_info.type == FMT_RCM_RWR )
    {// �����C�g�p
        if( lp_info->apg.rcm_openpg[pg_info->opg_info.id] != pg_info )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error OpenToClose: �Ώ�RCM PG���s����\n" );
            return false;
        }

        // �I�[�v��PG�o�^����
        lp_info->apg.rcm_openpg[pg_info->opg_info.id] = NULL;
    }
    else
    {
        PrintMessage( LOG_TYPE_ERROR, "Error OpenToClose: �s���ȃ��C�g�^�C�v\n" );
        return false;
    }

    // �N���[�Y�o�^
    pg_info->status = FTL_PGS_CLOSE;
    if( UpdateCloseRanking( pg_info ) == false )
    {
        PrintMessage( LOG_TYPE_ERROR, "Error OpenToClose: close�L���[�X�V���s\n" );
        return false;
    }

    return true;
}

// rcm->invalid�ȊO�̃p�X�͍폜�B
bool FtlInterface::CloseToInvalid( PG_INFO* pg_info )
{
    if( (pg_info->status != FTL_PGS_RCM && pg_info->status != FTL_PGS_CLOSE) || pg_info->vs_num != 0 )
    {// !rcm or !close  or ���������L���f�[�^�����Ă�ꍇ
        PrintMessage( LOG_TYPE_ERROR, "Error CloseToInvalid: ��ԕs����\n" );
        return false;
    }

    // �u���b�N���&�����֒ǉ�
    PB_INFO* pb;
    for( uint32_t i = 0; i < lp_info->pg_pb_num; i++ )
    {
        pb = pg_info->pb_list[i];
        if( pb == NULL || pb->status != FTL_PBS_BIND )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error CloseToInvalid: �u���b�N��ԕs����\n");
            return false;
        }

        pg_info->pb_list[i] = NULL;
        pb->status = FTL_PBS_INVALID;
        pb->pg_no  = FTL_PG_INVALID_ID;

        lp_info->pool.invalid_block_que[pb->bus_no][pb->ce_no].push_back(pb);
        lp_info->pool.total_invalid_pb_count++;

    }

    //-- �L���[����ǂ��o��
    if( pg_info->status == FTL_PGS_RCM )
    {// rcm�L���[
        PG_STL_ITR it = find( lp_info->apg.rcm_cmp_pg_que.begin(), lp_info->apg.rcm_cmp_pg_que.end(), pg_info );
        if( it != lp_info->apg.rcm_cmp_pg_que.end() )
        {
	        lp_info->apg.rcm_cmp_pg_que.erase( it );
        }else
        {
            it = find( lp_info->apg.rcm_pg_que.begin(), lp_info->apg.rcm_pg_que.end(), pg_info );
            if( it != lp_info->apg.rcm_pg_que.end() )
            {
                lp_info->apg.rcm_pg_que.erase( it );
            }else
            {
                PrintMessage( LOG_TYPE_ERROR, "Error CloseToInvalid: �Ώ�(rcm) not found\n");
                return false;
            }
        }
    } else
    {// close�L���[
        if( pg_info->que.head != &lp_info->apg.close_pg_que )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error CloseToInvalid: �Ώ�(close) not found\n");
            return false;
        }
        remove_item( &lp_info->apg.close_pg_que, &pg_info->que );
    }

    // PG������
    lp_info->InitPG( pg_info );
    lp_info->free_pg_que.push_back( pg_info );  // ��PG�������e�v�[���ɒǉ�

    return true;
}

//-- �L���_���y�[�W���ׂĂ�1��IO�����{����
bool FtlInterface::Format()
{
    FTL_PG_GADDR addr;

    for( uint32_t lp_no = 0; lp_no < lp_info->lp_num; lp_no++ )
    {
        addr = GetOpenPG( FMT_HOST_WR, lp_no );
        if( addr == FTL_PG_INVALID_ID )
        {
            return false;
        }

        if ( L2P_Update( FMT_HOST_WR, lp_no, FTL_SECTS_PER_LP, addr) == false )
        {
            return false;
        }
    }

    return true;
}

//-- �v�[���Ƀu���b�N�ǉ�
bool FtlInterface::InitialAdd ( POOL_INFO* pool_info, PB_INFO* pb_info )
{
    // PB�������J�E���g
    pool_info->pb_num++;

    //-- �t���[�L���[�փG���L���[
    pool_info->free_block_que[pb_info->bus_no][pb_info->ce_no].push_back(pb_info);
    pool_info->total_free_pb_count ++; // �S�󂫃u���b�N�����X�V

    return true;
}

bool FtlInterface::UpdateCloseRanking( PG_INFO* pg_info )
{
    QUEUE*   pos;
    PG_INFO* pg_tmp;

    // �}���\�[�g
    if( pg_info->que.head == NULL )
    {// ��ԑJ�ڒ���
        pos = begin_item( &lp_info->apg.close_pg_que );
    }else
    {// �ڑ���̍X�V
        pos = que_back( &pg_info->que );
        remove_item( &lp_info->apg.close_pg_que, &pg_info->que );
    }

    while( !is_que_end( pos ) )
    {// ���������~���̃L���[���T�[�`(���̕���������������)
        pg_tmp = (PG_INFO*)get_data(pos);
        if( pg_info->vs_num >= pg_tmp->vs_num )
            break;

        pos = que_back( pos );
    }

    insert_forward( &lp_info->apg.close_pg_que, pos, &pg_info->que );

#if 0
    uint16_t invalid_page_ratio;
    uint16_t rank;

    if ( (pb_info->que->head == &pool_info->rcm_block_que) ||
         (pb_info->que->head == &pool_info->rcm_comp_wait_que) )
    {// RCM�Ώۃu���b�N�̓N���[�Y�ɓ����Ă��Ȃ��B
        return true;
    }

    // �����o��
    remove_item( pb_info->que->head, pb_info->que );

    // �������̌v�Z�C100�{���Ċ���B
    invalid_page_ratio = (100 * ( PP_PER_PB - pb_info->vp_count )) / PP_PER_PB;

    // �v�[���RCM�ɂ����Ė��������ɒ[�ɍ����u���b�N�͂Ȃ�
    // �Ȃ̂ŁC�������Ⴂ���ōׂ���������
    if( invalid_page_ratio > 50 )
        rank = 0;
    else if ( invalid_page_ratio > 40 )
        rank = 1;
    else if ( invalid_page_ratio > 30 )
        rank = 2;
    else if ( invalid_page_ratio > 20 )
        rank = 3;
    else if ( invalid_page_ratio > 15 )
        rank = 4;
    else if ( invalid_page_ratio > 10 )
        rank = 5;
    else if ( invalid_page_ratio > 5 )
        rank = 6;
    else
        rank = 7;

    // �Ώۃ����N�̃L���[�ɃG���L���[����
    insert_end( &pool_info->close_block_que[rank], pb_info->que );
#endif

    return true;
}

#if 0
void SetPBasRcmTarget ( LP_INFO* l2p_info, uint32_t pb_no )
{

    PB_INFO*   pb_info;
    POOL_INFO* pool_info;

    // �Ώۂ̏��擾
    pb_info = &l2p_info->pb_list[pb_no];
    pool_info = pb_info->pool;

    if ( pb_info->que->head == &pool_info->rcm_block_que )
    {// ���Ƀu���b�N�o�^�ς�
        return;
    }

    // RCM�Ώۂ̓N���[�Y�L���[������o��
    remove_item( pb_info->que->head, pb_info->que );

    // �o�^
    insert_end( &pool_info->rcm_block_que, pb_info->que );

    // ���ϒǉ����L���y�[�W�����X�V
    pool_info->initial_vp_count =
        (pool_info->initial_vp_count * pool_info->rcm_block_que.count + pb_info->vp_count)
            / (pool_info->rcm_block_que.count + 1 );


    // RCM�����ʒu��������
    pb_info->rcm_search_pos = 0;

    return;
}

bool SetPBasRcmCompWait( LP_INFO* l2p_info, uint32_t pb_no )
{
    PB_INFO*   pb_info;
    POOL_INFO* pool_info;
    QUEUE*     que_entry;

    // �Ώۂ̏��擾
    pb_info = &l2p_info->pb_list[pb_no];
    pool_info = pb_info->pool;

    que_entry = pb_info->que;

    remove_item( &pool_info->rcm_block_que, que_entry ); // �u���b�N��RCM�L���[������o��

    // �҂��L���[�֒ǉ�
    insert_front( &pool_info->rcm_comp_wait_que, que_entry );

    return true;
}
#endif
