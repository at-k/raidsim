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
    // 二重初期化防止
    if( l2p_tbl != NULL )
        return false;

    if( fm_info->GetTotalSector() < usr_area_sector_num )
    {// ユーザ領域のほうがでかい
        return false;
    }

    //-- 制御・構成パラメータの設定
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

    //-- 数量の設定
    {
        pb_num = fm_info->GetPBNum();   // 物理ブロック数取得
        pg_num = pb_num / pg_pb_num; // PG数設定
        pb_num -= ( (pb_num % pg_pb_num) != 0 ? (pb_num % pg_pb_num) : 0 ); //半端なブロック数は無かったことに。

        pp_num = fm_info->GetPPNum(); // 物理ページ数取得
        if( pp_num != pb_num * PP_PER_PB )
        {// 削った分の調整
            if( pp_num < pb_num * PP_PER_PB )
                return false;
            else
                pp_num = pb_num * PP_PER_PB;
        }

        lp_num   = (uint32_t)(usr_area_sector_num / SECTS_PER_PP); // 論理ページ数取得
        op_ratio = (double)((1 - (long double)lp_num / pp_num)*100);  // OP率計算
        FTL_PP_GADDR effective_pp_num = pp_num * (pg_pb_num - pg_parity_num ) / pg_pb_num;
        double real_op_ratio_tmp = (double)((1 - (long double)lp_num / effective_pp_num)*100);
        if( real_op_ratio_tmp < 0 )
        {
            PrintMessage( LOG_TYPE_ERROR, "Error InitFTL : 有効な物理アドレスが不足\n" );
            return false;
        }else
        {
            real_op_ratio = real_op_ratio_tmp;
        }

    }

    //-- 実体の作成
    {
        pb_list = new PB_INFO[ pb_num ];     // PB作成
        pg_list = new PG_INFO[ pg_num ];     // PG生成

        l2p_tbl = new VPA_INFO[ lp_num ]; // 逆変換テーブル作成

        memset( l2p_tbl, FTL_LP_NOT_ASSIGNED, sizeof(VPA_INFO)*lp_num ); // 未割で初期化
    }

    //-- アクティブPGの設定
    {
        //memset( &apg, 0, sizeof(ACT_PG_INFO) );
        // stl queが混じってるのでだめ。
        init_que_head( &apg.close_pg_que );
        memset( apg.host_openpg, 0, sizeof(PG_INFO*)*FTL_OPENPG_NUM );
        memset( apg.rcm_openpg,  0, sizeof(PG_INFO*)*FTL_OPENPG_NUM );
        memset( apg.wl_openpg, 0, sizeof(PG_INFO*)*FTL_OPENPG_WL_NUM );
    }

    //-- プールの設定
    {
        //memset( &pool, 0, sizeof(POOL_INFO) ); // とりあえず0クリア
        // non-PODなのでだめ。
        pool.pb_num = 0;
        pool.total_free_pb_count = 0;
        pool.total_invalid_pb_count = 0;
        pool.pre_free_select_ce = 0;
        pool.pre_invalid_select_ce = 0;

        //-- キューの作成 & 初期化
        // 空きキュー & 無効キュー
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
        {// 全資源空き登録
            pool.erase_resv_que.push_back( i );
            pool.erase_resv_list[i] = NULL;
        }
    }

    //-- パリティグループの設定
    {
        for( uint32_t pg = 0; pg < pg_num; pg++ )
        {
            PG_INFO* pg_info = &pg_list[pg];
            //memset( pg_info, 0, sizeof(PG_INFO) );
            // non-PODがメンバにいるのでmemsetはだめ。

            pg_info->id      = pg;
            pg_info->pb_list = new PB_INFO*[pg_pb_num];

            InitPG( pg_info );

            free_pg_que.push_back( pg_info );  // 空きPG資源を各プールに
        }
    }

    //-- interface初期化
    if( ftl_interface->Init( this, fm_info ) == false )
        return false;
    if( ftl_asyn_interface->Init( this, fm_info, ftl_interface ) == false )
        return false;

    //-- ブロックの設定
    {
        FMADDR pb_fm_addr;
        CEADDR ce_addr;

        for( uint32_t pb_no = 0; pb_no < pb_num; pb_no++ )
        {
            PB_INFO* pb_info = &pb_list[pb_no];

            memset( pb_info, 0, sizeof(PB_INFO) ); // 0クリア

            //-- 0クリア以外の設定
            //
            pb_info->id = pb_no;
            pb_info->status = FTL_PBS_FREE;
            pb_info->pg_no  = FTL_PG_INVALID_ID;

            fm_info->GetPBFMAddressFromGID( pb_no, &pb_fm_addr );         // 物理アドレス取得
            fm_info->GetCEAddressFromDieAddress( &pb_fm_addr, &ce_addr ); // CEのアドレス取得

            // 接続CE情報を設定
            pb_info->ce_no  = ce_addr.ce_id;
            pb_info->bus_no = pb_fm_addr.bus;

            //-- プールへ追加
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
    pg_info->p2l_tbl.clear();  // 逆引き情報クリア
    pg_info->next_rewr_index  = 0;
    pg_info->next_rewr_pgsect = 0;

    init_que( &pg_info->que ); // キュー初期化
    pg_info->que.data = (void*)pg_info;
}

