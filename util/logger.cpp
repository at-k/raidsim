#include "logger.h"
#ifdef WIN32
#pragma warning( disable: 4996 )
#endif

#ifdef WIN32
#include <direct.h>
#endif
#include <sys/stat.h>

#include <stdarg.h>
#include <stdio.h>

#include <map>
#include <string>

#ifdef WIN32
int _mkdir(const char* dir) {
	return mkdir(dir);
}
#else
int _mkdir(const char* dir) {
	return mkdir(dir, ACCESSPERMS);
}
#endif

typedef struct
{
    FILE* file;
    bool  is_std_out;
} LOG_FILE_INFO;

class Logger
{
public:
    Logger(){}
    ~Logger();

    std::map<std::string, LOG_FILE_INFO> flist;
    std::string outdir;
};

Logger::~Logger()
{
    std::map<std::string, LOG_FILE_INFO>::iterator itr;

    for( itr = flist.begin(); itr != flist.end(); itr++ )
    {
        if( (*itr).second.file != NULL )
        {
            fclose( (*itr).second.file );
            (*itr).second.file = NULL;
        }
    }

    flist.clear();
}


Logger* logger       = NULL;

bool InitLogger(const char* outdir)
{
    if( logger != NULL )
    {// 既に初期化済み
        return false;
    }

    // 出力ディレクトリ作成
    int error_no = _mkdir(outdir);

    if( error_no == -1 )
    {// 作成失敗
        struct stat   stat_buf;
        stat(outdir, &stat_buf);
        if( (stat_buf.st_mode & 0170000) != 0040000 )
        {// 失敗要因が，「既に該当が存在している」以外の場合
            return false;
        }
    }

    // Loggerの作成
    logger = new Logger;
    logger->outdir = outdir;

    return true;
}

void CloseLogger()
{
    if ( logger != NULL )
    {
        delete logger;
        logger = NULL;
    }
}

bool AddLogType( const char* type ,const char* attri, bool is_std_out )
{
    if( logger == NULL )
        return false;

    char          file_name[256];
    FILE*         fp;
    LOG_FILE_INFO log_info;

    fp = NULL;

#ifdef WIN32
    sprintf( file_name, "%s\\%s.txt", logger->outdir.c_str(), type );
#else
    sprintf( file_name, "%s/%s.txt", logger->outdir.c_str(), type );
#endif

    fp = fopen( file_name, attri );

	if( fp == NULL )
        return false;

    // 追加
    log_info.file = fp;
    log_info.is_std_out = is_std_out;
    logger->flist.insert( std::map<std::string, LOG_FILE_INFO>::value_type( type, log_info ) );

    return true;
}

void PrintMessage( const char *type, const char *fmt, ... )
{
    std::map<std::string, LOG_FILE_INFO>::iterator itr;

	if(logger == NULL || logger->flist.empty() ) return;

    // key のファイルを探す
    itr = logger->flist.find( type );
    if( itr != logger->flist.end() )
    {// 見つかった
        if( itr->second.file != NULL )
        {// ちゃんとオープンされている

            // 書き込み開始
            va_list argp;
            va_start( argp, fmt );
            vfprintf( itr->second.file, fmt, argp );

            if( itr->second.is_std_out == true )
            {
                vprintf( fmt, argp );
            }

            // 即時書き込み
            fflush( itr->second.file );
        }
    }
}
