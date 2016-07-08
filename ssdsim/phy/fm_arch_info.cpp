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
    // 0�l�`�F�b�N
    if( bus_num_i == 0 || chip_per_bus_i == 0 || die_per_chip_i ==0 || byte_per_chip_i == 0 || dma_num_i == 0 || ce_per_bus_i == 0 ) {
		printf("%d, %d,%d,%ld,%d,%d,\n", bus_num_i, chip_per_bus_i, die_per_chip_i, byte_per_chip_i, dma_num_i, ce_per_bus_i );

        ERR_AND_RTN;
	}
    // ���ɏ���������Ă��邩�`�F�b�N
    if( bus_info != NULL )
        ERR_AND_RTN;

    bus_num = bus_num_i;
    dma_num = dma_num_i;

    chip_per_bus = chip_per_bus_i;
    die_per_chip = die_per_chip_i;

    // �g�[�^���e��(GB)�v�Z
    fm_byte = byte_per_chip_i * chip_per_bus * bus_num;
    if( sizeof(size_t) == 4 && (fm_byte / 1024 / 1024 / 1024 ) > MAX_FM_GBSIZE )
    {
        PrintMessage( LOG_TYPE_ERROR, "Erorr at FM_INFO::InitFlashModule -- FM Size is too large. Maximum support size is 2048GB -- input size is %d Byte\n", fm_byte);
        ERR_AND_RTN;
    }

    // �g�[�^���e�ʂ��K�����m�F
    uint64_t tmp = fm_byte;
    while( tmp != 0 && tmp != 2)
    {// fm_byte��2^x byte�łȂ�������false
        if( (tmp & 0x1) != 0 )
        {
            PrintMessage( LOG_TYPE_ERROR, "Erorr at FM_INFO::InitFlashModule -- �s���ȗe�ʂł� 2�̗ݏ�ɂȂ�p�ɐݒ肵�Ă�������-- input size is %d Byte\n", fm_byte);
            ERR_AND_RTN;
        }
        tmp = tmp >> 1;
    }

    // �o�X�쐬
    bus_info = new BUS_INFO[bus_num];

    // �S�Z�N�^�����v�Z
    fm_sects = BYTE2SECTOR( fm_byte );
    // �y�[�W���Ŋ����Ē[�����o��Ί��肫���悤�ɒ���
    fm_sects += (( fm_sects % SECTS_PER_PP == 0) ? 0:(SECTS_PER_PP - fm_sects % SECTS_PER_PP));

    // �����u���b�N�E�y�[�W���v�Z
    pb_num = (uint32_t)( fm_sects / SECTS_PER_PB );
    pp_num = pb_num * PP_PER_PB;

    // �`������u���b�N���v�Z
    pb_per_bus  = pb_num / bus_num;
    pb_per_chip = pb_num / (bus_num * chip_per_bus);
    pb_per_die  = pb_num / (bus_num * chip_per_bus * die_per_chip);

    //-- chip enable �\���L�q
    if( bus_num < dma_num )
    {
        PrintMessage( LOG_TYPE_ERROR, "Erorr at FM_INFO::InitFlashModule -- �o�X����DMA����菭�Ȃ��ݒ肳��Ă��܂�\n" );
        ERR_AND_RTN;
    }

    if( bus_num % dma_num != 0 )
    {
        PrintMessage( LOG_TYPE_ERROR, "Erorr at FM_INFO::InitFlashModule -- �o�X����DMA���Ŋ���؂��K�v������܂�\n" );
        ERR_AND_RTN;
    }

    ce_per_bus = ce_per_bus_i;  // �o�X�������CE�����ݒ�iCE�̏c�����̐��j

    if( ce_per_bus > chip_per_bus * die_per_chip )
    {
        PrintMessage( LOG_TYPE_ERROR, "Erorr at FM_INFO::InitFlashModule -- �o�X�������CE�����o�X������̃_�C�����z���Ă��܂�\n" );
        ERR_AND_RTN;
    }

    if( ( die_per_chip * chip_per_bus ) % ce_per_bus )
    {
        PrintMessage( LOG_TYPE_ERROR, "Erorr at FM_INFO::InitFlashModule -- �o�X������̃_�C����CE���Ŋ���؂�܂���\n");
        ERR_AND_RTN;
    }

    // �o�X������̃_�C�����o�X������CE���Ŋ���
    die_per_ce_per_bus = (die_per_chip * chip_per_bus) / ce_per_bus;

    // CE�쐬
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

    //-- �o�X�C�`�b�v�C�_�C�̎��̍쐬 & CE���̍쐬
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

    //-- ID�̐ݒ� ex..(BusID, ChipID, DieID, PBID)������̓��j�[�N�BFMADDR����ppo(�y�[�W�ʒu�j���������l

    // �ݒ�J�n
    for( uint32_t i = 0; i < bus_num ; i++ )
    {
        bus_info[i].id = i;    // ID�ݒ�
        bus_info[i].status = BUS_IDLE; // �o�X�̏�����Ԃ�IDLE

        for( uint32_t j = 0; j < chip_per_bus ; j++ )
        {
            bus_info[i].clist[j].id = j; // ID�ݒ�

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

