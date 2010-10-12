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

#include "php_gobject_private.h"

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
	php_gobject_closure *casted_closure = (php_gobject_closure *) gclosure;

	if (casted_closure->fci.function_name) {
		// php_printf("=> php_gobject_closure_invalidate: freeing function_name\n");
		zval_ptr_dtor(&(casted_closure->fci.function_name));
	}

	if (casted_closure->extra_params_count > 0 && casted_closure->extra_params) {
		size_t i;
		for (i = 0; i < casted_closure->extra_params_count; i++) {
			zval_ptr_dtor(casted_closure->extra_params[i]);
			efree(casted_closure->extra_params[i]);
			casted_closure->extra_params[i] = NULL;
		}

		efree(casted_closure->extra_params);
		casted_closure->extra_params = NULL;
	}
}

void php_gobject_closure_marshal(GClosure *closure, GValue *return_value, guint n_param_values, const GValue *param_values, gpointer invocation_hint, gpointer marshal_data)
{
	php_gobject_closure *casted_closure = (php_gobject_closure *) closure;

	TSRMLS_FETCH();

	zval *retval = NULL;
	casted_closure->fci.retval_ptr_ptr = &retval;

	zend_uint param_count = n_param_values + casted_closure->extra_params_count;
	zval ***params = ecalloc(param_count, sizeof(zval **));

	if (n_param_values > 0) {
		guint i;
		for (i = 0; i < n_param_values; i++) {
			const GValue *gvalue = &(param_values[i]);
			GType g_gtype = G_TYPE_FUNDAMENTAL(G_VALUE_TYPE(gvalue));

			zval *param_zvalue = NULL;

			if (g_gtype == G_TYPE_OBJECT) {
				GObject *gobject = g_value_get_object(gvalue);

				gpointer data = g_object_get_data(gobject, "gobject-for-php");

				if (data) {
					param_zvalue = (zval *)data;
					zval_add_ref(&param_zvalue);
				}
			}

			if (NULL == param_zvalue) {
				MAKE_STD_ZVAL(param_zvalue);
				gvalue_to_zval(gvalue, param_zvalue TSRMLS_CC);
			}

			params[i] = emalloc(sizeof(zval *));
			*params[i] = param_zvalue;
		}
	}

	size_t i;
	for (i = 0; i < casted_closure->extra_params_count; i++) {
		params[n_param_values + i] = casted_closure->extra_params[i];
	}

	// php_printf("php_gobject_closure_marshal: calling with %d additional params\n", param_count);
	zend_call_function_with_additional_params(&(casted_closure->fci), &(casted_closure->fci_cache), param_count, params TSRMLS_CC);

	for (i = 0; i < n_param_values; i++) {
		// clean only parameters from gobjects!
		zval_ptr_dtor(params[i]);
		efree(params[i]);
	}
	efree(params);

	// php_printf("php_gobject_closure_marshal: putting retval to %p\n", return_value);
	zval_to_gvalue(retval, return_value, 0 TSRMLS_CC);

	if (retval)
		zval_ptr_dtor(&retval);
}

gboolean php_gobject_closure_accumulator(GSignalInvocationHint *ihint, GValue *out, const GValue *in, gpointer data)
{
	TSRMLS_FETCH();

	// php_printf("php_gobject_closure_accumulator(..., %p, %p, %p)\n", out, in, data);

	zval *signal = php_gobject_signal_get_by_id(ihint->signal_id TSRMLS_CC);
	gobject_signal_object *signal_object = (gobject_signal_object *)zend_objects_get_address(signal TSRMLS_CC);

	zval ***params = ecalloc(2, sizeof(zval **));

	params[0] = emalloc(sizeof(zval *));
	MAKE_STD_ZVAL(*params[0]);
	gvalue_to_zval(out, *params[0] TSRMLS_CC);

	params[1] = emalloc(sizeof(zval *));
	MAKE_STD_ZVAL(*params[1]);
	gvalue_to_zval(in, *params[1] TSRMLS_CC);

	zval *retval = NULL;
	signal_object->accumulator_fci.retval_ptr_ptr = &retval;

	zend_call_function_with_additional_params(&signal_object->accumulator_fci, &signal_object->accumulator_fci_cache, 2, params TSRMLS_CC);

	if (retval)
		zval_ptr_dtor(&retval);

	zval_to_gvalue(*params[0], out, 0 TSRMLS_CC);

	zval_ptr_dtor(params[0]);
	efree(params[0]);
	zval_ptr_dtor(params[1]);
	efree(params[1]);
	efree(params);

	return TRUE;
}

GClosure *php_gobject_closure_new(GObject *gobject, zend_fcall_info fci, zend_fcall_info_cache fci_cache, zval ***params, int params_count TSRMLS_DC)
{
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

		if (params_count > 0) {
			casted_closure->extra_params = ecalloc(params_count, sizeof(zval **));
			casted_closure->extra_params_count = params_count;
		}

		size_t i;
		for (i = 0; i < params_count; i++) {
			zval_add_ref(params[i]);

			casted_closure->extra_params[i] = emalloc(sizeof(zval *));
			*casted_closure->extra_params[i] = *params[i];
		}
	}

	g_closure_add_invalidate_notifier(closure, NULL, php_gobject_closure_invalidate);
	g_closure_set_marshal(closure, php_gobject_closure_marshal);

	return closure;
}

GClosure *php_gobject_closure_new_class(zend_fcall_info fci, zend_fcall_info_cache fci_cache TSRMLS_DC)
{
	GClosure *closure;

	closure = g_closure_new_simple(sizeof(php_gobject_closure), NULL);

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
	}

	g_closure_add_invalidate_notifier(closure, NULL, php_gobject_closure_invalidate);
	g_closure_set_marshal(closure, php_gobject_closure_marshal);

	return closure;
}

PHP_MINIT_FUNCTION(gobject_closure)
{
	// testing
	guint signal_id = g_signal_new("php_test", G_TYPE_OBJECT, G_SIGNAL_RUN_FIRST, 0, NULL, NULL, php_gobject_closure_marshal, G_TYPE_NONE, 0);

	return SUCCESS;
}
