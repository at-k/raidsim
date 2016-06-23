#include "SeqGen.h"
#include "Command.h"

// スタートアドレスを初回I/O時に計算させるため、last_addr > max_addrにしておく
SeqGen::SeqGen(int _asu, double _read_frac, unsigned long long int _vol_size, double _start,
	       double _startvar, int _stride, double _length)
  : asu(_asu), read_frac(_read_frac), vol_size(_vol_size), start(_start),
    startvar(_startvar), stride(_stride), length(_length),
    max_addr(0), last_addr(1), last_transfer_size(0)
{
}

SeqGen::~SeqGen()
{
}

unsigned long long int SeqGen::get_start_addr(int transfer_size)
{
  unsigned long long int a;
  a = (unsigned long long int)((start + (get_random() - 0.5) * startvar) * vol_size);
  if (a + transfer_size > vol_size) {
    a = vol_size - transfer_size;
  }
  return a;
}

int SeqGen::generate(Command *cmd)
{
  unsigned long long int addr = last_addr + last_transfer_size + stride;
  int transfer_size = get_smix();

  if (addr + transfer_size > max_addr + 1) {
    addr = get_start_addr(transfer_size);
    max_addr = addr + (unsigned long long int)(length * vol_size);
    if (max_addr > vol_size - 1) {
      max_addr = vol_size - 1;
    }
  }
  cmd->lba = addr;

  if (get_random() < read_frac) {
    cmd->opcode = Command::READ;
  } else {
    cmd->opcode = Command::WRITE;
  }
  cmd->asu = asu;
  cmd->size = transfer_size;

  last_addr = addr;
  last_transfer_size = transfer_size;

  return 0;
}

// "SMIX"に指定された Transfer size を乱数で決める
int SeqGen::get_smix(void)
{
  double r = get_random();

  int size;
  if (r < 0.40) {
    size = 8;
  } else if (r < 0.64) {
    size = 16;
  } else if (r < 0.84) {
    size = 32;
  } else if (r < 0.92) {
    size = 64;
  } else {
    size = 128;
  }

  return size;
}
