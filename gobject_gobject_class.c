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

zend_class_entry *gobject_ce_gobject;
zend_object_handlers *php_gobject_gobject_handlers;

void gobject_gobject_free_storage(gobject_gobject_object *intern TSRMLS_DC)
{
	if (intern->gobject) {
		g_object_unref(intern->gobject);
	}

	if (intern->std.guards) {
		zend_hash_destroy(intern->std.guards);
		FREE_HASHTABLE(intern->std.guards);
	}

	if (intern->std.properties) {
		zend_hash_destroy(intern->std.properties);
		FREE_HASHTABLE(intern->std.properties);
	}

	efree(intern);
}

zend_object_value gobject_gobject_object_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;
	gobject_gobject_object *object;

	object = emalloc(sizeof(gobject_gobject_object));
	object->std.ce = ce;
	object->std.guards = NULL;

	ALLOC_HASHTABLE(object->std.properties);
	zend_hash_init(object->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);

	zval *tmp;
	zend_hash_copy(
		object->std.properties,
		&ce->default_properties,
		(copy_ctor_func_t) zval_add_ref,
		(void *) &tmp,
		sizeof(zval *)
	);

	object->gobject = g_object_new(G_TYPE_OBJECT, NULL);

	retval.handle = zend_objects_store_put(
		object,
		(zend_objects_store_dtor_t)zend_objects_destroy_object,
		(zend_objects_free_object_storage_t) gobject_gobject_free_storage,
		NULL
		TSRMLS_CC
	);
	retval.handlers = php_gobject_gobject_handlers;

	return retval;
}

zval *php_gobject_gobject_read_property(zval *zobject, zval *prop, int type TSRMLS_DC)
{
	return NULL;
}

void php_gobject_gobject_write_property(zval *zobject, zval *prop, zval *value TSRMLS_DC)
{
}

zval **php_gobject_gobject_get_property_ptr_ptr(zval *object, zval *member TSRMLS_DC)
{
	// we don't want to provide direct access to underlying properties
	return NULL;
}


PHP_METHOD(Glib_GObject_GObject, __construct)
{
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_Glib_GObject_GObject__connect, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_INFO(0, signal_name)
	ZEND_ARG_INFO(0, callback)
	ZEND_ARG_INFO(0, extra_param1)
	ZEND_ARG_INFO(0, ...)
ZEND_END_ARG_INFO()

PHP_METHOD(Glib_GObject_GObject, connect)
{
	int signal_name_len = 0;
	char *signal_name = NULL;
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;
	int params_count = 0;
	zval ***params = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sf*", &signal_name, &signal_name_len, &fci, &fci_cache, &params, &params_count) == FAILURE) {
		return;
	}

	zval *zval_object = getThis();
	gobject_gobject_object *z_obj = __php_objstore_object(zval_object);
	GObject *gobject = z_obj->gobject;
	GType gtype = G_OBJECT_TYPE(gobject);

	guint signal_id;
	GQuark signal_detail;

	if (!g_signal_parse_name(signal_name, gtype, &signal_id, &signal_detail, TRUE)) {
		php_error(E_WARNING, "%s signal name is invalid", signal_name);
		return;
		/* TODO , should we throw exception here? */
	}

	GClosure *closure = php_gobject_closure_new(fci, fci_cache, zval_object TSRMLS_CC);

	if (!closure) {
		php_error(E_WARNING, "Couldn't create new closure");
		return;
	}

	g_signal_connect_closure_by_id(gobject, signal_id, signal_detail, closure, FALSE);

	if (params_count) {
		php_printf("Need to add %d parameters\n", params_count);
		efree(params);
	}
}


const zend_function_entry gobject_gobject_methods[] = {
	// public
	PHP_ME(Glib_GObject_GObject, __construct, NULL,                                  ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Glib_GObject_GObject, connect,     arginfo_Glib_GObject_GObject__connect, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

PHP_MINIT_FUNCTION(gobject_gobject)
{
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, GOBJECT_NAMESPACE, "GObject", gobject_gobject_methods);
	gobject_ce_gobject = zend_register_internal_class(&ce TSRMLS_CC);
	gobject_ce_gobject->create_object  = gobject_gobject_object_new;

	// standard handlers + overriding
	php_gobject_gobject_handlers = malloc(sizeof(zend_object_handlers));
	memcpy(php_gobject_gobject_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_gobject_gobject_handlers->write_property = php_gobject_gobject_write_property;
	php_gobject_gobject_handlers->read_property = php_gobject_gobject_read_property;
	php_gobject_gobject_handlers->get_property_ptr_ptr = php_gobject_gobject_get_property_ptr_ptr;

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(gobject_gobject)
{
	// free(php_gobject_gobject_handlers);

	return SUCCESS;
}
