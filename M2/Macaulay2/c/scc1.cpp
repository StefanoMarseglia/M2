/*		Copyright 1993,2010 by Daniel R. Grayson		*/

#include "scc.h"
#include <sstream>

FILE *dependfile;
const char *targetname;
const char *outfilename;
static bool stop_after_dep = FALSE;
bool do_cxx = FALSE;
bool do_this_cxx = FALSE;
bool noline = FALSE;
bool nomacros = FALSE;
bool arraychks = TRUE;
bool casechks = TRUE;
bool compilerThreadLocal = FALSE;
bool pthreadThreadLocal = TRUE;
bool gdbm_ronly = FALSE;


static char Copyright[] = "Copyright 1993, 2010, by Daniel R. Grayson";
static char Version[]   = "Safe C - version 2.0";

char *getmem(unsigned n) {
     char *p = reinterpret_cast<char*>(GC_MALLOC(n));	/* GC_MALLOC clears the memory */
     if (p == NULL) fatal("out of memory");
     return p;
     }

static const char *progname;
scope global_scope;


node newnode1(unsigned int len, enum TAG tag) {
     node p = (node) getmem(len);
     memset(p,0x00,len);
     p->tag = tag;
     return p;
     }

char *strnperm(const char *s, unsigned n){
     char *t = getmem(n+1);
     strncpy(t,s,n);
     t[n]='\0';
     return t;
     }

char *strperm(const char *s){
     return strnperm(s,strlen(s));
     }

const char *intToString(int n){
     int sign = 1;
     static char s[20];
     int i;
     if (n == 0) return "0";
     if (n < 0) sign = -1, n = -n;
     i = 19;
     s[i--]=0;
     while (n>0) {
	  s[i] = (char)(n%10 + '0');
	  n/=10;
	  if (n==0) break;
	  i--;
	  }
     if (sign == -1) s[--i] = '-';
     return s+i;
     }

int substr(const char *s, const char *t){
     assert(s != NULL);
     assert(t != NULL);
     while (*s) {
	  if (*t != *s) return FALSE;
	  s++;
	  t++;
	  }
     return TRUE;
     }

int strequaln(const char *s, const char *t, unsigned int tlen){ /* s is null-terminated, but tlen is the length of t */
     assert(s != NULL);
     assert(t != NULL);
     while (*s && tlen>0) {
	  if (*t != *s) return FALSE;
	  s++;
	  t++, tlen--;
	  }
     return *s==0 && tlen==0;
     }

const char *tail(const char *s){
     const char *u = NULL;
     for (; *s; s++) if (*s == '.') u = s;
     return u == NULL ? s : u;
     }

const char *BaseName(const char *s) {
     const char *u = s;
     for (; *s; s++) if (*s == '/') u = s+1;
     return u;
     }

const char *newsuffix(const char *s, const char *suf){
     const char *t = tail(s);
     unsigned int len = t-s;
     char *u = getmem(len+1+strlen(suf));
     strncpy(u,s,len);
     strcpy(u+len,suf);
     return u;
     }

const char *newsuffixbase(const char *s, const char *suf){
     const char *t;
     char* u;
     unsigned int len;
     s = BaseName(s);
     t = tail(s);
     len = t-s;
     u = getmem(len+1+strlen(suf));
     strncpy(u,s,len);
     strcpy(u+len,suf);
     return u;
     }
// we would like this to be const we can't because POS is in a union, so it can't have a constructor.
struct POS empty_pos;

static char declarations_header[] = "\
/* included from " __FILE__ "*/\n\
\n\
#ifndef GC_MALLOC\n\
#ifndef _REENTRANT\n\
#define _REENTRANT\n\
#endif\n\
#include <pthread.h>\n\
#define GC_THREADS\n\
#include <gc/gc.h>\n\
#endif\n\
#include <../system/mutex.h>\n\
#include <../system/interp.hpp>\n\
#ifndef SCC_HEADER\n\
#define SCC_HEADER\n\
#ifdef __cplusplus\n\
  #define BASECLASS : public our_new_delete\n\
  #include \"newdelete.hpp\"\n\
#else\n\
  #define BASECLASS\n\
#endif\n\
#endif\n\
\n\
#if defined(__cplusplus)\n\
  extern \"C\" {\n\
#endif\n\
\n\
#ifndef SAFEC_basic_typedefs_defined\n\
#define SAFEC_basic_typedefs_defined\n\
typedef char M2_bool;\n\
struct tagged_union { unsigned short type_; };\n\
#endif\n\
\n\
";

static char declarations_trailer[] = "\
\n\
#if defined(__cplusplus)\n\
  }\n\
#endif\n\
\n\
";

static char code_header[] = "\
#include \"scc-core.h\"\n\
#include \"../system/supervisorinterface.h\"\n\
";

static void readsetup(scope v){
     init_dictionary(v);
     read_setup();
     }

static void usage() {
  printf("%s [options] foo.d ...\n",progname);
  printf("    --help        display this usage message\n");
  printf("    -cxx          generate a C++ file foo.cc instead of foo.c\n");
  printf("    -v            show version\n");
  printf("    -pthreadlocal use get/set specific instead of __thread\n");
  printf("    -dep          stop after creating foo.dep.tmp and foo.sig.tmp\n");
  printf("    -noline       insert no source code line numbers\n");
  printf("    -sig          stop after creating signature file foo.sig.tmp\n");
  printf("    -typecodes    print typecodes (from typecode.db), then stop\n");
  printf("    -typecodefile generate typecode.h file containing typecode definitions\n");
  printf("    -nomacros     do not parse internal macro definitions\n");
  printf("    -noarraychks  insert no array bound checking code\n");
  printf("    -nocasechks   insert no type case checking code\n");
  printf("    -O            optimize: no array bound checking, no type case checking\n");
  printf("    -tabwidth N   set tab width (default 8; affects column number in error messages)\n");
  printf("    -yydebug      debug the parser\n");
  printf("    -debug        set debugging mode on, write symbol table, list of types, and list of strings to foo.sym\n");
  printf("    -Ixxx         append xxx to the path used for finding *.sig files, initially \".\"\n");
  printf("    -ronly        open typecode.db in read only mode\n");
}

int main(int argc, char **argv){
     int i;
     char *p;
     GC_INIT();
     progname = BaseName(argv[0]);
     yyinit();
     for (p=argv[0]; *p; p++) if (*p=='/') progname = p+1;
     for (i=1; i<argc; i++) {
     	  if (EQUAL == strcmp(argv[i],"--help")) {
	       usage();
	       exit(0);
	       }
     	  if (EQUAL == strcmp(argv[i],"-dep")) {
	       stop_after_dep = TRUE;
	       continue;
	       }
	  if (EQUAL == strcmp(argv[i],"-ronly")) {
	    gdbm_ronly = TRUE;
	    continue;
	  }
     	  if (EQUAL == strcmp(argv[i],"-cxx")) {
	       do_cxx = TRUE;
	       continue;
	       }
     	  if (EQUAL == strcmp(argv[i],"-noline")) {
	       noline = TRUE;
	       continue;
	       }
	  if (EQUAL == strcmp(argv[i],"-pthreadlocal")) {
	       pthreadThreadLocal=TRUE;
               compilerThreadLocal=FALSE;
               continue;
	       }
     	  if (EQUAL == strcmp(argv[i],"-typecodes")) {
	       printtypecodes();
	       return 0;
	       }
	  if (EQUAL == strcmp(argv[i],"-typecodefile"))
	    {
			FILE* fp = fopen("typecodes.h","w");
			if(fp==NULL)
			{
				abort();
			}
			printTypeCodesToFile(fp);
			fclose(fp);
			return 0;
	    }
     	  if (EQUAL == strcmp(argv[i],"-noarraychks")) {
	       arraychks = FALSE;
	       continue;
	       }
     	  if (EQUAL == strcmp(argv[i],"-nocasechks")) {
	       casechks = FALSE;
	       continue;
	       }
	  if (EQUAL == strcmp(argv[i],"-nomacros")) {
	       nomacros = TRUE;
	       continue;
	       }
     	  if (EQUAL == strcmp(argv[i],"-O")) {
	       arraychks = FALSE;
	       casechks = FALSE;
	       continue;
	       }
	  if (EQUAL == strcmp(argv[i],"-tabwidth")) {
	       i++;
	       if (i < argc) tabwidth = atoi(argv[i]);
	       continue;
	       }
	  if (EQUAL == strcmp(argv[i],"-yydebug")) {
	       yydebug = 1;
	       continue;
	       }
	  if (EQUAL == strcmp(argv[i],"-debug")) {
	       debug = TRUE;
	       continue;
	       }
	  if (EQUAL == strcmp(argv[i],"-v")) {
	       puts(Version);
     	       puts(Copyright);
	       continue;
	       }
     	  if ('-' == argv[i][0] && 'I' == argv[i][1]) {
	       if (argv[i][2] == 0) {
		    error("-I option: missing directory");
		    usage();
		    exit(1);
		    }
	       char buf[256];
	       strcpy(buf,sigpath);
	       strcat(buf,":");
	       strcat(buf,argv[i]+2);
	       sigpath = strperm(buf);
	       continue;
	       }
	  if ('-' == argv[i][0]) {
	       error("unrecognized option %s\n",argv[i]);
	       usage();
	       exit(1);
	       }		  
	  if ( EQUAL == strcmp(".d",tail(argv[i])) || EQUAL == strcmp(".dd",tail(argv[i])) ) {
		  if(gdbm_ronly==false && stop_after_dep==false)
		  {
			  //this provides backwards compatability with non-scons build system.
			  FILE* fp = fopen("typecodes.h","w");
			  if(fp==NULL)
			  {
				  abort();
			  }
			  printTypeCodesToFile(fp);
			  fclose(fp);
			  return 0;
		  }
	       node f;
	       do_this_cxx = do_cxx || EQUAL == strcmp(".dd",tail(argv[i]));
	       global_scope = newoftype(struct SCOPE);
	       readsetup(global_scope);
	       targetname = newsuffixbase(argv[i],"");
	       f = readfile(argv[i]);
	       if (debug) {
		    const char *n = newsuffixbase(argv[i],".out");
		    if (NULL == freopen(n,"w", stdout)) {
			 fatal("can't open file %s",n);
			 }
		    d_put("After parsing:\n");
		    d_pp(f);
		    fflush(stdout);
		    }
	       outfilename = newsuffixbase(argv[i], do_this_cxx ? "-tmp.cc" : "-tmp.c");
	       {
		    const char *n = newsuffixbase(argv[i],".dep.tmp");
		    dependfile = fopen(n,"w");
		    if (dependfile == NULL) fatal("can't open file %s",n);
		    }
	       f = chkprogram(f);
	       if (debug) {
		    const char *n = newsuffixbase(argv[i],".log");
		    if (NULL == freopen(n,"w", stdout)) {
			 fatal("can't open file %s",n);
			 }
		    pprintl(f);
		    }
	       {
		    node t = global_scope->signature;
		    const char *n = newsuffixbase(argv[i],".sig.tmp");
		    if (NULL == freopen(n,"w", stdout)) {
			 fatal("can't open file %s",n);
			 }
		    printf("-- generated by %s\n\n",progname);
		    while (t != NULL) {
			 dprint(CAR(t));
			 d_put(";\n");
			 t = CDR(t);
			 }
		    }
	       if (stop_after_dep) quit();
	       checkfordeferredsymbols();
	       if (debug) {
		    const char *n = newsuffixbase(argv[i],".sym");
		    if (NULL == freopen(n,"w", stdout)) {
			 fatal("can't open file %s",n);
			 }
		    printsymboltable();
		    printtypelist();
		    printstringlist();
		    }
	       if (n_errors > 0) {
		    quit();
		    }
	       if (TRUE) {
		    const char *n = newsuffixbase(argv[i],"-exports.h.tmp");
		    if (NULL == freopen(n,"w", stdout)) {
			 fatal("can't open file %s",n);
			 }
		    printf("#ifndef %s_included\n",targetname);
		    printf("#define %s_included\n",targetname);
		    declarationsstrings = reverse(declarationsstrings);
		    while (declarationsstrings) {
			 node s = unpos(car(declarationsstrings));
			 assert(isstrconst(s));
			 d_put_unescape(s->body.string_const.characters);
			 d_put("\n");
			 declarationsstrings = cdr(declarationsstrings);
			 }
		    d_put(declarations_header);
		    printf("#include <typecodes.h>");
		    /* printtypecodes(); */
			cprintIncludeList();
		    cprinttypes();
		    d_put(declarations_trailer);
		    d_put("#endif\n");
		    }
	       if (TRUE) {
		    if (NULL == freopen(outfilename,"w", stdout)) {
			 fatal("can't open file %s",outfilename);
			 }
		    printf("#include \"%s\"\n",newsuffixbase(argv[i],"-exports.h"));
		    d_put(code_header);
		    headerstrings = reverse(headerstrings);
		    while (headerstrings) {
			 locn(car(headerstrings));
			 d_printpos();
			 node s = unpos(car(headerstrings));
			 assert(isstrconst(s));
			 d_put_unescape(s->body.string_const.characters);
			 d_put("\n");
			 locn(NULL);
			 headerstrings = cdr(headerstrings);
			 }
			std::stringstream ss;
		    generateCSemi(f, ss);
			d_put(ss.str().c_str());
		    }
	       }
	  else {
	       fprintf(stderr,"unknown file type %s\n",argv[i]);
	       usage();
	       exit(1);
	       }
	  }
     quit();
     return 0;
     }

     

/*
# Local Variables:
# compile-command: "make -C $M2BUILDDIR/Macaulay2/c "
# End:
*/