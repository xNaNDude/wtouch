

```

usage:
wtouch.exe [-(s|r|a|c|m|e)] [-f (<file>|-)] [-t[a|c|m] <time>] [--] [<file> ...]

change modification and access time of file(s) to current system time.

-s		 silent mode.
-r		 ignore files with readonly attribute
  		 (default is to remove then reset it if needed).
-a		 change access time (and others requested times only).
-c		 change creation time (and others requested times only).
-m		 change modification time (and others requested times only).
-t <time>	 where <time> is of the form : MMDDhhmm[[CC]YY][.ss]
  		 set <time> as file(s) times instead of system time.
-ta <time>	 where <time> is of the form : MMDDhhmm[[CC]YY][.ss]
  		 use <time> as access time (override system and -t <time>).
-tc <time>	 where <time> is of the form : MMDDhhmm[[CC]YY][.ss]
  		 use <time> as creation time (override system and -t <time>).
-tm <time>	 where <time> is of the form : MMDDhhmm[[CC]YY][.ss]
  		 use <time> as modification time (override system and -t <time>).
-f <file>	 read file(s) arguments from <file> or stdin when '-',
  		 one file per line, file(s) argument(s) on command line
  		 are still treated before <file>/stdin input.
-e		 treat -f input as voidtools everything output.
--		 the first non option argument always stop option(s) reading,
  		 this option force this stop, allowing file(s) named like
  		 options.

```

note: everything support only ignore header and times/attributes fields.

