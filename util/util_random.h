#ifndef RAND_UTIL__H
#define RAND_UTIL__H

#include <stdarg.h>
#include <stdio.h>
#include <string>

void                    sim_srand( unsigned long int seed );
unsigned long long int  sim_rand64( unsigned long long int range );
unsigned int            sim_rand32( unsigned int  range );

#endif
