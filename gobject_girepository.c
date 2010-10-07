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

#define GOBJECT_PHP_FENTRY(zend_name, name, arg_info, flags) { zend_name, name, arg_info, (zend_uint) (sizeof(arg_info)/sizeof(struct _zend_arg_info)-1), flags }
#define GOBJECT_PHP_NAMED_FE(zend_name, name, arg_info)      GOBJECT_PHP_FENTRY(zend_name, name, arg_info, 0)

PHP_FUNCTION(gobject_universal_method)
{
	zend_function *active_function = EG(current_execute_data)->function_state.function;

	const char *method_name = active_function->common.function_name;
	const char *class_name = active_function->common.scope->name;

	php_printf("Hey! Someone called me! My name is: %s::%s\n", class_name, method_name);

	GType gtype = g_type_from_phpname(class_name TSRMLS_CC);
	GIRepository *gir = GOBJECT_G(gir);

	GIObjectInfo *info = g_irepository_find_by_gtype(gir, gtype);
	GIFunctionInfo *m_info = g_object_info_find_method(info, method_name);

	GIFunctionInfoFlags flags = g_function_info_get_flags(m_info);
	gboolean can_throw_gerror = (flags & GI_FUNCTION_THROWS) != 0;
	gint n_args = g_callable_info_get_n_args((GICallableInfo*) m_info);

	gint required_args = 0; // TODO: cache this value somewhere
	for (gint i = 0; i < n_args; i++) {
		GIArgInfo *a_info = g_callable_info_get_arg(m_info, i);
		if (!g_arg_info_is_optional(a_info)) {
			required_args++;
		}
		g_base_info_unref(a_info);
	}

	// verify number of arguments
	int args = ZEND_NUM_ARGS();
	if (args < required_args) {
		zend_error(E_WARNING, "%s::%s() expects %s %d parameter%s, %d given",
					class_name, method_name,
					"at least", required_args,
					required_args == 1 ? "" : "s", args);
		return;
	}

	// convert arguments (+ verify if types match)
	//  TODO

	// call underlying gobject c-function
	// ffi_call(…)

	// propagate return value (+ out parameters)

	g_base_info_unref(m_info);
	g_base_info_unref(info);
}

static zend_function_entry* gobject_girepository_get_methods(GIObjectInfo *info TSRMLS_DC)
{
	gint n = g_object_info_get_n_methods(info);

	if (n == 0)
		return NULL;

	zend_function_entry *functions = ecalloc(n + 1, sizeof(zend_function_entry));
	size_t pos = 0;

	for (gint i = 0; i < n; i++) {
		GIFunctionInfo *m_info = g_object_info_get_method(info, i);
		GIFunctionInfoFlags flags = g_function_info_get_flags(m_info);

		if (flags & GI_FUNCTION_IS_METHOD) {
			const gchar *name = g_base_info_get_name(m_info);

			zend_function_entry fe = GOBJECT_PHP_NAMED_FE(name, PHP_FN(gobject_universal_method), NULL);
			functions[pos++] = fe;
		}

		g_base_info_unref(m_info);
	}

	zend_function_entry empty_fe = {NULL, NULL, NULL};
	functions[pos] = empty_fe;

	return functions;
}

void static gobject_girepository_load_class(GIObjectInfo *info TSRMLS_DC)
{
	char *phpclass = namespaced_name(g_base_info_get_namespace(info), g_base_info_get_name(info));

	// Is it loaded?
	zend_class_entry *class_ce = zend_fetch_class(phpclass, strlen(phpclass), ZEND_FETCH_CLASS_NO_AUTOLOAD TSRMLS_CC);

	if (class_ce) {
		// already registered
		efree(phpclass);
		return;
	}

	// Load parent
	GIObjectInfo *parent_info = g_object_info_get_parent(info);
	gobject_girepository_load_class(parent_info TSRMLS_CC);

	char *php_parent_name = namespaced_name(g_base_info_get_namespace(parent_info), g_base_info_get_name(parent_info));
	g_base_info_unref(parent_info);

	{
		zend_class_entry *parent_ce = zend_fetch_class(php_parent_name, strlen(php_parent_name), ZEND_FETCH_CLASS_NO_AUTOLOAD TSRMLS_CC);

		if (NULL == parent_ce) {
			zend_throw_exception_ex(spl_ce_OutOfBoundsException, 0 TSRMLS_CC, "Parent PHP class (%s) is not registered", php_parent_name);
			efree(php_parent_name);
			return;
		}
		efree(php_parent_name);

		// Init PHP class
		zend_class_entry ce;

		zend_function_entry *phpclass_methods = gobject_girepository_get_methods(info TSRMLS_CC);
		INIT_CLASS_ENTRY_EX(ce, phpclass, strlen(phpclass), phpclass_methods);

		zend_class_entry *target = zend_register_internal_class_ex(&ce, parent_ce, NULL TSRMLS_CC);
		target->create_object  = gobject_gobject_object_new;

		efree(phpclass);
		efree(phpclass_methods);
	}
}


PHP_FUNCTION(GIRepository_load_ns)
{
	char *ns_name = NULL, *version = NULL;
	int ns_name_len = 0, version_len = 0;
	long flags = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|sl", &ns_name, &ns_name_len, &version, &version_len, &flags) == FAILURE) {
		return;
	}

	GIRepository *gir = GOBJECT_G(gir);

	// 1. Check if loaded
	if (g_irepository_is_registered(gir, ns_name, version))
		RETURN_TRUE;

	// 2. Load
	GError *err = NULL;
	GITypelib *type_lib = g_irepository_require(gir, ns_name, version, 0, &err);

	if (err) {
		zend_throw_exception_ex(spl_ce_OutOfBoundsException, 0 TSRMLS_CC, "Failed to load \"%s\" namespace: %s", ns_name, err->message);
		g_error_free(err);
		return;
	}

	// 3. store type_lib in array of namespaces, which should be unloaded
	// TODO

	// 4. Init functions, classes, …
	gint n = g_irepository_get_n_infos(gir, ns_name);
	for (gint i = 0; i < n; i++) {
		GIBaseInfo *b_info = g_irepository_get_info(gir, ns_name, i);
		GIInfoType info_type = g_base_info_get_type(b_info);

		switch (info_type) {
			case GI_INFO_TYPE_OBJECT:
				gobject_girepository_load_class(b_info TSRMLS_CC);
			break;

			case GI_INFO_TYPE_TYPE:
				php_printf("-> type %s\n", g_base_info_get_name(b_info));
			break;

			case GI_INFO_TYPE_STRUCT:
				php_printf("-> struct %s\n", g_base_info_get_name(b_info));
			break;

			case GI_INFO_TYPE_ENUM:
				php_printf("-> enumeration %s\n", g_base_info_get_name(b_info));
			break;

			default:
				php_printf("-> ??? %s\n", g_base_info_get_name(b_info));
			break;
		}
		g_base_info_unref(b_info);
	}
	php_printf("\n");
	// TODO

	php_printf("Hello %s\n", ns_name);
	RETURN_TRUE;
}


PHP_RINIT_FUNCTION(gobject_girepository)
{
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(gobject_girepository)
{
	return SUCCESS;
}
