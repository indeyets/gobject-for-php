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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <php.h>
#include <zend_interfaces.h>

#include "php_gobject.h"

typedef struct {
	GClosure gclosure;
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;
	zval ***extra_params;
	int extra_params_count;
	zval *swap_data;  // other object for gtk_signal_connect_object
} php_gobject_closure;

static int zend_call_function_with_additional_params(zend_fcall_info *_fci, zend_fcall_info_cache *fci_cache, zend_uint param_count, zval **params[] TSRMLS_DC) /* {{{ */
{
	zend_fcall_info fci = *_fci;
    size_t i;

	fci.param_count = _fci->param_count + param_count;
	fci.params = emalloc(fci.param_count * sizeof (zval **));
	
	for (i = 0; i < _fci->param_count; i++) {
		fci.params[i] = _fci->params[i];
	}
	
	for (i = 0; i < param_count; i++) {
		fci.params[i + _fci->param_count] = params[i];
	}

	int retval = zend_call_function(&fci, fci_cache TSRMLS_CC);

	efree(fci.params);

	return retval;
}
/* }}} */


static void php_gobject_closure_invalidate(gpointer data, GClosure *gclosure)
{
	php_printf("php_gobject_closure_invalidate()\n");
	php_gobject_closure *casted_closure = (php_gobject_closure *) gclosure;

	if (casted_closure->fci.function_name)
		zval_ptr_dtor(&(casted_closure->fci.function_name));

	if (casted_closure->extra_params_count > 0 && casted_closure->extra_params) {
		efree(casted_closure->extra_params);
		casted_closure->extra_params = NULL;
	}
}

static void php_gobject_closure_marshal(GClosure *closure, GValue *return_value, guint n_param_values, const GValue *param_values, gpointer invocation_hint, gpointer marshal_data)
{
	php_printf("php_gobject_closure_marshal()\n");
	php_gobject_closure *casted_closure = (php_gobject_closure *) closure;

	// zval *params = NULL;
	TSRMLS_FETCH();

	// MAKE_STD_ZVAL(params);
	// array_init(params);

	// add parameters to callback
	// if (casted_closure->extra_args != NULL) {
	// 	HashTable *args_hash = Z_ARRVAL_P(casted_closure->extra_args);
	// 	zend_uint argc = 1 + zend_hash_num_elements(args_hash);
	// 
	// 	zend_hash_internal_pointer_reset(args_hash);
	// 
	// 	size_t i;
	// 	for (i = 1; i < argc; i++) {
	// 		zval **ptr;
	// 		zend_hash_get_current_data(args_hash, (void **)&ptr);
	// 		zend_hash_move_forward(args_hash);
	// 
	// 		zval_add_ref(ptr);
	// 		zend_hash_next_index_insert(Z_ARRVAL_P(params), ptr, sizeof(zval *), NULL);
	// 	}
	// }

	// do the call
	zval *retval = NULL;

	// zend_fcall_info_args(&(casted_closure->fci), params TSRMLS_CC);
	casted_closure->fci.retval_ptr_ptr = &retval;

	zend_call_function_with_additional_params(&(casted_closure->fci), &(casted_closure->fci_cache), casted_closure->extra_params_count, casted_closure->extra_params TSRMLS_CC);
	// zend_fcall_info_args_clear(&(casted_closure->fci), 1);

	// zval_ptr_dtor(&params);

	// we do not care about retval now. but, actually, we should
	if (retval)
		zval_ptr_dtor(&retval);
}

GClosure *php_gobject_closure_new(GObject *gobject, zend_fcall_info fci, zend_fcall_info_cache fci_cache, zval ***params, int params_count TSRMLS_DC)
{
	php_printf("php_gobject_closure_new()\n");
	GClosure *closure;

	closure = g_closure_new_object(sizeof(php_gobject_closure), gobject);

	if (!closure) {
		php_error(E_ERROR, "Couldn't create new closure");
		return NULL;
	}

	{ // setting php-parts of structure
		php_gobject_closure *casted_closure = (php_gobject_closure*) closure;

		if (fci.function_name)
			zval_add_ref(&fci.function_name);

		casted_closure->fci = fci;
		casted_closure->fci_cache = fci_cache;
		casted_closure->extra_params = params;
		casted_closure->extra_params_count = params_count;
	}

	g_closure_add_invalidate_notifier(closure, NULL, php_gobject_closure_invalidate);
	g_closure_set_marshal(closure, php_gobject_closure_marshal);

	return closure;
}

PHP_MINIT_FUNCTION(gobject_closure)
{
	// testing
	guint signal_id = g_signal_new("php_test", G_TYPE_OBJECT, G_SIGNAL_RUN_FIRST, 0, NULL, NULL, php_gobject_closure_marshal, G_TYPE_NONE, 0);

	return true;
}
