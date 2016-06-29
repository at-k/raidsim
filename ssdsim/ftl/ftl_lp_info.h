#ifndef LDSTRUCT_H
#define LDSTRUCT_H

#include <limits.h>
#include <deque>
#include <vector>

#include "nf_common.h"
#include "phy/fm_macro_def.h"
#include "pbque.h"

//-- FTL�̃f�[�^�\�����܂Ƃ�- --
//-- ���̒�`
// HLBA: Host Logical Block Number : �z�X�g�A�h���X�Ń��j�[�N
// LPN : Logical Page Number       : �z�X�g�A�h���X��1��1�Ή�
// PPN : Physical Page Number      : �����y�[�W�ԍ��Ńp�b�P�[�W�����j�[�N
// PPO : Physical Page Offset      : �����u���b�N���̕����y�[�W�I�t�Z�b�g
// VPA : Virtual Page Address      : ���z�y�[�W�ԍ�   (32bit: PG#, 24bit: PG Offset Sector, 8bit: Length)


//-- �}�N����`
//#define VP_BITMAP_BYTES (PP_PER_PB / (8*sizeof(uchar)) )  // �����y�[�W�̗L��/��������r�b�g�}�b�v�T�C�Y
#define FTL_LP_NOT_ASSIGNED    0xFFFFFFFF                 // ���\�[�X�����蓖��LP�p��ID
#define FTL_PG_INVALID_ID      0xFFFFFFFF                 // ����PG�ԍ�
#define FTL_REWR_END           0xFFFFFFFF                 // ReWrite�I��

#define FTL_PG_BLOCK_NUM     32                           // ND1P��N+1��
#define FTL_PG_PARITY_NUM     1                           // NDxP��P��
#define FTL_SECTS_PER_LP     16                           // 1�_���y�[�W������̃Z�N�^��
//#define FTL_CW_PER_PG        ((FTL_PG_BLOCK_NUM-FTL_PG_PARITY_NUM) * PP_PER_PB * CW_PER_PP ) // PG�������CW��
//#define FTL_PG_END_CW        FTL_CW_PER_PG                // �ŏICW

#define FTL_OPENPG_NUM           1                           // ����OpenPG��
#define FTL_OPENPG_WL_NUM        7                           // ����OpenPG��,WL����p

#define FTL_MAX_REWR_REQ      16                          // �ő哯���v��ReWrite��
#define FTL_MAX_ERASE_REQ     4                           // �ő哯�������u���b�N
#define FTL_MAX_ERASE_REG     256                         // �ő哯�������\��


//-- ��s�錾
class  FM_INFO;
class  FtlInterface;
class  FtlAsynReqInterface;

//-- �^�錾
//���
enum FTL_PG_STATUS
{
    FTL_PGS_FREE,
    FTL_PGS_OPEN,
    FTL_PGS_CLOSE,
    FTL_PGS_RCM
};
enum FTL_PB_STATUS
{
    FTL_PBS_FREE,   // ��
    FTL_PBS_BIND,   // PG�\����
    FTL_PBS_INVALID // ����
};

//�A�h���X���
typedef uint32_t FTL_LP_GADDR;
typedef uint32_t FTL_PG_GADDR;
typedef uint32_t FTL_PB_GADDR;
typedef uint32_t FTL_PP_GADDR;
typedef uint32_t FTL_PG_OADDR; // pg offset address
typedef uint16_t FTL_PB_OADDR; // pb offset address

//-- IO�v�����
enum FTL_FMIO_TYPE
{
    FMT_HOST_WR, // �z�X�g�v��
    FMT_HOST_RD, // �z�X�g�v��
    FMT_RCM_RWR, // RCM
    FMT_REF_RWR, // RF
    FMT_BLK_ERS, // �������N�G�X�g
    FMT_DEFAULT  // �����l
};

//-- �\���̒�`
// �g���I�v�V����
typedef struct {
    bool      enable_pg_composition;// pg�\���ݒ�̂��߂̃t���O
    uint32_t  pg_pb_num;
    uint32_t  pg_parity_num;

    bool      enable_rcm_th;// rcm臒l�ݒ�̂��߂̃t���O
    uint32_t  rcm_th;
} FTL_EXT_OPT;

// �I�[�v�����
typedef struct {
   FTL_FMIO_TYPE type; // �I�[�v���^�C�v
   uchar           id; // �o�^ID�i�z��ʒu�j
} FTL_OPENPG_INFO;

// �u���b�N�����\��
typedef struct {
    FTL_PB_GADDR pb_id;  // �u���b�N�A�h���X
    uint32_t     reg_id; // �o�^�`�P�b�gID
} FTL_ERASE_REQ;

// VPA�\���̌`��
typedef struct {
    FTL_PG_GADDR pgn;
    FTL_PG_OADDR ofs;
    uchar        len;
    uchar        rcm_count; // �A��RCM���{��
} VPA_INFO;

// �t�������
typedef struct {
    FTL_LP_GADDR lpn;
    uchar        len;
} REVERSE_INFO;

// CW�ʒu���
typedef struct {
    FTL_PB_GADDR pbn;
    FTL_PB_OADDR ppo;
    uchar    cwo;
    uchar    len;
} CW_INFO;

// �����u���b�N���
typedef struct __PB_INFO {
    FTL_PB_GADDR  id;          // ���j�[�N�ȕ����u���b�NID
    FTL_PB_STATUS status;      // �u���b�N���

    FTL_PG_GADDR  pg_no;       // PG�ԍ�

    uint32_t  erase_count;     // �݌v������(�򉻓x)
    uint64_t  last_erase_time; // �ŏI��������

    //-- ����p���
    uint16_t  ce_no;  // ����CE�ԍ� ( 0�`bus������CE�� - 1 )
    uint16_t  bus_no; // ����BUS�ԍ�( �ʔ� )

} PB_INFO;

// �p���e�B�O���[�v���
typedef struct __PG_INFO {
    FTL_PG_GADDR  id;        // ���j�[�NID
    FTL_PG_STATUS status;    // �X�e�[�^�X

    uchar         attr;      // �I�[�v������(hot/cold�I��)�C�b��I��1B�C���f�b�N�X�Ƃ���

    FTL_PG_OADDR  vs_num;    // PG���̗L���Z�N�^��
    FTL_LP_GADDR  lp_num;    // PG���̗L���_���y�[�W�Q�Ɛ�

    FTL_OPENPG_INFO  opg_info;  // �I�[�v��PG���
    FTL_PG_OADDR     next_ofs;  // ��������A�h���X
    PB_INFO**        pb_list; // �\���u���b�N���

    std::deque<REVERSE_INFO> p2l_tbl; // �t����
    FTL_PG_OADDR  next_rewr_index;    // ��ReWrite�C���f�b�N�X
    FTL_PG_OADDR  next_rewr_pgsect;   // ��ReWritePG���I�t�Z�b�g�Z�N�^#

    QUEUE  que; // close que�p

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
