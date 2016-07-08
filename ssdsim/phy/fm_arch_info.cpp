#include "fm_arch_info.h"
#include "fm_macro_def.h"
#include <nf_common.h>

FM_INFO::FM_INFO()
{
    bus_info = NULL;
    ce_info = NULL;

    bus_num = 0;
    pb_num = 0;
    pp_num = 0;
}

FM_INFO::~FM_INFO()
{
    if( bus_info != NULL )
    {
        for( uint32_t i = 0; i < bus_num ; i++ )
        {
            for( uint32_t j = 0; j < chip_per_bus ; j++ )
            {
                if( bus_info[i].clist[j].dlist!= NULL)
                {
                    if( die_per_chip > 1 )
                        delete[] bus_info[i].clist[j].dlist;
                    else
                        delete bus_info[i].clist[j].dlist;
                }
            }
            if( bus_info[i].clist != NULL )
                delete[] bus_info[i].clist;
        }
        delete[] bus_info;
    }

    if( ce_info != NULL )
    {
        for( uint32_t i = 0; i < dma_num; i++ )
        {
            delete[] ce_info[i];
        }
        delete[] ce_info;
    }
}

bool FM_INFO::InitFlashModule(
    const uint16_t  bus_num_i,
    const uint16_t  chip_per_bus_i,
    const uint16_t  die_per_chip_i,
    const uint64_t  byte_per_chip_i,
    const uint16_t  dma_num_i,
    const uint16_t  ce_per_bus_i )
{
    // 0値チェック
    if( bus_num_i == 0 || chip_per_bus_i == 0 || die_per_chip_i ==0 || byte_per_chip_i == 0 || dma_num_i == 0 || ce_per_bus_i == 0 ) {
		printf("%d, %d,%d,%ld,%d,%d,\n", bus_num_i, chip_per_bus_i, die_per_chip_i, byte_per_chip_i, dma_num_i, ce_per_bus_i );

        ERR_AND_RTN;
	}
    // 既に初期化されているかチェック
    if( bus_info != NULL )
        ERR_AND_RTN;

    bus_num = bus_num_i;
    dma_num = dma_num_i;

    chip_per_bus = chip_per_bus_i;
    die_per_chip = die_per_chip_i;

    // トータル容量(GB)計算
    fm_byte = byte_per_chip_i * chip_per_bus * bus_num;
    if( sizeof(size_t) == 4 && (fm_byte / 1024 / 1024 / 1024 ) > MAX_FM_GBSIZE )
    {
        PrintMessage( LOG_TYPE_ERROR, "Erorr at FM_INFO::InitFlashModule -- FM Size is too large. Maximum support size is 2048GB -- input size is %d Byte\n", fm_byte);
        ERR_AND_RTN;
    }

    // トータル容量が適正か確認
    uint64_t tmp = fm_byte;
    while( tmp != 0 && tmp != 2)
    {// fm_byteが2^x byteでなかったらfalse
        if( (tmp & 0x1) != 0 )
        {
            PrintMessage( LOG_TYPE_ERROR, "Erorr at FM_INFO::InitFlashModule -- 不正な容量です 2の累乗になる用に設定してください-- input size is %d Byte\n", fm_byte);
            ERR_AND_RTN;
        }
        tmp = tmp >> 1;
    }

    // バス作成
    bus_info = new BUS_INFO[bus_num];

    // 全セクタ数を計算
    fm_sects = BYTE2SECTOR( fm_byte );
    // ページ数で割って端数が出れば割りきれるように調製
    fm_sects += (( fm_sects % SECTS_PER_PP == 0) ? 0:(SECTS_PER_PP - fm_sects % SECTS_PER_PP));

    // 物理ブロック・ページ数計算
    pb_num = (uint32_t)( fm_sects / SECTS_PER_PB );
    pp_num = pb_num * PP_PER_PB;

    // ～あたりブロック数計算
    pb_per_bus  = pb_num / bus_num;
    pb_per_chip = pb_num / (bus_num * chip_per_bus);
    pb_per_die  = pb_num / (bus_num * chip_per_bus * die_per_chip);

    //-- chip enable 構成記述
    if( bus_num < dma_num )
    {
        PrintMessage( LOG_TYPE_ERROR, "Erorr at FM_INFO::InitFlashModule -- バス数がDMA数より少なく設定されています\n" );
        ERR_AND_RTN;
    }

    if( bus_num % dma_num != 0 )
    {
        PrintMessage( LOG_TYPE_ERROR, "Erorr at FM_INFO::InitFlashModule -- バス数はDMA数で割り切れる必要があります\n" );
        ERR_AND_RTN;
    }

    ce_per_bus = ce_per_bus_i;  // バスあたりのCE数数設定（CEの縦方向の数）

    if( ce_per_bus > chip_per_bus * die_per_chip )
    {
        PrintMessage( LOG_TYPE_ERROR, "Erorr at FM_INFO::InitFlashModule -- バスあたりのCE数がバスあたりのダイ数を越えています\n" );
        ERR_AND_RTN;
    }

    if( ( die_per_chip * chip_per_bus ) % ce_per_bus )
    {
        PrintMessage( LOG_TYPE_ERROR, "Erorr at FM_INFO::InitFlashModule -- バスあたりのダイ数をCE数で割り切れません\n");
        ERR_AND_RTN;
    }

    // バスあたりのダイ数をバスあたりCE数で割る
    die_per_ce_per_bus = (die_per_chip * chip_per_bus) / ce_per_bus;

    // CE作成
    ce_info = new CE_INFO*[dma_num];

    for( uint16_t i = 0; i < dma_num ; i++ )
    {
        ce_info[i] = new CE_INFO[ce_per_bus];
        for( uint16_t k = 0; k < ce_per_bus; k++ )
        {
            ce_info[i][k].id     = k;
            ce_info[i][k].status = CE_IDLE;
        }
    }

    //-- バス，チップ，ダイの実体作成 & CE実体作成
    for( uint16_t i = 0; i < bus_num ; i++ )
    {
        bus_info[i].clist  = new CHIP_INFO[chip_per_bus];

        for( uint32_t j = 0; j < chip_per_bus ; j++ )
        {
            bus_info[i].clist[j].dlist = new DIE_INFO[die_per_chip];

            /*for( uint32_t k = 0; k < die_per_chip ; k++ )
            {
                bus_info[i].clist[j].dlist[k].pblist = new PB_INFO[pb_per_die];
            }*/
        }
    }

    //-- IDの設定 ex..(BusID, ChipID, DieID, PBID)←これはユニーク。FMADDRからppo(ページ位置）を除いた値

    // 設定開始
    for( uint32_t i = 0; i < bus_num ; i++ )
    {
        bus_info[i].id = i;    // ID設定
        bus_info[i].status = BUS_IDLE; // バスの初期状態はIDLE

        for( uint32_t j = 0; j < chip_per_bus ; j++ )
        {
            bus_info[i].clist[j].id = j; // ID設定

            for( uint32_t k = 0; k < die_per_chip ; k++ )
            {
                bus_info[i].clist[j].dlist[k].id       = k;
                bus_info[i].clist[j].dlist[k].status   = DIE_IDLE;
                bus_info[i].clist[j].dlist[k].next_die = NULL;

                bus_info[i].clist[j].dlist[k].bus  = i;
                bus_info[i].clist[j].dlist[k].chip = j;

            }
        }
    }

    return true;
}

