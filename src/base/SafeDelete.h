
#pragma once


#ifndef SAFE_DELETE
#define SAFE_DELETE(x) do{ delete (x); (x)=NULL; } while(0)
#endif

#ifndef SAFE_DECREF
#define SAFE_DECREF(x) do{ if (x) { (x)->DecRef(); (x)=NULL; } } while(0)
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) do{ delete[] (x); (x)=NULL; } while(0)
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(x) do{ free(x); (x)=NULL; } while(0)
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x)	do{ if (x) { (x)->Release(); (x)=NULL; } } while(0)
#endif

#ifndef SAFE_DEL_RELEASE
#define SAFE_DEL_RELEASE(x)	do{ (x)->Release(); delete (x); (x)=NULL; } while(0)
#endif


