
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <windows.h>
#include <shlwapi.h>

typedef char                  char_t;
typedef char const           cchar_t;
typedef unsigned char        uchar_t;
typedef unsigned char const ucchar_t;

typedef char*                  cstr_t;
typedef char const*           ccstr_t;
typedef unsigned char*        ucstr_t;
typedef unsigned char const* uccstr_t;

typedef unsigned int     uint_t;
typedef unsigned short  ushrt_t;
typedef ushrt_t*        ushrt_p;
typedef ushrt_t const* ucshrt_p;

static ccstr_t const newline = "\n";

// just wrapping win api because i'v never used it

typedef SYSTEMTIME      stime_t;
typedef stime_t*        stime_p;
typedef stime_t const* cstime_p;

static inline
stime_t nilstime(void) {
  return (stime_t)
    { 0 /* wYear */,      0 /* wMonth */,
      0 /* wDayOfWeek */, 0 /* wDay (of month) */,
      0 /* wHour */,      0 /* wMinute */,
      0 /* wSecond */,    0 /* wMilliseconds */ };
}

static inline
void stimeprn(cstime_p const t) {
  if(t) {
    fprintf(stderr, "year: %u, month: %u, dow: %u, dom: %u\n",
            t->wYear, t->wMonth, t->wDayOfWeek, t->wDay );
    fprintf(stderr, "hr: %u, mn: %u, sc: %u, ms: %u\n",
            t->wHour, t->wMinute, t->wSecond, t->wMilliseconds );
  } else fprintf(stderr, "nil stime_p\n");
}

typedef FILETIME        ftime_t;
typedef ftime_t*        ftime_p;
typedef ftime_t const* cftime_p;

static inline
ftime_t nilftime(void) {
  return (ftime_t) { 0 /* dwLowDateTime */, 0 /* dwHighDateTime */ };
}

static inline
void ftimeprn(cftime_p const t) {
  if(t) fprintf(stderr, "high: %u, low: %u\n",
                t->dwHighDateTime, t->dwHighDateTime);
  else fprintf(stderr, "nil ftime_p\n");
}

typedef uint_t        err_t;
typedef err_t*        err_p;
typedef err_t const* cerr_p;

err_t errprn(err_t const err, bool verbose) {
  if(err && verbose) { // retrieve sys errmsg
    void* buf;
    FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM     |
      FORMAT_MESSAGE_IGNORE_INSERTS  ,
      NULL, err,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR) &buf, 0, NULL );
    fprintf(stderr, "%s", (cstr_t) buf);
    LocalFree(buf);
  }
  return err;
}

// seems it's easier/shorter to get attributes before a file handle
// to manage times of ro files because SetFileTime require a writable
// file handle not available from a read only file

typedef WIN32_FILE_ATTRIBUTE_DATA attr_t;
typedef attr_t*                   attr_p;

attr_t nilattr(void) {
  return (attr_t)
    { 0,        // dwFileAttributes
      { 0, 0 }, // ftCreationTime   { dwLowDateTime, dwHighDateTime }
      { 0, 0 }, // ftLastAccessTime { dwLowDateTime, dwHighDateTime }
      { 0, 0 }, // ftLastWriteTime  { dwLowDateTime, dwHighDateTime }
      0,        // nFileSizeHigh
      0 };      // nFileSizeLow
}

typedef HANDLE     fhandle_t;
typedef fhandle_t* fhandle_p;

// wrap with error report

static inline
size_t widen(ccstr_t const s, void* out, size_t osz) {
  // todo win32 unicode/ucs2 support for filehandle
  // does'nt apply for the rest of the code assuming utf8
  if(!s) return 0;
  size_t len, slen = strlen(s);
  len = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, s, slen+1, 0, 0);
  if(len >= osz) return 0;
  return 0;
}

static inline
err_t filehandle(void* fpathname, fhandle_p fhdl, bool write, bool wide) {
  if(!fhdl || !fpathname) return ERROR_INVALID_PARAMETER;
  *fhdl = wide ?
            CreateFileW( (LPCWSTR) fpathname,
                         GENERIC_READ | (write ? GENERIC_WRITE : 0), 0, NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL |
                           FILE_FLAG_BACKUP_SEMANTICS, NULL ) :
            CreateFile( (LPCSTR) fpathname,
                        GENERIC_READ | (write ? GENERIC_WRITE : 0), 0, NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL |
                          FILE_FLAG_BACKUP_SEMANTICS, NULL );
  return (*fhdl) == INVALID_HANDLE_VALUE ? GetLastError() : 0;
}

static inline
err_t closehandle(fhandle_t h) {
  return CloseHandle(h) ? 0 : GetLastError();
}

static inline
err_t time_s2f(stime_p s, ftime_p f) {
  return SystemTimeToFileTime(s, f) ? 0 : GetLastError();
}

static inline
err_t time_f2s(ftime_p f, stime_p s) {
  return FileTimeToSystemTime(f, s) ? 0 : GetLastError();
}

static inline
err_t localtime_s2f(stime_p s, ftime_p f) {
  ftime_t tmp;
  if(!time_s2f(s, f) && LocalFileTimeToFileTime(f, &tmp))
    return (*f = tmp), 0;
  return GetLastError();
}

static inline
err_t setftime(fhandle_t fhdl, cftime_p crt, cftime_p acs, cftime_p wrt) {
  return SetFileTime(fhdl, crt, acs, wrt) ? 0 : GetLastError();
}

static inline
err_t fileattr(ccstr_t fpathname, attr_p fattr) {
  return fattr ?
    ( GetFileAttributesEx(fpathname, GetFileExInfoStandard, fattr) ?
        0 : GetLastError() )
    : ERROR_INVALID_PARAMETER;
}

static inline
err_t setfattr(cstr_t fpathname, uint_t attrs) {
  return SetFileAttributes(fpathname, attrs) ? 0 : GetLastError();
}

static inline
bool is_ro(attr_p a) {
  return a ? a->dwFileAttributes & FILE_ATTRIBUTE_READONLY : 0;
}

static inline
err_t set_ro(cstr_t f, attr_p a) {
  return f && a ?
    setfattr(f, a->dwFileAttributes | FILE_ATTRIBUTE_READONLY) :
    ERROR_INVALID_PARAMETER;
}

static inline
err_t unset_ro(cstr_t f, attr_p a) {
  return f && a ?
    setfattr(f, a->dwFileAttributes ^ FILE_ATTRIBUTE_READONLY) :
    ERROR_INVALID_PARAMETER;
}

static inline
bool exist(ccstr_t f) {
  uint32_t a = GetFileAttributes(f);
  return (a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY));
}

static inline
void* badpp(void) { SetLastError(ERROR_INVALID_PARAMETER); return 0; }

static inline
err_t badpv(void) { return ERROR_INVALID_PARAMETER; }

/*****************************************************************************/

typedef enum {
  MDF_IDX = 0,
  MDF = 0x01,
  CRT_IDX = 1,
  CRT = 0x02,
  ACS_IDX = 2,
  ACS = 0x04,
  ALL_IDX = 3,
  ALL = 0x08,
  ERR = -1
} opidx_e;

typedef struct {
  opidx_e reqops; // requested time(s) mask.
  opidx_e settms; // provided time(s) mask.
  ftime_t tms[4]; // provided time(s) value(s).
  cstr_t   input; // file(s) list input (for stdin ('-') input value is -1).
  ushrt_t   year; // default year for incomplete(s) time(s).
  bool     ignro; // ignore read only files.
  bool      vrbs; // verbose (output errors).
} ctx_t;

typedef ctx_t* ctx_p;

opidx_e op2idx(opidx_e op) {
  return (op & MDF ? MDF_IDX :
         (op & CRT ? CRT_IDX :
         (op & ACS ? ACS_IDX :
         (op & ALL ? ALL_IDX :
          ERR))));
}

static inline
ctx_t nilctx(void) {
  return (ctx_t)
    { 0, 0, { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } }, 0, 0, 0, 1 };
}

static inline
bool ftcmp(ftime_t const t1, ftime_t const t2) {
  return (t1.dwLowDateTime == t2.dwLowDateTime) &&
         (t1.dwHighDateTime == t2.dwHighDateTime);
}

static inline
uint64_t ft2val(ftime_t const t) {
  uint64_t out = t.dwHighDateTime;
  return (out << 32) + t.dwLowDateTime;
}

err_t touch(cstr_t f, ctx_p ctx) {
  uint_t err;
  attr_t att;
  
  if(!f) return badpv();
  if((err = fileattr(f, &att)))
    return errprn(err, ctx->vrbs);
  
  opidx_e todo = 0;
  ftime_p p[3] = { 0, 0, 0 };
  
  if( (ctx->reqops & MDF) &&
      !ftcmp(ctx->tms[op2idx(MDF)], att.ftLastWriteTime) )
    todo |= ((p[op2idx(MDF)] = &(ctx->tms[op2idx(MDF)])), MDF);
  
  if( (ctx->reqops & CRT) &&
      !ftcmp(ctx->tms[op2idx(CRT)], att.ftLastWriteTime) )
    todo |= ((p[op2idx(CRT)] = &(ctx->tms[op2idx(CRT)])), CRT);
  
  if( (ctx->reqops & ACS) &&
      !ftcmp(ctx->tms[op2idx(ACS)], att.ftLastWriteTime) )
    todo |= ((p[op2idx(ACS)] = &(ctx->tms[op2idx(ACS)])), ACS);
  
  if(!todo) return 0;
  
  bool rstro;
  if((rstro = is_ro(&att))) {
    if(ctx->ignro) return 0;
    if((err = unset_ro(f, &att)))
      return errprn(err, ctx->vrbs);
  }
  
  fhandle_t hfile;
  static ftime_t const access_protect = { 0xFFFFFFFF, 0xFFFFFFFF };
  
  if(!(err = filehandle(f, &hfile, 1, 0))) {
    if(!(err = setftime(hfile, p[op2idx(CRT)],
                               p[op2idx(ACS)],
                               p[op2idx(MDF)])))
      err = setftime(hfile, 0, &access_protect, 0);
    if(!err) err = closehandle(hfile);
    else closehandle(hfile);
  }
  
  err_t rstro_err = rstro ? set_ro(f, &att) : 0;
  return err ?
    errprn(err, ctx->vrbs) : (rstro_err ? errprn(rstro_err, ctx->vrbs) : 0);
}

// read time format: MMDDhhmm[[CC]YY][.ss]
// note: date/time validation is made by win api

static inline
uchar_t ctodgt(uchar_t const c) {
  return ((c > 47) && (c < 58)) ? (c - 48) : -1;
}

static inline
uchar_t get2dgt(uchar_t c0, uchar_t c1) {
  return (((c0 = ctodgt(c0)) > 9) || ((c1 = ctodgt(c1)) > 9)) ?
    -1 : (c0 * 10 + c1);
}

static inline
cstr_t timefield(cstr_t v, ushrt_p out) {
  return (*out = get2dgt(v[0],v[1])) == (uchar_t) -1 ? 0 : v+2;
}

ftime_p readtime(cstr_t v, ushrt_t dflt_year, ftime_p out) {
  size_t len = strlen(v);
  ushrt_t seclen = ((len & 0x1) << 1) | (len & 0x1);
  
  if(!out || (len < 8) || (len > 15) || (seclen && (v[len - seclen] != '.')))
    return badpp();
  
  stime_t tmp = nilstime(); // read opts seconds
  if(seclen && !timefield(v + len - 2, &tmp.wSecond)) return badpp();
  
  if(len > (seclen + 8)) { // read opts century & year
    
    ushrt_t century = 0;
    if(len > (seclen + 10)) {
      if(!timefield(v + len - (seclen + 4), &century)) return badpp();
    }
    else century = dflt_year / 100;
    
    if(!timefield(v + len - (seclen + 2), &tmp.wYear)) return badpp();
    tmp.wYear += 100 * century;
  } else tmp.wYear = dflt_year;
  
  // read rest
  if(!(v = timefield(v, &tmp.wMonth)) || !(v = timefield(v, &tmp.wDay)) ||
     !(v = timefield(v, &tmp.wHour)) || !(v = timefield(v, &tmp.wMinute)) )
    return badpp();
  
  // validate
  return localtime_s2f(&tmp, out) ? 0 : out;
}

// popts return :
// -1 : no more options/parameter(s), no error.
// -2 : missing option parameter error.
// -3 : repeat option parameter error.
// -4 : invalid time option parameter.
// >0 : index of first non option parameter.
//
// note: ctx year must be set before reading times with popts/gettme.

static int const popt_maxerr = -4;

static inline
int ovrf(int idx, int end) { return (idx >= end) ? -1 : idx; }

static inline
int opt_mss(ccstr_t type, ccstr_t prm) {
  static ccstr_t const fmt = "%s : missing/empty %s option parameter\n";
  fprintf(stderr, fmt, prm, type);
  return -2;
}

static inline
int opt_rpt(int idx, cstr_t str[], bool withprm) {
  #define REPEATFMT " : option repeat/conflict\n"
  static ccstr_t const fmt1 = "%s" REPEATFMT;
  static ccstr_t const fmt2 = "%s %s" REPEATFMT;
  if(withprm) fprintf(stderr, fmt2, str[idx], str[idx+1]);
  else fprintf(stderr, fmt1, str[idx]);
  return -3;
}

static inline
int opt_btm(int idx, cstr_t str[]) {
  static ccstr_t const fmt = "%s %s : bad time parameter\n";
  fprintf(stderr, fmt, str[idx], str[idx+1]);
  return -4;
}

static inline
int gettme(ctx_p ctx, opidx_e op, int idx, int end, cstr_t str[]) {
  if(ovrf(idx+1, end) < 0) return opt_mss("time", str[idx]);
  if(!readtime(str[idx+1], ctx->year, &(ctx->tms[op2idx(op)])))
    return opt_btm(idx, str);
  if(ctx->settms & op) return opt_rpt(idx, str, true);
  
  ctx->settms |= op;
  return ++idx;
}

static inline
int getinp(ctx_p ctx, int idx, int end, cstr_t str[]) {
  if(ovrf(idx+1, end) < 0) return opt_mss("input file", str[idx]);
  if(ctx->input) return opt_rpt(idx, str, true);
  
  size_t len;
  if((len = strlen(str[idx+1])))
    ctx->input = ((len == 1) && (str[idx+1][0] == '-')) ?
      (cstr_t) -1 : str[idx+1];
  else return opt_mss("input file", str[idx]);

  return ++idx;
}

int popts(int c, cstr_t v[], ctx_p ctx) {
  int idx = 1;
  while(idx < c) {
    size_t len = strlen(v[idx]);
    if(!len || (len > 3) || (len == 1) || !(v[idx][0] == '-'))
      return idx;
    if(len == 2)
      switch(v[idx][1]) {
        case 's':
          if(!ctx->vrbs) opt_rpt(idx, v, false);
          ctx->vrbs = false;
          break;
        case 't':
          if((idx = gettme(ctx, ALL, idx, c, v)) < 0) return idx;
          break;
        case 'f':
          if(((idx = getinp(ctx, idx, c, v)) < 0)) return idx;
          break;
        case 'r':
          if(ctx->ignro) return opt_rpt(idx, v, false);
          ctx->ignro = true;
          break;
        case 'a':
          if(ctx->reqops & ACS) return opt_rpt(idx, v, false);
          ctx->reqops |= ACS;
          break;
        case 'c':
          if(ctx->reqops & CRT) return opt_rpt(idx, v, false);
          ctx->reqops |= CRT;
          break;
        case 'm':
          if(ctx->reqops & MDF) return opt_rpt(idx, v, false);
          ctx->reqops |= MDF;
          break;
        case '-': return ovrf(++idx, c); break;
        default : return idx; break;
      }
    if((len == 3) && (v[idx][1] == 't')) {
      opidx_e op = 0;
      switch(v[idx][2]) {
        case 'a': op = ACS; break;
        case 'c': op = CRT; break;
        case 'm': op = MDF; break;
        default : return idx; break;
      }
      if((idx = gettme(ctx, op, idx, c, v)) < 0) return idx;
    }
    ++idx;
  }
  return ovrf(idx, c);
}

typedef enum {
  UNKNOW_USAGE = 0,
  TIMEOP_ERROR = 1,
  FILEOP_ERROR = 2,
  USAGE_ERROR = 3,
} main_error;

ccstr_t const error[] = {
  "unknown usage error : %u\n",
  "error %u on time\n",
  "error %u on file : \"%s\"\n",
  "usage:\n"
  "%s [-(s|r|a|c|m)] [-f (<file>|-)] [-t[a|c|m] <time>] [--] [<file> ...]\n\n"
  "change modification and access time of file(s) to current system time.\n\n"
  "-s\t\t silent mode.\n"
  "-r\t\t ignore files with readonly attribute\n"
  "  \t\t (default is to remove then reset it if needed).\n"
  "-a\t\t change access time (and others requested times only).\n"
  "-c\t\t change creation time (and others requested times only).\n"
  "-m\t\t change modification time (and others requested times only).\n"
  "-t <time>\t where <time> is of the form : MMDDhhmm[[CC]YY][.ss]\n"
  "  \t\t set <time> as file(s) times instead of system time.\n"
  "-ta <time>\t where <time> is of the form : MMDDhhmm[[CC]YY][.ss]\n"
  "  \t\t use <time> as access time (override system and -t <time>).\n"
  "-tc <time>\t where <time> is of the form : MMDDhhmm[[CC]YY][.ss]\n"
  "  \t\t use <time> as creation time (override system and -t <time>).\n"
  "-tm <time>\t where <time> is of the form : MMDDhhmm[[CC]YY][.ss]\n"
  "  \t\t use <time> as modification time (override system and -t <time>).\n"
  "-f <file>\t read file(s) arguments from <file> or stdin when '-',\n"
  "  \t\t one file per line, file(s) argument(s) on command line\n"
  "  \t\t are still treated before <file>/stdin input.\n"
  "--\t\t the first non option argument always stop option(s) reading,\n"
  "  \t\t this option force this stop, allowing file(s) named like\n"
  "  \t\t options.\n"
};

void usage(int exit_code, bool verbose, cstr_t name) {
  cstr_t cmdname = "cmd";
  char_t tmp[MAX_PATH];
  
  if(verbose) {
    if(name) {
      strcpy(tmp, name);
      PathStripPathA(tmp);
      cmdname = tmp;
    }
    
    if(exit_code > 0) // not popt error
      fprintf(stderr, error[UNKNOW_USAGE], exit_code);
    printf(error[USAGE_ERROR], cmdname);
  }
  
  if(exit_code < 0) // popt error
    exit_code = -exit_code;
  
  fflush(stdout);
  fflush(stderr);
  exit(exit_code);
}

int main(int argc, char* argv[]) {
  ctx_t ctx = nilctx();
  
  // get system/utc time.
  stime_t systm_stime;
  GetSystemTime(&systm_stime);
  
  // set default year for incomplete times opts.
  ctx.year = systm_stime.wYear;
  
  // getopts & abort on usage error.
  int idx = popts(argc, argv, &ctx);
  if(idx < -1) usage(idx, ctx.vrbs, argv[0]);
  
  // following errors are'nt opts errors.
  int outerr = - popt_maxerr;
  
  err_t tmperr;
  
  // set default time to provided default or system time.
  ftime_t dflt_ftime = { 0, 0 };
  if(ctx.settms & ALL)
    dflt_ftime = ctx.tms[ALL_IDX];
  else {
    tmperr = errprn(time_s2f(&systm_stime, &dflt_ftime), ctx.vrbs);
    if(tmperr && ctx.vrbs) {
      fprintf(stderr, error[TIMEOP_ERROR], tmperr);
      fflush(stderr);
      exit(outerr + TIMEOP_ERROR);
    }
  }
  
  // use default time for times not provided.
  if(!(ctx.settms & ACS)) ctx.tms[ACS_IDX] = dflt_ftime;
  if(!(ctx.settms & CRT)) ctx.tms[CRT_IDX] = dflt_ftime;
  if(!(ctx.settms & MDF)) ctx.tms[MDF_IDX] = dflt_ftime;
  
  // default is to update modification and access time
  if(!(ctx.reqops & (ACS | CRT | MDF)))
    ctx.reqops = (ACS | MDF);
  
  FILE* input = 0;
  // select optionnal input file list
  switch((uintptr_t) ctx.input) {
    case 0:
      if(idx == -1) usage(EXIT_SUCCESS, ctx.vrbs, argv[0]);
      input = 0;
      break;
    case -1:
      input = stdin;
      break;
    default:
      errno = 0;
      if(!(input = fopen(ctx.input, "r"))) {
        if(errno && ctx.vrbs) {
          fprintf(stderr, error[FILEOP_ERROR], errno, ctx.input);
          cstr_t errstr = strerror(errno);
          if(errstr) fprintf(stderr, errstr), fprintf(stderr, newline);
          fflush(stderr);
        }
        exit(outerr + FILEOP_ERROR);
      }
      break;
  }
  
  // do the job
  
  if(idx > 0) // cmdline
    for(int i = idx; i < argc; ++i)
      if((tmperr = touch(argv[i], &ctx)) && ctx.vrbs) {
        fprintf(stderr, error[FILEOP_ERROR], tmperr, argv[i]);
        fflush(stderr);
      }
  
  if(input) {
    char fpname[UINT16_MAX];
    size_t len;
    
    while(fgets(fpname, UINT16_MAX, input)) {
      len = strlen(fpname);
      if(len > 1) {
        fpname[len-1] = 0; // - newline.
        if((tmperr = touch(fpname, &ctx)) && ctx.vrbs) {
          fprintf(stderr, error[FILEOP_ERROR], tmperr, fpname);
          fflush(stderr);
        }
      }
    }
    fclose(input);
  }
  
  exit(EXIT_SUCCESS);
}


