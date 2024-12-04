typedef struct lua_State lua_State;
#define LUA_MULTRET	(-1)
#define LUA_REGISTRYINDEX	(-10000)
#define LUA_ENVIRONINDEX	(-10001)
#define LUA_GLOBALSINDEX	(-10002)
#define lua_upvalueindex(i)	(LUA_GLOBALSINDEX-(i))
#define LUA_YIELD	1
#define LUA_ERRRUN	2
#define LUA_ERRSYNTAX	3
#define LUA_ERRMEM	4
#define LUA_ERRERR	5
#define LUA_TNONE		(-1)
#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8
#define LUA_MINSTACK	20
#define LUA_GCSTOP		0
#define LUA_GCRESTART		1
#define LUA_GCCOLLECT		2
#define LUA_GCCOUNT		3
#define LUA_GCCOUNTB		4
#define LUA_GCSTEP		5
#define LUA_GCSETPAUSE		6
#define LUA_GCSETSTEPMUL	7
#define LUA_HOOKCALL	0
#define LUA_HOOKRET	1
#define LUA_HOOKLINE	2
#define LUA_HOOKCOUNT	3
#define LUA_HOOKTAILRET 4
#define LUA_MASKCALL	(1 << LUA_HOOKCALL)
#define LUA_MASKRET	(1 << LUA_HOOKRET)
#define LUA_MASKLINE	(1 << LUA_HOOKLINE)
#define LUA_MASKCOUNT	(1 << LUA_HOOKCOUNT)
#define LUA_IDSIZE 60
typedef void (*lua_Hook) (void * lua_State, lua_Debug * ar);
typedef struct lua_Debug {
  int event;
  const char *name;	/* (n) */
  const char *namewhat;	/* (n) `global', `local', `field', `method' */
  const char *what;	/* (S) `Lua', `C', `main', `tail' */
  const char *source;	/* (S) */
  int currentline;	/* (l) */
  int nups;		/* (u) number of upvalues */
  int linedefined;	/* (S) */
  int lastlinedefined;	/* (S) */
  char short_src[LUA_IDSIZE]; /* (S) */
  /* private part */
  int i_ci;  /* active function */
}lua_Debug;

// LUA_API int lua_sethook (lua_State *L, lua_Hook func, int mask, int count);
typedef void *(*lua_sethookFuncType)(void *lua_State, lua_Hook callback, int mask, int count);
void *(*lua_sethook)(void *lua_State, lua_Hook callback, int mask, int count);

// void lua_settop (lua_State *L, int index);
typedef void (*lua_settopFuncType)(void *lua_State, int index);
void (*lua_settop)(void *lua_State, int index);

// int lua_pcall (lua_State *L, int nargs, int nresults, int msgh);
typedef int (*lua_pcallFuncType)(void *lua_State, int nargs, int nresults, int errFunc);
int (*lua_pcall)(void *lua_State, int nargs, int nresults, int errFunc);

// void lua_setfield (lua_State *L, int index, const char *k);
typedef void (*lua_setfieldFuncType)(void *lua_State, int index, const char *cFunctionName);
void (*lua_setfield)(void *lua_State, int index, const char *cFunctionName);

// const char *lua_tolstring (lua_State *L, int index, size_t *len);
typedef const char *(*lua_tolstringFuncType)(void *lua_State, int index, size_t *len);
const char *(*lua_tolstring)(void *lua_State, int index, size_t *len);

// void lua_pushcclosure (lua_State *L, lua_CFunction fn, int n);
typedef void (*lua_pushcclosureFuncType)(void *lua_State, void *cFunction, int n);
void (*lua_pushcclosure)(void *lua_State, void *cFunction, int n);

//#define lua_setglobal(L,s)
typedef void *(*lua_setglobalFuncType)(void *lua_State, const char *cFunctionName);
void *(*lua_setglobal)(void *lua_State, const char *cFunctionName);

// LUA_API int lua_getinfo (lua_State *L, const char *what, lua_Debug *ar);
typedef void *(*lua_getinfoFuncType)(void *lua_State, const char *what, lua_Debug *ar);
void *(*lua_getinfo)(void *lua_State, const char *what, lua_Debug *ar);