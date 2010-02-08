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

zend_object_value gobject_type_object_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;
	gobject_type_object *object;
	zval *tmp;

	object = emalloc(sizeof(gobject_type_object));
	object->std.ce = ce;
	object->std.guards = NULL;

	ALLOC_HASHTABLE(object->std.properties);
	zend_hash_init(object->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
	zend_hash_copy(object->std.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

	object->gtype = 0;
	object->parent = 0;

	MAKE_STD_ZVAL(object->name);

	PHP_GOBJ_INIT_ARRAYOBJ(object->properties);
	PHP_GOBJ_INIT_ARRAYOBJ(object->signals);
	PHP_GOBJ_INIT_ARRAYOBJ(object->interfaces);

	{
	}

	retval.handle = zend_objects_store_put(object, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t) gobject_type_free_storage, NULL TSRMLS_CC);
	retval.handlers = php_gobject_type_handlers;

	return retval;
}

// dummy constructor
PHP_METHOD(Glib_GObject_Type, __construct)
{
}


const zend_function_entry gobject_type_methods[] = {
	// public
	// PHP_ME(Glib_GObject_Type, string,      NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	// private
	PHP_ME(Glib_GObject_Type, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
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

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(gobject_type)
{
	free(php_gobject_type_handlers);

	return SUCCESS;
}
