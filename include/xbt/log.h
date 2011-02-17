/* log - a generic logging facility in the spirit of log4j                  */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup XBT_log
 *  @brief A generic logging facility in the spirit of log4j (grounding feature)
 *
 *
 */

/** \defgroup XBT_log_cats Existing log categories
 *  \ingroup XBT_log
 *  \brief (automatically extracted) 
 *     
 *  This is the list of all existing log categories in SimGrid.
 *  This list was automatically extracted from the source code by
 *  the src/xbt_log_extract_hierarchy utility.
 *     
 *  You can thus be certain that it is uptodate, but it may somehow
 *  lack a final manual touch.
 *  Anyway, nothing's perfect ;)
 */

/* XBT_LOG_MAYDAY: define this to replace the logging facilities with basic
   printf function. Useful to debug the logging facilities themselves */
#undef XBT_LOG_MAYDAY
//#define XBT_LOG_MAYDAY

#ifndef _XBT_LOG_H_
#define _XBT_LOG_H_

#include "xbt/misc.h"
#include <stdarg.h>
SG_BEGIN_DECL()
/**\brief Log priorities
 * \ingroup XBT_log
 *
 * The different existing priorities.
*/
typedef enum {
  xbt_log_priority_none = 0,    /* used internally (don't poke with) */
  xbt_log_priority_trace = 1,          /**< enter and return of some functions */
  xbt_log_priority_debug = 2,          /**< crufty output  */
  xbt_log_priority_verbose = 3,        /**< verbose output for the user wanting more */
  xbt_log_priority_info = 4,           /**< output about the regular functionning */
  xbt_log_priority_warning = 5,        /**< minor issue encountered */
  xbt_log_priority_error = 6,          /**< issue encountered */
  xbt_log_priority_critical = 7,       /**< major issue encountered */

  xbt_log_priority_infinite = 8,       /**< value for XBT_LOG_STATIC_THRESHOLD to not log */

  xbt_log_priority_uninitialized = -1   /* used internally (don't poke with) */
} e_xbt_log_priority_t;


/*
 * define NLOG to disable at compilation time any logging request
 * define NDEBUG to disable at compilation time any logging request of priority below INFO
 */


/**
 * @def XBT_LOG_STATIC_THRESHOLD
 * @ingroup XBT_log
 *
 * All logging requests with priority < XBT_LOG_STATIC_THRESHOLD are disabled at
 * compile time, i.e., compiled out.
 */
#ifdef NLOG
#  define XBT_LOG_STATIC_THRESHOLD xbt_log_priority_infinite
#else

#  ifdef NDEBUG
#    define XBT_LOG_STATIC_THRESHOLD xbt_log_priority_verbose
#  else                         /* !NLOG && !NDEBUG */

#    ifndef XBT_LOG_STATIC_THRESHOLD
#      define XBT_LOG_STATIC_THRESHOLD xbt_log_priority_none
#    endif                      /* !XBT_LOG_STATIC_THRESHOLD */
#  endif                        /* NDEBUG */
#endif                          /* !defined(NLOG) */

/* Transforms a category name to a global variable name. */
#define _XBT_LOGV(cat)   _XBT_LOG_CONCAT(_simgrid_log_category__, cat)
#define _XBT_LOG_CONCAT(x,y) x ## y

/* The root of the category hierarchy. */
#define XBT_LOG_ROOT_CAT   root

/* The whole tree of categories is connected by setting the address of
 * the parent category as a field of the child one.
 *
 * In strict ansi C, we are allowed to initialize a variable with "a
 * pointer to an lvalue designating an object of static storage
 * duration" [ISO/IEC 9899:1999, Section 6.6].
 * 
 * Unfortunately, Visual C builder does not target any standard
 * compliance, and C99 is not an exception to this unfortunate rule.
 * 
 * So, we work this around by adding a XBT_LOG_CONNECT() macro,
 * allowing to connect a child to its parent. It should be used
 * during the initialization of the code, before the child category
 * gets used.
 * 
 * When compiling with gcc, this is not necessary (XBT_LOG_CONNECT
 * defines to nothing). When compiling with MSVC, this is needed if
 * you don't want to see your child category become a child of root
 * directly.
 */
#if defined(_MSC_VER)
# define _XBT_LOG_PARENT_INITIALIZER(parent) NULL
# define XBT_LOG_CONNECT(parent_cat,child)       _XBT_LOGV(child).parent = &_XBT_LOGV(parent_cat)
#else
# define _XBT_LOG_PARENT_INITIALIZER(parent) &_XBT_LOGV(parent)
# define XBT_LOG_CONNECT(parent_cat,child)      /*  xbt_assert(_XBT_LOGV(child).parent == &_XBT_LOGV(parent_cat)) */
#endif

/* XBT_LOG_NEW_SUBCATEGORY_helper:
 * Implementation of XBT_LOG_NEW_SUBCATEGORY, which must declare "extern parent" in addition
 * to avoid an extra declaration of root when XBT_LOG_NEW_SUBCATEGORY is called by
 * XBT_LOG_NEW_CATEGORY */
#define XBT_LOG_NEW_SUBCATEGORY_helper(catName, parent, desc) \
    XBT_EXPORT_NO_IMPORT(s_xbt_log_category_t) _XBT_LOGV(catName) = {       \
        _XBT_LOG_PARENT_INITIALIZER(parent),            \
        NULL /* firstChild */,                          \
	NULL /* nextSibling */,                         \
        #catName,                                       \
        xbt_log_priority_uninitialized /* threshold */, \
        1 /* isThreshInherited */,                      \
        NULL /* appender */,                            \
	NULL /* layout */,                              \
	1 /* additivity */                              \
    }
/**
 * \ingroup XBT_log
 * \param catName name of new category
 * \param parent father of the new category in the tree
 * \param desc string describing the purpose of this category
 * \hideinitializer
 *
 * Defines a new subcategory of the parent. 
 */
#define XBT_LOG_NEW_SUBCATEGORY(catName, parent, desc)    \
    extern s_xbt_log_category_t _XBT_LOGV(parent); \
    XBT_LOG_NEW_SUBCATEGORY_helper(catName, parent, desc) \

/**
 * \ingroup XBT_log  
 * \param catName name of new category
 * \param desc string describing the purpose of this category
 * \hideinitializer
 *
 * Creates a new subcategory of the root category.
 */
# define XBT_LOG_NEW_CATEGORY(catName,desc)  \
   XBT_LOG_NEW_SUBCATEGORY_helper(catName, XBT_LOG_ROOT_CAT, desc)


/**
 * \ingroup XBT_log  
 * \param cname name of the cat
 * \hideinitializer
 *
 * Indicates which category is the default one.
 */

#if defined(XBT_LOG_MAYDAY) || defined(SUPERNOVAE_MODE) /*|| defined (NLOG) * turning logging off */
# define XBT_LOG_DEFAULT_CATEGORY(cname)
#else
# define XBT_LOG_DEFAULT_CATEGORY(cname) \
	 static xbt_log_category_t _XBT_LOGV(default) _XBT_GNUC_UNUSED = &_XBT_LOGV(cname)
#endif

/**
 * \ingroup XBT_log  
 * \param cname name of the cat
 * \param desc string describing the purpose of this category
 * \hideinitializer
 *
 * Creates a new subcategory of the root category and makes it the default
 * (used by macros that don't explicitly specify a category).
 */
# define XBT_LOG_NEW_DEFAULT_CATEGORY(cname,desc)        \
    XBT_LOG_NEW_CATEGORY(cname,desc);                   \
    XBT_LOG_DEFAULT_CATEGORY(cname)

/**
 * \ingroup XBT_log  
 * \param cname name of the cat
 * \param parent name of the parent
 * \param desc string describing the purpose of this category
 * \hideinitializer
 *
 * Creates a new subcategory of the parent category and makes it the default
 * (used by macros that don't explicitly specify a category).
 */
#define XBT_LOG_NEW_DEFAULT_SUBCATEGORY(cname, parent, desc) \
    XBT_LOG_NEW_SUBCATEGORY(cname, parent, desc);            \
    XBT_LOG_DEFAULT_CATEGORY(cname)

/**
 * \ingroup XBT_log  
 * \param cname name of the cat
 * \hideinitializer
 *
 * Indicates that a category you'll use in this file (to get subcategories of it, 
 * for example) really lives in another file.
 */

#define XBT_LOG_EXTERNAL_CATEGORY(cname) \
   extern s_xbt_log_category_t _XBT_LOGV(cname)

/**
 * \ingroup XBT_log
 * \param cname name of the cat
 * \hideinitializer
 *
 * Indicates that the default category of this file was declared in another file.
 */

#define XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(cname) \
   XBT_LOG_EXTERNAL_CATEGORY(cname);\
   XBT_LOG_DEFAULT_CATEGORY(cname)

/* Functions you may call */

XBT_PUBLIC(void) xbt_log_control_set(const char *cs);

/* Forward declarations */
typedef struct xbt_log_appender_s s_xbt_log_appender_t,
    *xbt_log_appender_t;
typedef struct xbt_log_layout_s s_xbt_log_layout_t, *xbt_log_layout_t;
typedef struct xbt_log_event_s s_xbt_log_event_t, *xbt_log_event_t;
typedef struct xbt_log_category_s s_xbt_log_category_t,
    *xbt_log_category_t;

/*
 * Do NOT access any members of this structure directly. FIXME: move to private?
 */
#ifdef _XBT_WIN32
#define XBT_LOG_BUFF_SIZE  16384        /* Size of the static string in which we build the log string */
#else
#define XBT_LOG_BUFF_SIZE 2048  /* Size of the static string in which we build the log string */
#endif
struct xbt_log_category_s {
  xbt_log_category_t parent;
  xbt_log_category_t firstChild;
  xbt_log_category_t nextSibling;
  const char *name;
  int threshold;
  int isThreshInherited;
  xbt_log_appender_t appender;
  xbt_log_layout_t layout;
  int additivity;
};

struct xbt_log_event_s {
  xbt_log_category_t cat;
  e_xbt_log_priority_t priority;
  const char *fileName;
  const char *functionName;
  int lineNum;
  va_list ap;
  va_list ap_copy;              /* need a copy to launch dynamic layouts when the static ones overflowed */
#ifdef _XBT_WIN32
  char *buffer;
#else
  char buffer[XBT_LOG_BUFF_SIZE];
#endif
};

/**
 * \ingroup XBT_log_implem
 * \param cat the category (not only its name, but the variable)
 * \param thresholdPriority the priority
 *
 * Programatically alters a category's threshold priority (don't use).
 */
XBT_PUBLIC(void) xbt_log_threshold_set(xbt_log_category_t cat,
                                       e_xbt_log_priority_t
                                       thresholdPriority);

/**
 * \ingroup XBT_log_implem  
 * \param cat the category (not only its name, but the variable)
 * \param app the appender
 *
 * Programatically sets the category's appender.
 * (the prefered interface is throught xbt_log_control_set())
 *
 */
XBT_PUBLIC(void) xbt_log_appender_set(xbt_log_category_t cat,
                                      xbt_log_appender_t app);
/**
 * \ingroup XBT_log_implem  
 * \param cat the category (not only its name, but the variable)
 * \param lay the layout
 *
 * Programatically sets the category's layout.
 * (the prefered interface is throught xbt_log_control_set())
 *
 */
XBT_PUBLIC(void) xbt_log_layout_set(xbt_log_category_t cat,
                                    xbt_log_layout_t lay);

/**
 * \ingroup XBT_log_implem  
 * \param cat the category (not only its name, but the variable)
 * \param additivity whether logging actions must be passed to parent.
 *
 * Programatically sets whether the logging actions must be passed to 
 * the parent category.
 * (the prefered interface is throught xbt_log_control_set())
 *
 */
XBT_PUBLIC(void) xbt_log_additivity_set(xbt_log_category_t cat,
                                        int additivity);

/** @brief create a new simple layout 
 *
 * This layout is not as flexible as the pattern one
 */
XBT_PUBLIC(xbt_log_layout_t) xbt_log_layout_simple_new(char *arg);
XBT_PUBLIC(xbt_log_layout_t) xbt_log_layout_format_new(char *arg);
XBT_PUBLIC(xbt_log_appender_t) xbt_log_appender_file_new(char *arg);


/* ********************************** */
/* Functions that you shouldn't call  */
/* ********************************** */
XBT_PUBLIC(void) _xbt_log_event_log(xbt_log_event_t ev,
                                    const char *fmt,
                                    ...) _XBT_GNUC_PRINTF(2, 3);

XBT_PUBLIC(int) _xbt_log_cat_init(xbt_log_category_t category,
                                  e_xbt_log_priority_t priority);


XBT_PUBLIC_DATA(s_xbt_log_category_t) _XBT_LOGV(XBT_LOG_ROOT_CAT);


extern xbt_log_appender_t xbt_log_default_appender;
extern xbt_log_layout_t xbt_log_default_layout;

/* ********************** */
/* Public functions again */
/* ********************** */

/**
 * \ingroup XBT_log 
 * \param catName name of the category
 * \param priority minimal priority to be enabled to return true (must be #e_xbt_log_priority_t)
 * \hideinitializer
 *
 * Returns true if the given priority is enabled for the category.
 * If you have expensive expressions that are computed outside of the log
 * command and used only within it, you should make its evaluation conditional
 * using this macro.
 */
#define XBT_LOG_ISENABLED(catName, priority) \
            _XBT_LOG_ISENABLEDV(_XBT_LOGV(catName), priority)

/*
 * Helper function that implements XBT_LOG_ISENABLED.
 *
 * NOTES
 * First part is a compile-time constant.
 * Call to _log_initCat only happens once.
 */
#define _XBT_LOG_ISENABLEDV(catv, priority)                  \
       (priority >= XBT_LOG_STATIC_THRESHOLD                 \
        && priority >= catv.threshold                         \
        && (catv.threshold != xbt_log_priority_uninitialized \
            || _xbt_log_cat_init(&catv, priority)) )

/*
 * Internal Macros
 * Some kludge macros to ease maintenance. See how they're used below.
 *
 * IMPLEMENTATION NOTE: To reduce the parameter passing overhead of an enabled
 * message, the many parameters passed to the logging function are packed in a
 * structure. Since these values will be usually be passed to at least 3
 * functions, this is a win.
 * It also allows adding new values (such as a timestamp) without breaking
 * code. 
 * Setting the LogEvent's valist member is done inside _log_logEvent.
 */
#ifdef _XBT_WIN32
#include <stdlib.h>             /* calloc */
#define _XBT_LOG_EV_BUFFER_ZERO() \
  _log_ev.buffer = (char*) calloc(XBT_LOG_BUFF_SIZE + 1, sizeof(char))
#else
#include <string.h>             /* memset */
#define _XBT_LOG_EV_BUFFER_ZERO() \
  memset(_log_ev.buffer, 0, XBT_LOG_BUFF_SIZE)
#endif

/* Logging Macros */

#ifdef XBT_LOG_MAYDAY
# define XBT_CLOG_(cat, prio, f, ...) \
  fprintf(stderr,"%s:%d:" f "%c", __FILE__, __LINE__, __VA_ARGS__)
# define XBT_CLOG(cat, prio, ...) XBT_CLOG_(cat, prio, __VA_ARGS__, '\n')
# define XBT_LOG(...) XBT_CLOG(0, __VA_ARGS__)
#else
# define XBT_CLOG_(catv, prio, ...)                                     \
  do {                                                                  \
    if (_XBT_LOG_ISENABLEDV(catv, prio)) {                              \
      s_xbt_log_event_t _log_ev;                                        \
      _log_ev.cat = &(catv);                                            \
      _log_ev.priority = (prio);                                        \
      _log_ev.fileName = __FILE__;                                      \
      _log_ev.functionName = _XBT_FUNCTION;                             \
      _log_ev.lineNum = __LINE__;                                       \
      _XBT_LOG_EV_BUFFER_ZERO();                                        \
      _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
    }                                                                   \
  }  while (0)
# define XBT_CLOG(cat, prio, ...) XBT_CLOG_(_XBT_LOGV(cat), prio, __VA_ARGS__)
# define XBT_LOG(...) XBT_CLOG_((*_XBT_LOGV(default)), __VA_ARGS__)
#endif

/** @ingroup XBT_log
 *  @hideinitializer
 * \param c the category on which to log
 * \param f the format string
 * \param ... arguments of the format
 *  @brief Log an event at the DEBUG priority on the specified category with these args.
 */
#define XBT_CDEBUG(c, ...) XBT_CLOG(c, xbt_log_priority_debug, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the VERB priority on the specified category with these args.
 */
#define XBT_CVERB(c, ...) XBT_CLOG(c, xbt_log_priority_verbose, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the INFO priority on the specified category with these args.
 */
#define XBT_CINFO(c, ...) XBT_CLOG(c, xbt_log_priority_info, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the WARN priority on the specified category with these args.
 */
#define XBT_CWARN(c, ...) XBT_CLOG(c, xbt_log_priority_warning, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the ERROR priority on the specified category with these args.
 */
#define XBT_CERROR(c, ...) XBT_CLOG(c, xbt_log_priority_error, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the CRITICAL priority on the specified category with these args (CCRITICALn exists for any n<10).
 */
#define XBT_CCRITICAL(c, ...) XBT_CLOG(c, xbt_log_priority_critical, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 * \param f the format string
 * \param ...
 *  @brief Log an event at the DEBUG priority on the default category with these args.
 */
#define XBT_DEBUG(...) XBT_LOG(xbt_log_priority_debug, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the VERB priority on the default category with these args.
 */
#define XBT_VERB(...) XBT_LOG(xbt_log_priority_verbose, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the INFO priority on the default category with these args.
 */
#define XBT_INFO(...) XBT_LOG(xbt_log_priority_info, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the WARN priority on the default category with these args.
 */
#define XBT_WARN(...) XBT_LOG(xbt_log_priority_warning, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the ERROR priority on the default category with these args.
 */
#define XBT_ERROR(...) XBT_LOG(xbt_log_priority_error, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the CRITICAL priority on the default category with these args.
 */
#define XBT_CRITICAL(...) XBT_LOG(xbt_log_priority_critical, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log at TRACE priority that we entered in current function, appending a user specified format.
 */
#define XBT_IN(...) XBT_IN_(__VA_ARGS__, "")
#define XBT_IN_(fmt, ...) \
  XBT_LOG(xbt_log_priority_trace, ">> begin of %s" fmt "%s", \
          _XBT_FUNCTION, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log at TRACE priority that we exited the current function.
 */
#define XBT_OUT() XBT_LOG(xbt_log_priority_trace, "<< end of %s", _XBT_FUNCTION)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log at TRACE priority a message indicating that we reached that point.
 */
#define XBT_HERE() XBT_LOG(xbt_log_priority_trace, "-- was here")

SG_END_DECL()
#endif                          /* ! _XBT_LOG_H_ */
