#ifndef __FM_DEFINITION_H
#define __FM_DEFINITION_H

// -- FM�`�b�v�̐��\�֌W�p�����[�^
#define FMDIE_READ_BUSY_TIME_US     50 // read:  die op time (page->register)
#define FMDIE_WRITE_BUSY_TIME_US  1400 // write: die op time (page->register)
#define FMDIE_REGREAD_BUSY_TIME_US   1 // reg read: die op time
#define FMDIE_ERASE_BUSY_TIME_US  5000 // erase: die op time
#define FMBUS_TX_BUSY_TIME_US       59 // read/write: register->controller(bus op time)
#define FMBUS_COM_TX_BUSY_TIME_US    1 // read/erase�̍ŏ���1���,write�̍Ō�̃��^�[��

// -- �o�C�g��`
#define BYTES_PER_SECTOR           512                                // 1�Z�N�^������̃o�C�g��
#define BYTE2SECTOR(x) ( (unsigned long long int)(x) / BYTES_PER_SECTOR ) // �o�C�g���Z�N�^
#define GB2SECTOR(x)   ( (unsigned long long int)(x)*(1024*1024*1024/ BYTES_PER_SECTOR) ) // �M�K�o�C�g���Z�N�^
#define SECTOR2BYTE(x) ( (x) * BYTES_PER_SECTOR )                     // �Z�N�^���o�C�g
#define SECTOR2GB(x)   ( (SECTOR_BYTES*( (x) / 1024)/1024) / 1024 )   // �Z�N�^���M�K�o�C�g

// -- �����y�[�W�\��
#define SECTS_PER_PP               16                   // 1�����y�[�W������̃Z�N�^��
#define BYTES_PER_PP     ( BYTES_PER_SECTOR*SECTS_PER_PP )  // 1�����y�[�W������̃o�C�g��
#define PP_PER_PB     256                               // 1�����u���b�N������̃y�[�W��
#define SECTS_PER_PB      ( SECTS_PER_PP*PP_PER_PB )    // 1�����u���b�N������̃Z�N�^��
#define BYTES_PER_PB      ( BYTES_PER_PP*PP_PER_PB )    // 1�����u���b�N������̃o�C�g��

// -- FM�\���Ɋւ���Default�ݒ�
#define DEF_BUS_NUM           8  // �o�X��
#define DEF_CHIP_PER_BUS      2  // �o�X������̃`�b�v��
#define DEF_DIE_PER_CHIP      4  // �`�b�v������̃_�C��
#define DEF_SISE_GB_PER_CHIP  1  // �`�b�v������̃T�C�Y(GByte)

// -- CE�\���Ɋւ���Default�ݒ�
#define DEF_DMA_NUM     4 // CE�̉������i�o�X�����j�̐�
#define DEF_CE_PER_BUS  4 // CE�̏c�����i�`�b�v�����j�̐�

// -- CW�Ɋւ���p�����[�^
#define SECTS_PER_CW    1  // Code Word�T�C�Y = ���k���ŏ��Ǘ��P��
#define CW_PER_PP      (SECTS_PER_PP / SECTS_PER_CW )

// -- ���k�Ɋւ���p�����[�^
#define MAX_COMP_RATIO 16                // �ő刳�k��

// -- 64bit�Ή�
#ifdef x64
#define MAX_FM_GBSIZE 16384 // 64bit�ł̃`�b�v������ő�GB�T�C�Y
#else
#define MAX_FM_GBSIZE 2048  // 32bit�ł̃`�b�v������ő�GB�T�C�Y
#endif

// --- �R���p�C���O�̃`�F�b�N
//- �u���b�N������y�[�W����8�Ŋ���؂�邩
#if (PP_PER_PB % 8 != 0)
#error Error!! PP_PER_PB is not byte multiple ( PP_PER_PB % 8 != 0 )
#endif

#endif
