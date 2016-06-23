#ifndef SIM_LOGGER__H
#define SIM_LOGGER__H

#include <stdarg.h>
#include <stdio.h>
#include <string>


bool InitLogger( const char* outdir );
void CloseLogger();
bool AddLogType( const char* type, const char* attribute, bool is_out_std = false );
void PrintMessage( const char *type, const char *fmt, ... );

extern bool g_is_trace_on;

extern char trace_buffer_tmp[256];
extern char trace_buffer[256];
extern char trace_header[128];

// global変数trace_headerに文字列をセット
inline void SetTraceHeader( char* fmt, ... )
{
    if( g_is_trace_on )
    {
        va_list argp;
        va_start( argp, fmt );
        vsprintf( trace_header, fmt, argp );
    }
}

// trace出力，header + level数分の","の後に指定文字列
inline void PrintTrace( const char *type, unsigned char level,  char *fmt, ...)
{
    if( g_is_trace_on )
    {
        std::string buffer;
        va_list argp;
        va_start( argp, fmt );
        buffer = "";

        for( unsigned char i = 0; i < level+1; i++ )
            buffer.append(",");

        sprintf( trace_buffer_tmp,
                 "%s%s%s", trace_header, buffer.c_str(), fmt );

        vsprintf( trace_buffer, trace_buffer_tmp, argp );

        PrintMessage( type, trace_buffer );

    }
}

#endif
