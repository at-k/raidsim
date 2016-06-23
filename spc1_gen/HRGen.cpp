#include <math.h>
#include "HRGen.h"
#include "Command.h"

HRGen::HRGen(int _asu, double _read_frac, int _transfer_size, unsigned long long int vol_size,
			 double lower_limit, double upper_limit)
			 : asu(_asu), read_frac(_read_frac), transfer_size(_transfer_size)
{
	double hotspot_size = (upper_limit - lower_limit) * vol_size;
	h_max = int(log(hotspot_size / 64) / log(2.0)) - 1;
	double tree_size = 64;
	for (int i = 0; i < h_max + 1; i++) {
		tree_size *= 2;
	}
	offset = (unsigned long long int)(lower_limit * vol_size + get_random() * (hotspot_size - tree_size));

	//  fprintf(stderr, "%d %d %d %ld %ld %ld\n", int(hotspot_size), int(tree_size), h_max, unsigned long long int(hotspot_size - tree_size), offset, unsigned long long int(offset - (lower_limit * vol_size)));
	node_num = 1;
	for (int i = 0; i < h_max + 1; i++) {
		node_num *= 2;
	}
	last_read = new int[node_num];
	for (int i = 0; i < node_num; i++) {
		last_read[i] = leaf_size - 1; // Å‰‚Í‹æ‰æ‚Ìæ“ª‚ª‘I‘ð‚³‚ê‚é‚æ‚¤‚É
	}
	leaf = (int)(get_random() * node_num);
}

HRGen::~HRGen()
{
	delete[] last_read;
}

int HRGen::select_leaf(int last_leaf)
{
	int h;
	int a;

	for (h = h_start; h < h_max; h++) {
		if (get_random() < climb_ratio) {
			break;
		}
	}
	a = 2;
	for (int i = 1; i < h; i++) {
		a *= 2;
	}
	int new_leaf = a * (last_leaf / a) + int(a * get_random());
	return new_leaf;
}

unsigned long long int HRGen::select_addr(Command::IO_OPCODE opcode)
{
	unsigned long long int addr;
	unsigned long long int block;

	leaf = (int)select_leaf(leaf);
	if (opcode == Command::READ) {
		if (last_read[leaf] == leaf_size - 1) {
			block = 0;
		} else {
			block = last_read[leaf] + 1;
		}
		last_read[leaf] = (int)block;
		addr = offset + leaf * leaf_size * block_size + block * block_size;
	} else {
		// WRITE
		if (get_random() < 0.15) {
			// same WR
			addr = last_written_addr;
		} else {
			// final leaf selection
			block = 0;
			if (get_random() < 0.5) {
				if (last_read[leaf] == leaf_size - 1) {
					block = 0;
				} else {
					block = last_read[leaf] + 1;
				}
			} else {
				block = (unsigned long long int)(get_random() * leaf_size);
			}
			addr = offset + leaf * leaf_size * block_size + block * block_size;
		}
		last_written_addr = addr;
	}
	return addr;
}

int HRGen::generate(Command *cmd)
{
	if (get_random() < read_frac) {
		cmd->opcode = Command::READ;
	} else {
		cmd->opcode = Command::WRITE;
	}
	unsigned long long int a = select_addr(cmd->opcode);
	cmd->lba = a;
//	cmd->lba = 0;

	cmd->asu = asu;
	cmd->size = transfer_size;

	return 0;
}
