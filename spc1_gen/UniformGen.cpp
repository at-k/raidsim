#include "UniformGen.h"
#include "Command.h"

UniformGen::UniformGen(int _asu, double _read_frac, int _transfer_size,
					   unsigned long long int _vol_size, double _lower_limit, double _upper_limit)
					   : asu(_asu), read_frac(_read_frac), transfer_size(_transfer_size),
					   vol_size(_vol_size), lower_limit(_lower_limit), upper_limit(_upper_limit)
{
	max_addr = (unsigned long long int)(upper_limit * vol_size) - transfer_size;
}

UniformGen::~UniformGen()
{
}

int UniformGen::generate(Command *cmd)
{
	if (get_random() < read_frac) {
		cmd->opcode = Command::READ;
	} else {
		cmd->opcode = Command::WRITE;
	}

    unsigned long long int a = (unsigned long long int) ((get_random() * (upper_limit - lower_limit) + lower_limit) * vol_size);
	if (a > max_addr) {
		a = max_addr;
	}
	cmd->lba = a;

	cmd->asu = asu;
	cmd->size = transfer_size;

	return 0;
}
