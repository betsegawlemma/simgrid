#ifndef _XBT_JCONTEXT_H
#define _XBT_JCONTEXT_H

#include "portable.h"
#include "xbt/misc.h"

#include "xbt/xbt_context_private.h"
#include "java/jmsg.h"
#include "java/jmsg_process.h"

SG_BEGIN_DECL()

typedef struct s_xbt_jcontext
{
	XBT_CTX_BASE_T;
   	jobject jprocess;  				/* the java process instance binded with the msg process structure							*/
	JNIEnv* jenv;	  				/* jni interface pointer associated to this thread											*/
}s_xbt_jcontext_t,* xbt_jcontext_t;

SG_END_DECL()

#endif /* !_XBT_JCONTEXT_H */
