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
#define GOBJECT_NAMESPACE "Glib\\GObject"

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
	zend_bool is_registered;
} gobject_type_object;

typedef struct _gobject_signal_object {
	zend_object std;
	guint signal_id;
	GSignalFlags flags;
	zval *param_types; // array
	GType return_type;
	zend_fcall_info class_closure_fci;
	zend_fcall_info_cache class_closure_fci_cache;
	zend_fcall_info accumulator_fci;
	zend_fcall_info_cache accumulator_fci_cache;
} gobject_signal_object;

typedef struct _gobject_gobject_object {
	zend_object std;
	GObject *gobject;
	GSList *closures;
} gobject_gobject_object;

#define __php_objstore_object(obj_zval) ((gobject_gobject_object *)zend_object_store_get_object(obj_zval TSRMLS_CC))
#define __php_gobject_ptr(obj_zval) (__php_objstore_object(obj_zval)->gobject)

ZEND_BEGIN_MODULE_GLOBALS(gobject)
ZEND_END_MODULE_GLOBALS(gobject)

ZEND_EXTERN_MODULE_GLOBALS(gobject)

#ifdef ZTS
# define GOBJECT_G(v) TSRMG(gobject_globals_id, zend_gobject_globals *, v)
#else
# define GOBJECT_G(v) (gobject_globals.v)
#endif

// extension
PHP_MINIT_FUNCTION(gobject);
PHP_MSHUTDOWN_FUNCTION(gobject);
PHP_RINIT_FUNCTION(gobject);
PHP_RSHUTDOWN_FUNCTION(gobject);
PHP_MINFO_FUNCTION(gobject);
PHP_GINIT_FUNCTION(gobject);

// component init
PHP_MINIT_FUNCTION(gobject_paramspec);
PHP_MINIT_FUNCTION(gobject_type);
PHP_MINIT_FUNCTION(gobject_gobject);
PHP_RINIT_FUNCTION(gobject_gobject);
PHP_MINIT_FUNCTION(gobject_closure);
PHP_MINIT_FUNCTION(gobject_signal);

// component shutdown
PHP_MSHUTDOWN_FUNCTION(gobject_type);
PHP_MSHUTDOWN_FUNCTION(gobject_gobject);
PHP_MSHUTDOWN_FUNCTION(gobject_signal);

PHP_RSHUTDOWN_FUNCTION(gobject_gobject);

// api
extern zend_class_entry *gobject_ce_gobject;

GClosure *php_gobject_closure_new(GObject *gobject, zend_fcall_info fci, zend_fcall_info_cache fci_cache, zval ***params, int params_count TSRMLS_DC);
void php_gobject_closure_marshal(GClosure *closure, GValue *return_value, guint n_param_values, const GValue *param_values, gpointer invocation_hint, gpointer marshal_data);
zend_object_value gobject_gobject_object_new(zend_class_entry *ce TSRMLS_DC);
zend_bool gvalue_to_zval(const GValue *gvalue, zval *zvalue TSRMLS_DC);
zend_bool zval_to_gvalue(const zval *zvalue, GValue *gvalue);
GType g_type_from_phpname(const char *name);
char* phpname_from_gclass(const gchar *gclass);

#endif	/* PHP_GOBJECT_EXT_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
