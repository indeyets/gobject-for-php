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


zend_bool php_gobject_store_class_closure(GClosure *class_closure TSRMLS_DC)
{
	HashTable *ht = &GOBJECT_G(class_closure_hash);

	if (SUCCESS == zend_hash_next_index_insert(ht, (void*)&class_closure, sizeof(GClosure *), NULL))
		return TRUE;

	return FALSE;
}

void php_gobject_invalidate_class_closure(GClosure **class_closure)
{
	g_closure_invalidate(*class_closure);
}


void gobject_type_free_storage(gobject_type_object *intern TSRMLS_DC)
{
	if (intern->gtype) {
		// g_param_spec_unref(intern->gtype);
	}

	zval_ptr_dtor(&intern->name);

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
	zend_hash_init(object->std.properties, zend_hash_num_elements(&ce->default_properties), NULL, ZVAL_PTR_DTOR, 0);

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
		return object->name;
	} else if (proplen == 6  && strncmp(propname, "parent", 6)      == 0) {
		zval *retval = NULL;
		MAKE_STD_ZVAL(retval);
		ZVAL_STRING(retval, g_type_name(object->parent), 1);

		Z_DELREF_P(retval); // setting refcount to 0. receiving side is responsible for adding ref

		return retval;
	} else if (proplen == 7  && strncmp(propname, "signals", 7)     == 0) {
		return object->signals;
	} else if (proplen == 10 && strncmp(propname, "properties", 10) == 0) {
		return object->properties;
	} else if (proplen == 10 && strncmp(propname, "interfaces", 10) == 0) {
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

	gobject_type_object *object = (gobject_type_object *)zend_objects_get_address(zobject TSRMLS_CC);

	if (proplen == 4 && strncmp(propname, "name", 4) == 0) {
		zval_ptr_dtor(&object->name); // free old one
		Z_ADDREF_P(value);
		object->name = value;
	} else if (proplen == 6 && strncmp(propname, "parent", 6) == 0) {
		convert_to_string(value);
		const gchar *parent_name = Z_STRVAL_P(value);
		GType parent_gtype = g_type_from_phpname(parent_name);

		if (0 == parent_gtype) {
			zend_throw_exception_ex(spl_ce_OutOfBoundsException, 0 TSRMLS_CC, "Unknown type name: %s", parent_name);
			return;
		}

		object->parent = parent_gtype;
	} else {
		zend_throw_exception_ex(spl_ce_OutOfBoundsException, 0 TSRMLS_CC, "No way to set this property");
		return;
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

static int glib_gobject_type_register_signal(zend_object_iterator *iter, gobject_type_object **puser TSRMLS_DC)
{
	zval **signal_p = NULL;
	zend_user_it_get_current_data(iter, &signal_p TSRMLS_CC);

	char *key;
	uint key_len;
	zend_user_it_get_current_key(iter, &key, &key_len, NULL TSRMLS_CC);

	gobject_signal_object *gsignal = (gobject_signal_object *)zend_objects_get_address(*signal_p TSRMLS_CC);

	if (gsignal->signal_id > 0) {
		php_error(E_ERROR, "This signal associated with \"%s\" name is already registered. Clone object before reuse", key);
		efree(key);

		return ZEND_HASH_APPLY_STOP;
	}

	gobject_type_object *object = *puser;

	GType *param_types = NULL;
	guint param_count = 0;

	if (gsignal->param_types) {
		HashTable *hash = HASH_OF(gsignal->param_types);
		param_count = zend_hash_num_elements(hash);

		param_types = ecalloc(param_count, sizeof(GType));

		ulong i;
		for (i = 0; i < param_count; i++) {
			void *pdata = NULL;
			if (zend_hash_index_find(hash, i, &pdata) == FAILURE) {
				php_error(E_WARNING, "Couldn't fetch parameter #%ld of signal", i);
				continue;
			}

			// zval *param = *((zval **) pdata);
			zval **param_p = (zval **) pdata;
			param_types[i] = g_type_from_phpname(Z_STRVAL_PP(param_p));
		}
	}

	GClosure *class_closure = NULL;
	if (!callback_is_empty(&gsignal->class_closure_fci)) {
		class_closure = php_gobject_closure_new_class(gsignal->class_closure_fci, gsignal->class_closure_fci_cache TSRMLS_CC);
		php_gobject_store_class_closure(class_closure TSRMLS_CC);
	}

	GSignalAccumulator accu = NULL;

	if (!callback_is_empty(&gsignal->accumulator_fci)) {
		accu = php_gobject_closure_accumulator;
	}

	gsignal->signal_id = g_signal_newv(
		key, object->gtype, gsignal->flags,				// name, class, flags
		class_closure,									// default closure
		accu, NULL,										// accu + data
		php_gobject_closure_marshal,					// marshaller
		gsignal->return_type, param_count, param_types	// return + parameters
	);

	php_gobject_store_signal_association(*signal_p TSRMLS_CC);

	if (param_types) {
		efree(param_types);
	}

	efree(key);

	return ZEND_HASH_APPLY_KEEP;
}

static int glib_gobject_type_register_property(zend_object_iterator *iter, GObjectClass **type_class_ptr TSRMLS_DC)
{
	zval **pspec_p = NULL;
	zend_user_it_get_current_data(iter, &pspec_p TSRMLS_CC);

	char *key = NULL;
	uint key_len;
	ulong int_key;
	zend_user_it_get_current_key(iter, &key, &key_len, &int_key TSRMLS_CC);

	GObjectClass *type_class = *type_class_ptr;

	gobject_paramspec_object *gpspec = (gobject_paramspec_object *)zend_objects_get_address(*pspec_p TSRMLS_CC);
	GParamSpec *pspec = gpspec->paramspec;

	g_object_class_install_property(type_class, int_key + 1, pspec);

	if (key) {
		efree(key);
	}

	return ZEND_HASH_APPLY_KEEP;
}

PHP_METHOD(Glib_GObject_Type, generate)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	gobject_type_object *object = (gobject_type_object *)zend_objects_get_address(getThis() TSRMLS_CC);
	const char *class_name = Z_STRVAL_P(object->name);
	GType new_gtype = g_type_from_name(class_name);

	if (0 != new_gtype) {
		zend_throw_exception_ex(spl_ce_OutOfBoundsException, 0 TSRMLS_CC, "This class is already registered: %s", class_name);
		return;
	}

	// 1. register gobject class
	new_gtype = g_type_register_static_simple(
		object->parent, class_name,
		sizeof(GObjectClass), NULL,
		sizeof(PhpGObject), (GInstanceInitFunc)php_gobject_gobject_init,
		0
	);
	object->gtype = new_gtype;

	if (0 == new_gtype) {
		zend_throw_exception_ex(spl_ce_OutOfBoundsException, 0 TSRMLS_CC, "Failed to initialise type: %s", class_name);
		return;
	}

	// 2. register php class
	{
		const char *parent_name = g_type_name(object->parent);
		char *full_parent_name = NULL;
		asprintf(&full_parent_name, "%s\\%s", GOBJECT_NAMESPACE, parent_name);

		zend_class_entry *parent_ce = zend_fetch_class(full_parent_name, strlen(full_parent_name), ZEND_FETCH_CLASS_NO_AUTOLOAD TSRMLS_CC);
		free(full_parent_name);

		zend_class_entry ce;
		INIT_CLASS_ENTRY_EX(ce, class_name, Z_STRLEN_P(object->name), NULL);

		zend_class_entry *target = zend_register_internal_class_ex(&ce, parent_ce, NULL TSRMLS_CC);
		target->create_object  = gobject_gobject_object_new;
	}

	// 3. register signals
	{
		spl_iterator_apply(object->signals, (spl_iterator_apply_func_t)glib_gobject_type_register_signal, (void *)&object TSRMLS_CC);
	}

	// 4. register properties
	{
		GObjectClass *type_class = g_type_class_ref(new_gtype);
		type_class->get_property = (GObjectGetPropertyFunc)php_gobject_gobject_get_glib_property;
		type_class->set_property = (GObjectSetPropertyFunc)php_gobject_gobject_set_glib_property;
		type_class->finalize     = (GObjectFinalizeFunc)php_gobject_gobject_finalize;

		spl_iterator_apply(object->properties, (spl_iterator_apply_func_t)glib_gobject_type_register_property, (void *)&type_class TSRMLS_CC);
		g_type_class_unref(type_class); // not needed anymore
	}

	// object->is_registered = 1;
	RETURN_TRUE;
}


PHP_RINIT_FUNCTION(gobject_type)
{
	zend_hash_init(&GOBJECT_G(class_closure_hash), 50, NULL, (void (*)(void *))php_gobject_invalidate_class_closure, 0);

	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(gobject_type)
{
	zend_hash_graceful_destroy(&GOBJECT_G(class_closure_hash));

	return SUCCESS;
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
