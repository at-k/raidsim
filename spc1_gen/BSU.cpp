#include <stdlib.h>
#include "BSU.h"
#include "IOGen.h"
#include "HRGen.h"
#include "SeqGen.h"
#include "UniformGen.h"
#include "Command.h"

BSU::BSU(unsigned long long int asu1_size, unsigned long long int asu2_size, unsigned long long int asu3_size)
{
  iogen[0] = new UniformGen(0, 0.5, 8, asu1_size, 0, 1);
  iogen[1] = new HRGen(0, 0.5, 8, asu1_size, 0.15, 0.2);
  iogen[2] = new SeqGen(0, 1, asu1_size, 0.4, 0.4, 0, 0.1);
  iogen[3] = new HRGen(0, 0.5, 8, asu1_size, 0.7, 0.75);
  iogen[4] = new UniformGen(1, 0.3, 8, asu2_size, 0, 1);
  iogen[5] = new HRGen(1, 0.3, 8, asu2_size, 0.47, 0.52);
  iogen[6] = new SeqGen(1, 1, asu2_size, 0.4, 0.4, 0, 0.1);
  iogen[7] = new SeqGen(2, 0, asu3_size, 0.35, 0.7, 0, 0.3);

  intensity[0] = 0.035;
  intensity[1] = 0.281;
  intensity[2] = 0.070;
  intensity[3] = 0.210;
  intensity[4] = 0.018;
  intensity[5] = 0.070;
  intensity[6] = 0.035;
  intensity[7] = 0.281;
}

BSU::~BSU()
{
  for (int i = 0; i < IOGEN_NUM; i++) {
    delete iogen[i];
  }
}

int BSU::generate(Command *cmd)
{
  double r = double(sim_rand32(RAND_MAX)) / (RAND_MAX + 1.0);
  int i;

  for (i = 0; i < IOGEN_NUM - 1; i++) {
    r -= intensity[i];
    if (r < 0) {
      break;
    }
  }
  int ret = iogen[i]->generate(cmd);

  return ret;
}


