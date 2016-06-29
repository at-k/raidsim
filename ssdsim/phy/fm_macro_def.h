#ifndef __FM_DEFINITION_H
#define __FM_DEFINITION_H

// -- FMチップの性能関係パラメータ
#define FMDIE_READ_BUSY_TIME_US     50 // read:  die op time (page->register)
#define FMDIE_WRITE_BUSY_TIME_US  1400 // write: die op time (page->register)
#define FMDIE_REGREAD_BUSY_TIME_US   1 // reg read: die op time
#define FMDIE_ERASE_BUSY_TIME_US  5000 // erase: die op time
#define FMBUS_TX_BUSY_TIME_US       59 // read/write: register->controller(bus op time)
#define FMBUS_COM_TX_BUSY_TIME_US    1 // read/eraseの最初の1回目,writeの最後のリターン

// -- バイト定義
#define BYTES_PER_SECTOR           512                                // 1セクタあたりのバイト数
#define BYTE2SECTOR(x) ( (unsigned long long int)(x) / BYTES_PER_SECTOR ) // バイト→セクタ
#define GB2SECTOR(x)   ( (unsigned long long int)(x)*(1024*1024*1024/ BYTES_PER_SECTOR) ) // ギガバイト→セクタ
#define SECTOR2BYTE(x) ( (x) * BYTES_PER_SECTOR )                     // セクタ→バイト
#define SECTOR2GB(x)   ( (SECTOR_BYTES*( (x) / 1024)/1024) / 1024 )   // セクタ→ギガバイト

// -- 物理ページ構成
#define SECTS_PER_PP               16                   // 1物理ページあたりのセクタ数
#define BYTES_PER_PP     ( BYTES_PER_SECTOR*SECTS_PER_PP )  // 1物理ページあたりのバイト数
#define PP_PER_PB     256                               // 1物理ブロックあたりのページ数
#define SECTS_PER_PB      ( SECTS_PER_PP*PP_PER_PB )    // 1物理ブロックあたりのセクタ数
#define BYTES_PER_PB      ( BYTES_PER_PP*PP_PER_PB )    // 1物理ブロックあたりのバイト数

// -- FM構成に関するDefault設定
#define DEF_BUS_NUM           8  // バス数
#define DEF_CHIP_PER_BUS      2  // バスあたりのチップ数
#define DEF_DIE_PER_CHIP      4  // チップあたりのダイ数
#define DEF_SISE_GB_PER_CHIP  1  // チップあたりのサイズ(GByte)

// -- CE構成に関するDefault設定
#define DEF_DMA_NUM     4 // CEの横方向（バス方向）の数
#define DEF_CE_PER_BUS  4 // CEの縦方向（チップ方向）の数

// -- CWに関するパラメータ
#define SECTS_PER_CW    1  // Code Wordサイズ = 圧縮時最小管理単位
#define CW_PER_PP      (SECTS_PER_PP / SECTS_PER_CW )

// -- 圧縮に関するパラメータ
#define MAX_COMP_RATIO 16                // 最大圧縮率

// -- 64bit対応
#ifdef x64
#define MAX_FM_GBSIZE 16384 // 64bit版のチップあたり最大GBサイズ
#else
#define MAX_FM_GBSIZE 2048  // 32bit版のチップあたり最大GBサイズ
#endif

// --- コンパイル前のチェック
//- ブロックあたりページ数が8で割り切れるか
#if (PP_PER_PB % 8 != 0)
#error Error!! PP_PER_PB is not byte multiple ( PP_PER_PB % 8 != 0 )
#endif

#endif
