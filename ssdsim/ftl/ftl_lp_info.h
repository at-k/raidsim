#ifndef LDSTRUCT_H
#define LDSTRUCT_H

#include <limits.h>
#include <deque>
#include <vector>

#include "nf_common.h"
#include "phy/fm_macro_def.h"
#include "pbque.h"

//-- FTL data structure- --
//-- definition
// HLBA: Host Logical Block Address : host address
// LPN : Logical Page Number        : corresponding to host address
// PPN : Physical Page Number       : unique physical page number
// PPO : Physical Page Offset       : offset page number in physical block
// VPA : Virtual Page Address       : virtual page number (32bit: PG#, 24bit: PG Offset Sector, 8bit: Length)


//-- macro definition
#define FTL_LP_NOT_ASSIGNED    0xFFFFFFFF      // map value for not assigned logical page
#define FTL_PG_INVALID_ID      0xFFFFFFFF      // invalid PG number
#define FTL_REWR_END           0xFFFFFFFF      // end rewrite

#define FTL_PG_BLOCK_NUM      32               // Block RAID(N in NDxP)
#define FTL_PG_PARITY_NUM     1                // Block RAID(x in NDxP)
#define FTL_SECTS_PER_LP      16               // LP sector size

#define FTL_OPENPG_NUM        1	               // open pg number
#define FTL_OPENPG_WL_NUM     7                // open pg for special

#define FTL_MAX_REWR_REQ      16               // maximum GC request
#define FTL_MAX_ERASE_REQ     4                // maximum erase requesst
#define FTL_MAX_ERASE_REG     256              // maximum erase reservation


//-- declaration
class  FM_INFO;
class  FtlInterface;
class  FtlAsynReqInterface;

//-- type declaration
//status
enum FTL_PG_STATUS
{
    FTL_PGS_FREE,
    FTL_PGS_OPEN,
    FTL_PGS_CLOSE,
    FTL_PGS_RCM
};
enum FTL_PB_STATUS
{
    FTL_PBS_FREE,
    FTL_PBS_BIND,   // added to PG
    FTL_PBS_INVALID
};

//address space definition
typedef uint32_t FTL_LP_GADDR;
typedef uint32_t FTL_PG_GADDR;
typedef uint32_t FTL_PB_GADDR;
typedef uint32_t FTL_PP_GADDR;
typedef uint32_t FTL_PG_OADDR; // pg offset address
typedef uint16_t FTL_PB_OADDR; // pb offset address

//-- IO request
enum FTL_FMIO_TYPE
{
    FMT_HOST_WR, // host request write
    FMT_HOST_RD, // host request read
    FMT_RCM_RWR, // block reclamation
    FMT_REF_RWR, // RF
    FMT_BLK_ERS, // erase
    FMT_DEFAULT  // invalid
};

//-- struct definition
// ftl option
typedef struct {
    bool      enable_pg_composition;
    uint32_t  pg_pb_num;
    uint32_t  pg_parity_num;
    bool      enable_rcm_th;
    uint32_t  rcm_th;
} FTL_EXT_OPT;

// open pg information
typedef struct {
   FTL_FMIO_TYPE type; // open type
   uchar           id; // registered ID(to specify array position)
} FTL_OPENPG_INFO;

// block erase reservation information
typedef struct {
    FTL_PB_GADDR pb_id;  // block address
    uint32_t     reg_id; // reservation ID
} FTL_ERASE_REQ;

// virtual page address
typedef struct {
    FTL_PG_GADDR pgn;
    FTL_PG_OADDR ofs;
    uchar        len;
    uchar        rcm_count; // option for wearleveling
} VPA_INFO;

// reverse map
typedef struct {
    FTL_LP_GADDR lpn;
    uchar        len;
} REVERSE_INFO;

// physical block information
typedef struct __PB_INFO {
    FTL_PB_GADDR  id;          // unique block id
    FTL_PB_STATUS status;

    FTL_PG_GADDR  pg_no;       // belonging PG no

    uint32_t  erase_count;     // cumulative erase count
    uint64_t  last_erase_time;

    //-- physical position information
    uint16_t  ce_no;  // CE no
    uint16_t  bus_no; // bus no

} PB_INFO;

// PG(parity group) information
typedef struct __PG_INFO {
    FTL_PG_GADDR  id;        // unique ID
    FTL_PG_STATUS status;

    uchar         attr;      // open attribution

    FTL_PG_OADDR  vs_num;    // valid secotr num
    FTL_LP_GADDR  lp_num;    // valid LP num

    FTL_OPENPG_INFO  opg_info;  // reference for open pg information
    FTL_PG_OADDR     next_ofs;  // next write position
    PB_INFO**        pb_list;   // block information

    std::deque<REVERSE_INFO> p2l_tbl; // reverse map
    FTL_PG_OADDR  next_rewr_index;    // for gc: next copy target
    FTL_PG_OADDR  next_rewr_pgsect;   // for gc: next copy target sector

    QUEUE  que; // close que
} PG_INFO;

//-- �󂫃u���b�N�I��p�C�O��܂ł̃u���b�N�I�����
typedef struct {
    uint16_t      cur_ce;  // ���݂̑I��CE
    uint16_t      ce_count;// ���܂ł̘A���I����
    uint16_t      next_select_bus_offset; // �O��I�������o�X�I�t�Z�b�g
} LAST_PB_INFO;

//-- �L���[�p�^
typedef std::deque<PB_INFO*> PB_STL_QUE;
typedef std::deque<PG_INFO*> PG_STL_QUE;
typedef PB_STL_QUE::iterator PB_STL_ITR;
typedef PG_STL_QUE::iterator PG_STL_ITR;
typedef QUEUE_HEAD           PG_UTL_QUE;

//-- �v�[��
//   �󂫃u���b�N�Ɩ����u���b�N�̊Ǘ�
typedef struct __POOL_INFO {

    FTL_PB_GADDR  pb_num;

    FTL_PB_GADDR  total_free_pb_count;    // �S�󂫃u���b�N��
    FTL_PB_GADDR  total_invalid_pb_count; // �L���[���̑S�����u���b�N��
    uint16_t  pre_free_select_ce;     // �O��󂫃u���b�N�I��CE
    uint16_t  pre_invalid_select_ce;  // �O�񖳌��u���b�N�I��CE

    //FTL_LP_GADDR  initial_vp_count;  // �����L���y�[�W��
    //FTL_PB_GADDR  rcm_tgt_pb_num;    // RCM�Ώۃu���b�N��

    // �u���b�N��ԊǗ��L���[
    uint16_t  que_bus_num;
    uint16_t  que_ce_num;

    PB_STL_QUE**  free_block_que;    // �󂫃u���b�N�L���[[BUS][CE]
    PB_STL_QUE**  invalid_block_que; // �����u���b�N�L���[[BUS][CE]

    std::deque<uint32_t>  erase_resv_que;         // �󂫏����\�񎑌��L���[
    PB_INFO*  erase_resv_list[FTL_MAX_ERASE_REG]; // �����\�񎑌�

} POOL_INFO;

//-- Active��PG���Ǘ�
//   ������Ǘ��CRCM�ΏۊǗ��C�Ȃ�
typedef struct __ACT_PG_INFO{

    // �L���[
    //PG_STL_QUE    close_pg_que;      // �L���f�[�^�u���b�N������PG
    PG_UTL_QUE    close_pg_que;
    PG_STL_QUE    rcm_pg_que;        // RCM�Ώۂ�PG
    PG_STL_QUE    rcm_cmp_pg_que;    // RCM�����҂���PG

    // �I�[�v���p���e�B�O���[�v
    PG_INFO*  host_openpg[FTL_OPENPG_NUM]; // �z�X�g�p
    PG_INFO*  rcm_openpg[FTL_OPENPG_NUM];  // ���N�����[�V�����p

    PG_INFO*  wl_openpg[FTL_OPENPG_WL_NUM]; // WL���ؗp�I�[�v��PG

} ACT_PG_INFO;

//-- FTL�̃f�[�^�\�����ꊇ�Ǘ�
//
class LP_INFO {
public:
    LP_INFO();
    ~LP_INFO();

    bool InitFTL( FM_INFO* fm_info, uint64_t usr_area_sector_num,
        FtlInterface* ftlinterface, FtlAsynReqInterface* ftl_asyn_if , FTL_EXT_OPT* option = NULL );

    void InitPG( PG_INFO* pg);

	// pool & pg & pb
    FTL_PG_GADDR   pg_num;
    FTL_PB_GADDR   pb_num;

    POOL_INFO   pool;    // pool��1�B
    ACT_PG_INFO apg;     // ������Ǘ���1�B
    PG_INFO*    pg_list;
    PB_INFO*    pb_list;

    // �p���e�B�O���[�v��ԊǗ��L���[
    PG_STL_QUE    free_pg_que;       // ��PG�L���[(�_�������Ȃ̂œ��Ƀ\�[�g�s�v)

    // �_���E�����y�[�W��
    FTL_LP_GADDR   lp_num;
    FTL_PP_GADDR   pp_num;
    VPA_INFO*      l2p_tbl;  // �_��

    long double    op_ratio;      // over provisioning ratio (�z�X�g���_)
    long double    real_op_ratio; // ����RAID���̎���OP
    uint32_t       rcm_th;        // rcm�J�n臒l(�X�V�̈�ɑ΂���100����)
    uint32_t       erase_th;      // �����Ώۃu���b�N����臒l

    uint32_t       pg_pb_num;     // pg�ӂ�pb��
    uint32_t       pg_parity_num; // pg�ӂ�parity�u���b�N��
    uint32_t       pg_cw_num;     // pg�ӂ�cw��

};

#endif
