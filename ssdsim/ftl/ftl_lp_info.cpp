#include "ftl_lp_info.h"
#include "ftl_lp_ctl.h"
#include "phy/fm_arch_info.h"

#include <stdlib.h>
#include <string.h>

LP_INFO::LP_INFO()
{
    pb_list   = NULL;
    pg_list   = NULL;
    l2p_tbl   = NULL;
}

LP_INFO::~LP_INFO()
{

}

bool LP_INFO::InitFTL( FM_INFO* fm_info, uint64_t usr_area_sector_num,
                      FtlInterface* ftl_interface, FtlAsynReqInterface* ftl_asyn_interface,
                      FTL_EXT_OPT* option )
{
    // ��d�������h�~
    if( l2p_tbl != NULL )
        return false;

    if( fm_info->GetTotalSector() < usr_area_sector_num )
    {// ���[�U�̈�̂ق����ł���
        return false;
    }

    //-- ����E�\���p�����[�^�̐ݒ�
    {
        // load default settings
        pg_pb_num = FTL_PG_BLOCK_NUM;
        pg_parity_num = FTL_PG_PARITY_NUM;
        pg_cw_num = ((pg_pb_num - pg_parity_num) * PP_PER_PB * CW_PER_PP);
        rcm_th    = 0; // minimum

        if( option != NULL )
        {
            if( option->enable_pg_composition )
            {
                pg_pb_num = option->pg_pb_num;
                pg_parity_num = option->pg_parity_num;
                pg_cw_num = ((pg_pb_num - pg_parity_num) * PP_PER_PB * CW_PER_PP);
            }

            if( option->enable_rcm_th )
            {
                rcm_th = option->rcm_th;
            }
        }
    }

    //-- ���ʂ̐ݒ�
    {
        pb_num = fm_info->GetPBNum();   // �����u���b�N���擾
        pg_num = pb_num / pg_pb_num; // PG���ݒ�
        pb_num -= ( (pb_num % pg_pb_num) != 0 ? (pb_num % pg_pb_num) : 0 ); //���[�ȃu���b�N���͖����������ƂɁB

        pp_num = fm_info->GetPPNum(); // �����y�[�W���擾
        if( pp_num != pb_num * PP_PER_PB )
        {// ��������̒���
            if( pp_num < pb_num * PP_PER_PB )
                return false;
            else
                pp_num = pb_num * PP_PER_PB;
        }

        lp_num   = (uint32_t)(usr_area_sector_num / SECTS_PER_PP); // �_���y�[�W���擾
        op_ratio = (double)((1 - (long double)lp_num / pp_num)*100);  // OP���v�Z
        FTL_PP_GADDR effective_pp_num = pp_num * (pg_pb_num - pg_parity_num ) / pg_pb_num;
        double real_op_ratio_tmp = (double)((1 - (long double)lp_num / effective_pp_num)*100);
        if( real_op_ratio_tmp < 0 )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error InitFTL : �L���ȕ����A�h���X���s��\n" );
            return false;
        }else
        {
            real_op_ratio = real_op_ratio_tmp;
        }

    }

    //-- ���̂̍쐬
    {
        pb_list = new PB_INFO[ pb_num ];     // PB�쐬
        pg_list = new PG_INFO[ pg_num ];     // PG����

        l2p_tbl = new VPA_INFO[ lp_num ]; // �t�ϊ��e�[�u���쐬

        memset( l2p_tbl, FTL_LP_NOT_ASSIGNED, sizeof(VPA_INFO)*lp_num ); // �����ŏ�����
    }

    //-- �A�N�e�B�uPG�̐ݒ�
    {
        //memset( &apg, 0, sizeof(ACT_PG_INFO) );
        // stl que���������Ă�̂ł��߁B
        init_que_head( &apg.close_pg_que );
        memset( apg.host_openpg, 0, sizeof(PG_INFO*)*FTL_OPENPG_NUM );
        memset( apg.rcm_openpg,  0, sizeof(PG_INFO*)*FTL_OPENPG_NUM );
        memset( apg.wl_openpg, 0, sizeof(PG_INFO*)*FTL_OPENPG_WL_NUM );
    }

    //-- �v�[���̐ݒ�
    {
        //memset( &pool, 0, sizeof(POOL_INFO) ); // �Ƃ肠����0�N���A
        // non-POD�Ȃ̂ł��߁B
        pool.pb_num = 0;
        pool.total_free_pb_count = 0;
        pool.total_invalid_pb_count = 0;
        pool.pre_free_select_ce = 0;
        pool.pre_invalid_select_ce = 0;

        //-- �L���[�̍쐬 & ������
        // �󂫃L���[ & �����L���[
        pool.que_bus_num = fm_info->GetBusNum();
        pool.que_ce_num  = fm_info->GetCeNumPerBus();

        pool.free_block_que    = new PB_STL_QUE*[ pool.que_bus_num ];
        pool.invalid_block_que = new PB_STL_QUE*[ pool.que_bus_num ];

        for( uint16_t bus = 0; bus < pool.que_bus_num; bus++ )
        {
            pool.free_block_que[bus]    = new PB_STL_QUE[ pool.que_ce_num ];
            pool.invalid_block_que[bus] = new PB_STL_QUE[ pool.que_ce_num ];
        }

        for( uint32_t i = 0; i < FTL_MAX_ERASE_REG; i++ )
        {// �S�����󂫓o�^
            pool.erase_resv_que.push_back( i );
            pool.erase_resv_list[i] = NULL;
        }
    }

    //-- �p���e�B�O���[�v�̐ݒ�
    {
        for( uint32_t pg = 0; pg < pg_num; pg++ )
        {
            PG_INFO* pg_info = &pg_list[pg];
            //memset( pg_info, 0, sizeof(PG_INFO) );
            // non-POD�������o�ɂ���̂�memset�͂��߁B

            pg_info->id      = pg;
            pg_info->pb_list = new PB_INFO*[pg_pb_num];

            InitPG( pg_info );

            free_pg_que.push_back( pg_info );  // ��PG�������e�v�[����
        }
    }

    //-- interface������
    if( ftl_interface->Init( this, fm_info ) == false )
        return false;
    if( ftl_asyn_interface->Init( this, fm_info, ftl_interface ) == false )
        return false;

    //-- �u���b�N�̐ݒ�
    {
        FMADDR pb_fm_addr;
        CEADDR ce_addr;

        for( uint32_t pb_no = 0; pb_no < pb_num; pb_no++ )
        {
            PB_INFO* pb_info = &pb_list[pb_no];

            memset( pb_info, 0, sizeof(PB_INFO) ); // 0�N���A

            //-- 0�N���A�ȊO�̐ݒ�
            //
            pb_info->id = pb_no;
            pb_info->status = FTL_PBS_FREE;
            pb_info->pg_no  = FTL_PG_INVALID_ID;

            fm_info->GetPBFMAddressFromGID( pb_no, &pb_fm_addr );         // �����A�h���X�擾
            fm_info->GetCEAddressFromDieAddress( &pb_fm_addr, &ce_addr ); // CE�̃A�h���X�擾

            // �ڑ�CE����ݒ�
            pb_info->ce_no  = ce_addr.ce_id;
            pb_info->bus_no = pb_fm_addr.bus;

            //-- �v�[���֒ǉ�
            if( ftl_interface->InitialAdd( &pool, pb_info ) == false )
                return false;
        }
    }

    return true;
}

void LP_INFO::InitPG( PG_INFO* pg_info )
{
    pg_info->status        = FTL_PGS_FREE;
    pg_info->vs_num        = 0;
    pg_info->lp_num        = 0;
    pg_info->opg_info.id   = 0;
    pg_info->opg_info.type = FMT_DEFAULT;
    pg_info->next_ofs      = 0;
    pg_info->attr = 0;

    memset( pg_info->pb_list, 0, sizeof(PB_INFO*)*pg_pb_num );
    pg_info->p2l_tbl.clear();  // �t�������N���A
    pg_info->next_rewr_index  = 0;
    pg_info->next_rewr_pgsect = 0;

    init_que( &pg_info->que ); // �L���[������
    pg_info->que.data = (void*)pg_info;
}

