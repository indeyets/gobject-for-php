/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2010 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Alexey Zakhlestin <indeyets@php.net>                         |
  +----------------------------------------------------------------------+
*/

#ifndef PHP_GOBJECT_EXT_H
#define PHP_GOBJECT_EXT_H

#define PHP_GOBJECT_VERSION "0.1.0-dev"
#define GOBJECT_NAMESPACE "Glib\\Gobject"

extern zend_module_entry gobject_module_entry;
#define phpext_gobject_ptr &gobject_module_entry

#ifdef PHP_WIN32
# define PHP_GOBJECT_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
# define PHP_GOBJECT_API __attribute__ ((visibility("default")))
#else
# define PHP_GOBJECT
#endif

#ifdef ZTS
# include "TSRM.h"
#endif

#include <glib-object.h>

typedef struct _gobject_paramspec_object {
	zend_object std;
	GParamSpec *paramspec;
} gobject_paramspec_object;

typedef struct _gobject_type_object {
	zend_object std;
	GType gtype;
	GType parent;
	zval *name;
	zval *properties; // ArrayObject
	zval *signals;    // ArrayObject
	zval *interfaces; // ArrayObject
} gobject_type_object;

PHP_MINIT_FUNCTION(gobject);
PHP_MSHUTDOWN_FUNCTION(gobject);
PHP_RINIT_FUNCTION(gobject);
PHP_RSHUTDOWN_FUNCTION(gobject);
PHP_MINFO_FUNCTION(gobject);

PHP_MINIT_FUNCTION(gobject_paramspec);
PHP_MINIT_FUNCTION(gobject_type);

ZEND_BEGIN_MODULE_GLOBALS(gobject)
ZEND_END_MODULE_GLOBALS(gobject)

#ifdef ZTS
# define GOBJECT_G(v) TSRMG(gobject_globals_id, zend_gobject_globals *, v)
#else
# define GOBJECT_G(v) (gobject_globals.v)
#endif

#endif	/* PHP_GOBJECT_EXT_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
