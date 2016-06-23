#include "util_random.h"

#include <sys/stat.h>
#include <sys/timeb.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

//#define GSL_ENABLE

// use gsl library
#ifdef GSL_ENABLE

#include <gsl/gsl_rng.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_statistics.h>

//#pragma comment(lib, "gsl.lib")
//#pragma comment(lib, "cblas.lib")

#include "mt/mt64.h"

class GSL
{

public:
    gsl_rng *gr;

    GSL()
    {
        gsl_rng_env_setup();
        gsl_rng_type *T = (gsl_rng_type*)gsl_rng_default;
        gr = gsl_rng_alloc( T );
    }
    ~GSL()
    {
        gsl_rng_free( gr );
    }
};

GSL  gsl_rand;


void     sim_srand ( unsigned long int seed )
{
    // GSL‚Ì—”‰Šú‰»
    gsl_rng_set ( gsl_rand.gr, seed );

    // MT‚Ì—”‰Šú‰»
    unsigned long long init[4]={0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL}, length=4;
    init[0] = seed;
    init_by_array64( init, length );
}

unsigned long long int sim_rand64 ( unsigned long long int range )
{

    unsigned long long int ret = genrand64_int64();

    return ret % range;
}

unsigned int sim_rand32 ( unsigned int range )
{
    return gsl_rng_uniform_int( gsl_rand.gr, range );
}

#else
// use c++11 random library

#include <random>

class C11_Random {
	public:
		std::mt19937*		mt;
		std::mt19937_64*	mt_64;

		C11_Random() {
			mt = NULL; mt_64 = NULL;
		}
		~C11_Random() {
			if(mt != NULL) delete mt;
			if(mt_64 != NULL) delete mt_64;
		}
};

C11_Random c11_rand;

void     sim_srand ( unsigned long int seed )
{
	c11_rand.mt	   = new std::mt19937(seed);
	c11_rand.mt_64 = new std::mt19937_64(seed);
}

unsigned long long int sim_rand64 ( unsigned long long int range )
{
	if( c11_rand.mt_64 == NULL )
		sim_srand(0);
	std::uniform_int_distribution<unsigned long long int> dist(0, range);
    return dist(*c11_rand.mt_64);
}

unsigned int sim_rand32 ( unsigned int range )
{
	if( c11_rand.mt == NULL )
		sim_srand(0);
	std::uniform_int_distribution<unsigned int> dist(0, range);
	return dist(*c11_rand.mt);
}

#endif

