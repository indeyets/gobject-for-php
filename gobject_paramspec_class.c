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

zend_class_entry *gobject_ce_paramspec;

void gobject_paramspec_free_storage(gobject_paramspec_object *intern TSRMLS_DC)
{
	if (intern->paramspec) {
		g_param_spec_unref(intern->paramspec);
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

zend_object_value gobject_paramspec_object_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;
	gobject_paramspec_object *object;
	zval *tmp;

	object = emalloc(sizeof(gobject_paramspec_object));
	object->std.ce = ce;
	object->std.guards = NULL;
	object->paramspec = NULL;

	ALLOC_HASHTABLE(object->std.properties);
	zend_hash_init(object->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
	zend_hash_copy(
		object->std.properties,
		&ce->default_properties,
		(copy_ctor_func_t) zval_add_ref,
		(void *) &tmp,
		sizeof(zval *)
	);

	retval.handle = zend_objects_store_put(
		object,
		(zend_objects_store_dtor_t)zend_objects_destroy_object,
		(zend_objects_free_object_storage_t) gobject_paramspec_free_storage,
		NULL
		TSRMLS_CC
	);
	retval.handlers = zend_get_std_object_handlers();
	return retval;
}

// dummy constructor
PHP_METHOD(Glib_GObject_ParamSpec, __construct)
{
}

PHP_METHOD(Glib_GObject_ParamSpec, string)
{
	char *name = NULL, *nick = NULL, *blurb = NULL, *default_value = NULL;
	int name_len, nick_len, blurb_len, default_value_len;
	long flags = 0;

	// public static function string($name, $flags = 0, $default_value = '', $nickname = '', $description = '')
	if (zend_parse_parameters(
			ZEND_NUM_ARGS() TSRMLS_CC, "s|lsss",
			&name, &name_len,
			&flags,
			&default_value, &default_value_len,
			&nick, &nick_len,
			&blurb, &blurb_len
		) == FAILURE
	) {
		return;
	}

	object_init_ex(return_value, gobject_ce_paramspec);
	gobject_paramspec_object *paramspec_object =
		(gobject_paramspec_object *)zend_objects_get_address(return_value TSRMLS_CC);

	paramspec_object->paramspec = g_param_spec_string(name, nick, blurb, default_value, flags);
	g_param_spec_ref_sink(paramspec_object->paramspec);

	zend_call_method_with_0_params(
		&return_value,
		gobject_ce_paramspec,
		&gobject_ce_paramspec->constructor, "__construct",
		NULL
	);
}


const zend_function_entry gobject_paramspec_methods[] = {
	// public
	PHP_ME(Glib_GObject_ParamSpec, string,      NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	// private
	PHP_ME(Glib_GObject_ParamSpec, __construct, NULL, ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	{NULL, NULL, NULL}
};

PHP_MINIT_FUNCTION(gobject_paramspec)
{
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, GOBJECT_NAMESPACE, "ParamSpec", gobject_paramspec_methods);
	gobject_ce_paramspec = zend_register_internal_class(&ce TSRMLS_CC);
	gobject_ce_paramspec->create_object  = gobject_paramspec_object_new;

#define PARAMSPEC_CONSTANT(k, v) \
	zend_declare_class_constant_long(gobject_ce_paramspec, k, sizeof(k)-1, v TSRMLS_CC)

	PARAMSPEC_CONSTANT("READABLE",       G_PARAM_READABLE);
	PARAMSPEC_CONSTANT("WRITABLE",       G_PARAM_WRITABLE);
	PARAMSPEC_CONSTANT("READWRITE",      G_PARAM_READWRITE);

	PARAMSPEC_CONSTANT("CONSTRUCT",      G_PARAM_CONSTRUCT);
	PARAMSPEC_CONSTANT("CONSTRUCT_ONLY", G_PARAM_CONSTRUCT_ONLY);
	PARAMSPEC_CONSTANT("LAX_VALIDATION", G_PARAM_LAX_VALIDATION);

	PARAMSPEC_CONSTANT("STATIC_NAME",    G_PARAM_STATIC_NAME);
	PARAMSPEC_CONSTANT("STATIC_NICK",    G_PARAM_STATIC_NICK);
	PARAMSPEC_CONSTANT("STATIC_BLURB",   G_PARAM_STATIC_BLURB);
	PARAMSPEC_CONSTANT("STATIC_STRINGS", G_PARAM_STATIC_STRINGS);

	PARAMSPEC_CONSTANT("MASK",           G_PARAM_MASK);
	PARAMSPEC_CONSTANT("USER_SHIFT",     G_PARAM_USER_SHIFT);
#undef PARAMSPEC_CONSTANT

	return SUCCESS;
}
