#ifndef SIM_LOGGER__H
#define SIM_LOGGER__H

#include <stdarg.h>
#include <stdio.h>
#include <string>

bool InitLogger( const char* outdir );
void CloseLogger();
bool AddLogType( const char* type, const char* attribute, bool is_out_std = false );
void PrintMessage( const char *type, const char *fmt, ... );

#endif
