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

#include <ext/spl/spl_exceptions.h>

#include "php_gobject.h"

zend_class_entry *gobject_ce_gobject;
zend_object_handlers *php_gobject_gobject_handlers;

void unregister_gobject_closure(gpointer _zobject, GClosure *closure)
{
    gobject_gobject_object *zobject = (gobject_gobject_object *) _zobject;
    zobject->closures = g_slist_remove(zobject->closures, closure);
}

void register_gobject_closure(zval *zval_object, GClosure *closure TSRMLS_DC)
{
	gobject_gobject_object *zobject = __php_objstore_object(zval_object);

	if (g_slist_find(zobject->closures, closure) != NULL) {
		// registered already
		return;
	}

	zobject->closures = g_slist_prepend(zobject->closures, closure);
	g_closure_add_invalidate_notifier(closure, zobject, unregister_gobject_closure);
}



void gobject_gobject_free_storage(gobject_gobject_object *intern TSRMLS_DC)
{
	if (intern->gobject) {
		g_object_set_data(intern->gobject, "gobject-for-php", NULL);
		g_object_unref(intern->gobject);
	}

	GSList *tmp = intern->closures;
	while (tmp) {
		GClosure *closure = (GClosure *) tmp->data;
		// Save the pointer, because phpg_gobject_unwatch_closure() will remove
		// the closure from the list.
		tmp = tmp->next;

		if (closure) {
			g_closure_invalidate(closure);
		}
	}
	intern->closures = NULL;
	g_slist_free(intern->closures);

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
	zend_hash_init(object->std.properties, zend_hash_num_elements(&ce->default_properties), NULL, ZVAL_PTR_DTOR, 0);

	zval *tmp;
	zend_hash_copy(
		object->std.properties,
		&ce->default_properties,
		(copy_ctor_func_t) zval_add_ref,
		(void *) &tmp,
		sizeof(zval *)
	);

	object->closures = g_slist_alloc();
	object->gobject = NULL; // this is later assigned either in constructor or from external source

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
	zval *self = getThis();
	gobject_gobject_object *object = (gobject_gobject_object *)zend_object_store_get_object(self TSRMLS_CC);
	zend_class_entry *ce = object->std.ce;

	{
		GType new_gtype = g_type_from_phpname(ce->name);

		if (new_gtype == 0) {
			zend_throw_exception_ex(spl_ce_OutOfBoundsException, 0 TSRMLS_CC, "This class doesn't have known GObject counterpart: %s", ce->name);
			return;
		}

		object->gobject = g_object_new(new_gtype, NULL);
		g_object_set_data(object->gobject, "gobject-for-php", self);
	}
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
	GObject *gobject = __php_gobject_ptr(zval_object);
	GType gtype = G_OBJECT_TYPE(gobject);

	guint signal_id;
	GQuark signal_detail;

	if (!g_signal_parse_name(signal_name, gtype, &signal_id, &signal_detail, TRUE)) {
		php_error(E_WARNING, "%s signal name is invalid", signal_name);

		if (params) {
			efree(params);
		}

		return;
		/* TODO , should we throw exception here? */
	}

	GClosure *closure = php_gobject_closure_new(gobject, fci, fci_cache, params, params_count TSRMLS_CC);

	if (params) {
		efree(params);
	}

	if (!closure) {
		php_error(E_WARNING, "Couldn't create new closure");
		return;
	}

	gulong handler_id = g_signal_connect_closure_by_id(gobject, signal_id, signal_detail, closure, FALSE);
	register_gobject_closure(zval_object, closure TSRMLS_CC);

	RETURN_LONG((long)handler_id);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_Glib_GObject_GObject__disconnect, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, handler_id)
ZEND_END_ARG_INFO()

PHP_METHOD(Glib_GObject_GObject, disconnect)
{
	long handler_id = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &handler_id) == FAILURE) {
		return;
	}

	GObject *gobject = __php_gobject_ptr(getThis());
	g_signal_handler_disconnect(gobject, (gulong)handler_id);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_Glib_GObject_GObject__notify, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, property_name)
ZEND_END_ARG_INFO()

PHP_METHOD(Glib_GObject_GObject, notify)
{
	int property_name_len;
	char *property_name;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &property_name, &property_name_len) == FAILURE) {
		return;
	}

	GObject *gobject = __php_gobject_ptr(getThis());
	g_object_notify(gobject, property_name);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_Glib_GObject_GObject__emit, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, signal_name)
	ZEND_ARG_INFO(0, param1)
	ZEND_ARG_INFO(0, ...)
ZEND_END_ARG_INFO()

PHP_METHOD(Glib_GObject_GObject, emit)
{
	int signal_name_len = 0;
	char *signal_name = NULL;
	int params_len = 0;
	zval ***params = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s*", &signal_name, &signal_name_len, &params, &params_len) == FAILURE) {
		return;
	}

	GObject *gobject = __php_gobject_ptr(getThis());

	guint signal_id = g_signal_lookup(signal_name, G_OBJECT_TYPE(gobject));

	if (signal_id == 0) {
		php_error(E_WARNING, "Signal \"%s\" does not exist", signal_name);
		return;
	}

	GValue *signal_params = ecalloc(1 + params_len, sizeof(GValue));

	// signal_params[0] is reserved for instance
	g_value_init(signal_params + 0, G_TYPE_OBJECT);
	g_value_set_instance(signal_params + 0, G_OBJECT(gobject));

	int i;
	for (i = 0; i < params_len; i++) {
		zval **param = params[i];

		if (zval_to_gvalue(*param, (signal_params + i + 1)) == FALSE) {
			efree(signal_params);
			php_error(E_WARNING, "conversion of param %d failed", i+1);
			return;
		}
	}

	GValue *retval = g_new0(GValue, 1);
	// GValue retval = {0,};
	g_value_init(retval, G_TYPE_STRING);

	// php_printf("g_signal_emitv(%p, %d, 0, %p)\n", signal_params, signal_id, retval);
	g_signal_emitv(signal_params, signal_id, 0, retval);
	ZVAL_STRING(return_value, g_value_get_string(retval), 1);
	g_free(retval);

	efree(signal_params);
	if (params_len) {
		efree(params);
	}
}

const zend_function_entry gobject_gobject_methods[] = {
	// public
	PHP_ME(Glib_GObject_GObject, __construct, NULL,                                     ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Glib_GObject_GObject, connect,     arginfo_Glib_GObject_GObject__connect,    ZEND_ACC_PUBLIC)
	PHP_ME(Glib_GObject_GObject, notify,      arginfo_Glib_GObject_GObject__notify,     ZEND_ACC_PUBLIC)
	PHP_ME(Glib_GObject_GObject, emit,        arginfo_Glib_GObject_GObject__emit,       ZEND_ACC_PUBLIC)
	PHP_ME(Glib_GObject_GObject, disconnect,  arginfo_Glib_GObject_GObject__disconnect, ZEND_ACC_PUBLIC)
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
	free(php_gobject_gobject_handlers);

	return SUCCESS;
}
