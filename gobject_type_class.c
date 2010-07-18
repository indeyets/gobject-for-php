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

zend_class_entry *gobject_ce_type;
static zend_object_handlers *php_gobject_type_handlers;

#define PHP_GOBJ_INIT_ARRAYOBJ(var) { \
	MAKE_STD_ZVAL(var); \
	object_init_ex(var, spl_ce_ArrayObject); \
	zend_call_method_with_0_params(&var, spl_ce_ArrayObject, &spl_ce_ArrayObject->constructor, "__construct", NULL); \
}

void gobject_type_free_storage(gobject_type_object *intern TSRMLS_DC)
{
	if (intern->gtype) {
		// g_param_spec_unref(intern->gtype);
	}

	zval_ptr_dtor(&intern->properties);
	zval_ptr_dtor(&intern->signals);
	zval_ptr_dtor(&intern->interfaces);

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

zend_object_value gobject_type_object_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;
	gobject_type_object *object;

	object = emalloc(sizeof(gobject_type_object));
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

	object->gtype = 0;
	object->parent = 0;
	object->is_registered = 0;

	MAKE_STD_ZVAL(object->name);

	PHP_GOBJ_INIT_ARRAYOBJ(object->properties);
	PHP_GOBJ_INIT_ARRAYOBJ(object->signals);
	PHP_GOBJ_INIT_ARRAYOBJ(object->interfaces);

	retval.handle = zend_objects_store_put(
		object,
		(zend_objects_store_dtor_t)zend_objects_destroy_object,
		(zend_objects_free_object_storage_t) gobject_type_free_storage,
		NULL
		TSRMLS_CC
	);
	retval.handlers = php_gobject_type_handlers;

	return retval;
}

zval *php_gobject_type_read_property(zval *zobject, zval *prop, int type TSRMLS_DC)
{
	const char *propname = Z_STRVAL_P(prop);
	int proplen = Z_STRLEN_P(prop);

	gobject_type_object *object = (gobject_type_object *)zend_objects_get_address(zobject TSRMLS_CC);

	if (       proplen == 4  && strncmp(propname, "name", 4)        == 0) {
		Z_ADDREF_P(object->name);
		return object->name;
	} else if (proplen == 6  && strncmp(propname, "parent", 6)      == 0) {
		// TODO
		return NULL;
	} else if (proplen == 7  && strncmp(propname, "signals", 7)     == 0) {
		Z_ADDREF_P(object->signals);
		return object->signals;
	} else if (proplen == 10 && strncmp(propname, "properties", 10) == 0) {
		Z_ADDREF_P(object->properties);
		return object->properties;
	} else if (proplen == 10 && strncmp(propname, "interfaces", 10) == 0) {
		Z_ADDREF_P(object->interfaces);
		return object->interfaces;
	} else {
		zend_throw_exception_ex(spl_ce_OutOfBoundsException, 0 TSRMLS_CC, "No way to get this property");
		return NULL;
	}
}

void php_gobject_type_write_property(zval *zobject, zval *prop, zval *value TSRMLS_DC)
{
	const char *propname = Z_STRVAL_P(prop);
	int proplen = Z_STRLEN_P(prop);

	if (proplen == 4 && strncmp(propname, "name", 4) == 0) {
		
	} else if (proplen == 6 && strncmp(propname, "parent", 6) == 0) {
	} else {
		zend_throw_exception_ex(spl_ce_OutOfBoundsException, 0 TSRMLS_CC, "No way to set this property");
	}
}

zval **php_gobject_type_get_property_ptr_ptr(zval *object, zval *member TSRMLS_DC)
{
	// we don't want to provide direct access to underlying properties
	return NULL;
}


ZEND_BEGIN_ARG_INFO_EX(arginfo_Glib_GObject_Type__from, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, classname_or_instance)
ZEND_END_ARG_INFO()

PHP_METHOD(Glib_GObject_Type, from)
{
	zval *input = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &input) == FAILURE) {
		return;
	}

	char *classname = NULL;
	zend_uint classname_len = 0;

	if (Z_TYPE_P(input) == IS_STRING) {
		classname = Z_STRVAL_P(input);
		classname_len = Z_STRLEN_P(input);
	} else if (Z_TYPE_P(input) == IS_OBJECT) {
		int need_to_copy = zend_get_object_classname(input, &classname, &classname_len TSRMLS_CC);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "this method accepts only strings and objects");
		return;
	}

	php_printf("MAGIC\b");
	return;

	// TODO: try to get class from gobject-hierarchy
	zval *zobject;
	MAKE_STD_ZVAL(zobject);
	object_init_ex(zobject, gobject_ce_type);

	gobject_type_object *object = (gobject_type_object *)zend_objects_get_address(zobject TSRMLS_CC);
	object->is_registered = 1;

	zend_call_method_with_0_params(&zobject, gobject_ce_type, &gobject_ce_type->constructor, "__construct", NULL);

	RETURN_ZVAL(zobject, 0, 1);
}

// dummy constructor
PHP_METHOD(Glib_GObject_Type, __construct)
{
}

PHP_METHOD(Glib_GObject_Type, generate)
{
	php_printf("MAGIC\b");
	RETURN_FALSE;
	// TODO: try to register class in gobject-hierarchy
	// object->is_registered = 1;
}

const zend_function_entry gobject_type_methods[] = {
	// public
	PHP_ME(Glib_GObject_Type, from,        arginfo_Glib_GObject_Type__from, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Glib_GObject_Type, __construct, NULL,                            ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Glib_GObject_Type, generate,    NULL,                            ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

PHP_MINIT_FUNCTION(gobject_type)
{
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, GOBJECT_NAMESPACE, "Type", gobject_type_methods);
	gobject_ce_type = zend_register_internal_class(&ce TSRMLS_CC);
	gobject_ce_type->create_object  = gobject_type_object_new;

	// standard handlers + overriding
	php_gobject_type_handlers = malloc(sizeof(zend_object_handlers));
	memcpy(php_gobject_type_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_gobject_type_handlers->write_property = php_gobject_type_write_property;
	php_gobject_type_handlers->read_property = php_gobject_type_read_property;
	php_gobject_type_handlers->get_property_ptr_ptr = php_gobject_type_get_property_ptr_ptr;

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(gobject_type)
{
	free(php_gobject_type_handlers);

	return SUCCESS;
}
