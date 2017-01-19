#ifndef ASYN_IO_MNG_H
#define ASYN_IO_MNG_H

//-- 非同期I/O（RCM & RF）...とりあえずRFは保留
//   状態遷移系はやらない，RCM対象PB/(LC)の決定と，ページコピーコマンド生成のみ
//   というか基本的にパラメータの変更はやらない

#include "../phy/fm_arch_info.h"
#include "ftl_lp_info.h"

// 消去・RCMなどの非同期要求関係
class FtlAsynReqInterface
{
public:
	virtual ~FtlAsynReqInterface(){}

    virtual bool Init( LP_INFO* _lp_info, FM_INFO* _fm_info, FtlInterface* _ftl_if );

    //-- RCMによるIO要求を刈り取る
    //   RCM実施する場合はここからブロックを選ぶ。実際のコピーは上にお任せ
    //   返り値は要求数
    int GetReWriteReq( REVERSE_INFO req_list[FTL_MAX_REWR_REQ] );

    //-- Erase要求刈り取り
    //   刈り取らないと空き不足で死ぬ
    //   返り値は要求数
    int GetEraseReq( FTL_ERASE_REQ erase_req[FTL_MAX_ERASE_REQ] );

protected:
    LP_INFO* lp_info;
    FM_INFO* fm_info;

    FTL_PB_GADDR rcm_start_th;
    //FTL_PB_GADDR pre_free_pb_num;

    int GetNeededBlockNum();

    //-- CloseキューからRCM対象PGを探して
    //   RCM対象キューに遷移させる
    virtual bool ReloadRcmTarget( int tgt_num );

    REVERSE_INFO GetNextReWriteLPN( PG_INFO* pg_info );

};


#endif
