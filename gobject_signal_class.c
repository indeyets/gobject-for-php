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
#include <zend_exceptions.h>

#include <ext/spl/spl_array.h>
#include <ext/spl/spl_exceptions.h>

#include "php_gobject.h"


zend_bool php_gobject_store_signal_association(zval *signal TSRMLS_DC)
{
	gobject_signal_object *object = (gobject_signal_object *)zend_objects_get_address(signal TSRMLS_CC);

	if (object->signal_id == 0)
		return FALSE;

	HashTable *ht = &GOBJECT_G(signals_hash);

	Z_ADDREF_P(signal);
	if (SUCCESS == zend_hash_index_update(ht, object->signal_id, (void*)&signal, sizeof(zval *), NULL))
		return TRUE;

	return FALSE;
}

zend_bool php_gobject_remove_signal_association(guint signal_id TSRMLS_DC)
{
	HashTable *ht = &GOBJECT_G(signals_hash);

	if (SUCCESS == zend_hash_index_del(ht, signal_id))
		return TRUE;

	return FALSE;
}

zval * php_gobject_signal_get_by_id(guint signal_id TSRMLS_DC)
{
	HashTable *ht = &GOBJECT_G(signals_hash);

	void *ptr;
	zend_hash_index_find(ht, signal_id, &ptr);

	zval *retval = *(zval **)ptr;
	// efree(ptr);

	return retval;
}



zend_class_entry *gobject_ce_signal;
static zend_object_handlers *php_gobject_signal_handlers;

void gobject_signal_free_storage(gobject_signal_object *intern TSRMLS_DC)
{
	if (intern->std.guards) {
		zend_hash_destroy(intern->std.guards);
		FREE_HASHTABLE(intern->std.guards);
	}
	
	if (intern->std.properties) {
		zend_hash_destroy(intern->std.properties);
		FREE_HASHTABLE(intern->std.properties);
	}

	if (intern->param_types) {
		zval_ptr_dtor(&intern->param_types);
	}

	if (intern->class_closure_fci.function_name) {
		zval_ptr_dtor(&(intern->class_closure_fci.function_name));
	}

	if (intern->accumulator_fci.function_name) {
		zval_ptr_dtor(&(intern->accumulator_fci.function_name));
	}

	if (intern->signal_id > 0) {
		php_gobject_remove_signal_association(intern->signal_id TSRMLS_CC);
	}

	efree(intern);
}

zend_object_value gobject_signal_object_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;
	gobject_signal_object *object;

	object = emalloc(sizeof(gobject_signal_object));
	object->std.ce = ce;
	object->std.guards = NULL;

	ALLOC_HASHTABLE(object->std.properties);
	zend_hash_init(object->std.properties, zend_hash_num_elements(&ce->default_properties), NULL, ZVAL_PTR_DTOR, 0);

	zval *tmp;
	zend_hash_copy(
		object->std.properties,
		&ce->default_properties,
		(copy_ctor_func_t) zval_add_ref,
		(void *) &tmp,
		sizeof(zval *)
	);

	object->signal_id = 0;
	object->flags = 0;
	object->param_types = NULL;
	object->return_type = G_TYPE_NONE;
	object->class_closure_fci = empty_fcall_info;
	object->class_closure_fci_cache = empty_fcall_info_cache;
	object->accumulator_fci = empty_fcall_info;
	object->accumulator_fci_cache = empty_fcall_info_cache;

	retval.handle = zend_objects_store_put(
		object,
		(zend_objects_store_dtor_t)zend_objects_destroy_object,
		(zend_objects_free_object_storage_t) gobject_signal_free_storage,
		NULL
		TSRMLS_CC
	);
	retval.handlers = php_gobject_signal_handlers;

	return retval;
}


zval *php_gobject_signal_read_property(zval *zobject, zval *prop, int type TSRMLS_DC)
{
	const char *propname = Z_STRVAL_P(prop);
	int proplen = Z_STRLEN_P(prop);

	gobject_signal_object *object = (gobject_signal_object *)zend_objects_get_address(zobject TSRMLS_CC);

	if (       proplen == 5  && strncmp(propname, "flags", 5)        == 0) {
		zval *retval;
		MAKE_STD_ZVAL(retval);
		ZVAL_LONG(retval, object->flags);
		Z_DELREF_P(retval); // setting refcount to 0. receiving side is responsible for adding ref

		return retval;
	} else {
		zend_throw_exception_ex(spl_ce_OutOfBoundsException, 0 TSRMLS_CC, "No way to get this property");
		return NULL;
	}
}

void php_gobject_signal_write_property(zval *zobject, zval *prop, zval *value TSRMLS_DC)
{
	const char *propname = Z_STRVAL_P(prop);
	int proplen = Z_STRLEN_P(prop);

	gobject_signal_object *object = (gobject_signal_object *)zend_objects_get_address(zobject TSRMLS_CC);

	if (proplen == 5 && strncmp(propname, "flags", 5) == 0) {
		convert_to_long(value);
		object->flags = Z_LVAL_P(value);
	} else {
		zend_throw_exception_ex(spl_ce_OutOfBoundsException, 0 TSRMLS_CC, "No way to set this property");
	}
}

zval **php_gobject_signal_get_property_ptr_ptr(zval *object, zval *member TSRMLS_DC)
{
	// we don't want to provide direct access to underlying properties
	return NULL;
}

zend_object_value php_gobject_signal_clone(zval *original_zval TSRMLS_DC)
{
	zend_object_value retval;

	gobject_signal_object *original = (gobject_signal_object *)zend_objects_get_address(original_zval TSRMLS_CC);

	gobject_signal_object *object = emalloc(sizeof(gobject_signal_object));

	object->std.ce = original->std.ce;
	object->std.guards = original->std.guards;

	ALLOC_HASHTABLE(object->std.properties);
	zend_hash_init(object->std.properties, zend_hash_num_elements(original->std.properties), NULL, ZVAL_PTR_DTOR, 0);

	zval *tmp;
	zend_hash_copy(
		object->std.properties,
		original->std.properties,
		(copy_ctor_func_t) zval_add_ref,
		(void *) &tmp,
		sizeof(zval *)
	);

	object->signal_id = 0;
	object->flags = original->flags;

	zval_add_ref(&original->param_types);
	object->param_types = original->param_types;

	object->return_type = original->return_type;
	object->class_closure_fci = original->class_closure_fci;
	object->class_closure_fci_cache = original->class_closure_fci_cache;
	object->accumulator_fci = original->accumulator_fci;
	object->accumulator_fci_cache = original->accumulator_fci_cache;

	retval.handle = zend_objects_store_put(
		object,
		(zend_objects_store_dtor_t)zend_objects_destroy_object,
		(zend_objects_free_object_storage_t) gobject_signal_free_storage,
		NULL
		TSRMLS_CC
	);
	retval.handlers = php_gobject_signal_handlers;

	return retval;
}



ZEND_BEGIN_ARG_INFO_EX(arginfo_Glib_GObject_Signal___construct, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, flags)
	ZEND_ARG_INFO(0, param_types)
	ZEND_ARG_INFO(0, return_type)
	ZEND_ARG_INFO(0, class_closure)
	ZEND_ARG_INFO(0, accumulator)
ZEND_END_ARG_INFO()

PHP_METHOD(Glib_GObject_Signal, __construct)
{
	long flags = 0;
	zval *param_types = NULL;
	char *return_type = NULL;
	int return_type_len = 0;
	zend_fcall_info cc_fci = empty_fcall_info, acc_fci = empty_fcall_info;
	zend_fcall_info_cache cc_fci_cache = empty_fcall_info_cache, acc_fci_cache = empty_fcall_info_cache;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|lz!s!f!f!",
			&flags,
			&param_types, &return_type, &return_type_len,
			&cc_fci, &cc_fci_cache,
			&acc_fci, &acc_fci_cache
		) == FAILURE
	) {
		return;
	}

	gobject_signal_object *object = (gobject_signal_object *)zend_objects_get_address(getThis() TSRMLS_CC);
	object->flags = flags;

	object->class_closure_fci = cc_fci;
	object->class_closure_fci_cache = cc_fci_cache;
	if (!callback_is_empty(&cc_fci)) {
		if (object->class_closure_fci.function_name) {
			zval_add_ref(&object->class_closure_fci.function_name);
		}
	}

	object->accumulator_fci = acc_fci;
	object->accumulator_fci_cache = acc_fci_cache;
	if (!callback_is_empty(&acc_fci)) {
		if (object->accumulator_fci.function_name) {
			zval_add_ref(&object->accumulator_fci.function_name);
		}
	}

	if (param_types) {
		Z_ADDREF_P(param_types);
		object->param_types = param_types;
	}

	if (return_type) {
		GType new_gtype = g_type_from_phpname(return_type);

		if (0 == new_gtype) {
			zend_throw_exception_ex(spl_ce_OutOfBoundsException, 0 TSRMLS_CC, "This class is not registered: %s", return_type);
			return;
		}

		object->return_type = new_gtype;
	}
}


const zend_function_entry gobject_signal_methods[] = {
	// public
	PHP_ME(Glib_GObject_Signal, __construct, arginfo_Glib_GObject_Signal___construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	{NULL, NULL, NULL}
};

PHP_MINIT_FUNCTION(gobject_signal)
{
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, GOBJECT_NAMESPACE, "Signal", gobject_signal_methods);
	gobject_ce_signal = zend_register_internal_class(&ce TSRMLS_CC);
	gobject_ce_signal->create_object  = gobject_signal_object_new;

#define SIGNAL_CONSTANT(k) \
	zend_declare_class_constant_long(gobject_ce_signal, #k, sizeof(#k)-1, G_SIGNAL_##k TSRMLS_CC)

	// GSignalFlags
	SIGNAL_CONSTANT(RUN_FIRST);
	SIGNAL_CONSTANT(RUN_LAST);
	SIGNAL_CONSTANT(RUN_CLEANUP);
	SIGNAL_CONSTANT(NO_RECURSE);
	SIGNAL_CONSTANT(DETAILED);
	SIGNAL_CONSTANT(ACTION);
	SIGNAL_CONSTANT(NO_HOOKS);
	SIGNAL_CONSTANT(FLAGS_MASK);

	// GSignalMatchType
	SIGNAL_CONSTANT(MATCH_ID);
	SIGNAL_CONSTANT(MATCH_DETAIL);
	SIGNAL_CONSTANT(MATCH_CLOSURE);
	SIGNAL_CONSTANT(MATCH_FUNC);
	SIGNAL_CONSTANT(MATCH_DATA);
	SIGNAL_CONSTANT(MATCH_UNBLOCKED);
	SIGNAL_CONSTANT(MATCH_MASK);

	SIGNAL_CONSTANT(TYPE_STATIC_SCOPE);
#undef SIGNAL_CONSTANT

	// standard handlers + overriding
	php_gobject_signal_handlers = malloc(sizeof(zend_object_handlers));
	memcpy(php_gobject_signal_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	php_gobject_signal_handlers->write_property = php_gobject_signal_write_property;
	php_gobject_signal_handlers->read_property = php_gobject_signal_read_property;
	php_gobject_signal_handlers->get_property_ptr_ptr = php_gobject_signal_get_property_ptr_ptr;
	php_gobject_signal_handlers->clone_obj = php_gobject_signal_clone;

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(gobject_signal)
{
	free(php_gobject_signal_handlers);

	return SUCCESS;
}


PHP_RINIT_FUNCTION(gobject_signal)
{
	zend_hash_init(&GOBJECT_G(signals_hash), 50, NULL, ZVAL_PTR_DTOR, 0);

	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(gobject_signal)
{
	zend_hash_graceful_destroy(&GOBJECT_G(signals_hash));

	return SUCCESS;
}
