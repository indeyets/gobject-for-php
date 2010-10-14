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

#include "php_gobject_private.h"

#define GOBJECT_PHP_FENTRY(zend_name, name, arg_info, flags) { zend_name, name, arg_info, (zend_uint) (sizeof(arg_info)/sizeof(struct _zend_arg_info)-1), flags }
#define GOBJECT_PHP_NAMED_FE(zend_name, name, arg_info)      GOBJECT_PHP_FENTRY(zend_name, name, arg_info, 0)

PHP_FUNCTION(gobject_universal_method)
{
	zend_function *active_function = EG(current_execute_data)->function_state.function;

	const char *method_name = active_function->common.function_name;
	const char *class_name = NULL;
	
	if (active_function->common.scope) {
		class_name = active_function->common.scope->name;
	}

	GIRepository *gir = GOBJECT_G(gir);

	GIObjectInfo *info = NULL;
	GIFunctionInfo *m_info = NULL;

	if (class_name) {
		php_printf("Hey! Someone called me! My name is: %s::%s()\n", class_name, method_name);

		GType gtype = g_type_from_phpname(class_name TSRMLS_CC);
		info = g_irepository_find_by_gtype(gir, gtype);
		m_info = g_object_info_find_method(info, method_name);
	} else {
		php_printf("Hey! Someone called me! My name is: %s()\n", method_name);

		char *name = NULL, *namespace = NULL;
		parse_namespaced_name(method_name, &namespace, &name);

		m_info = g_irepository_find_by_name(gir, namespace, name);

		efree(namespace);
		efree(name);
	}

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
	int php_argc = ZEND_NUM_ARGS();
	if (php_argc < required_args) {
		php_error(E_WARNING, "%s::%s() expects %s %d parameter%s, %d given",
					class_name, method_name,
					"at least", required_args,
					required_args == 1 ? "" : "s", php_argc);
		return;
	}

	GIFunctionInvoker invoker;

	{
		GError *err = NULL;
		g_function_info_prep_invoker(m_info, &invoker, &err);

		if (err) {
			php_error(E_WARNING, "Failed to prepare function for invocation: %s", err->message);
			g_error_free(err);
			return;
		}
	}

	// convert arguments (+ verify if types match)
	guint in_args_len = invoker.cif.nargs;
	gpointer *in_arg_pointers = g_newa(gpointer, in_args_len);
	// guint out_args_len = function->js_out_argc;
	// guint inout_args_len = function->inout_argc;

	zval ***php_args = safe_emalloc(php_argc, sizeof(zval **), 0);
	zend_get_parameters_array_ex(php_argc, php_args);

	{
		GIArgument *in_arg_cvalues = g_newa(GIArgument, in_args_len);

		for (gint i = 0; i < n_args; i++) {
			php_printf("i: %d of %d\n", i+1, n_args);

			in_arg_pointers[i] = &in_arg_cvalues[i];

			GIArgInfo arg_info;
			g_callable_info_load_arg(m_info, i, &arg_info);

			GIDirection direction = g_arg_info_get_direction(&arg_info);

			if (direction == GI_DIRECTION_OUT) {
				php_printf("-> OUTPUT parameter\n");
				// out_arg_cvalues[out_args_pos].v_pointer = NULL;
				// in_arg_cvalues[in_args_pos].v_pointer = &out_arg_cvalues[out_args_pos];
				// out_args_pos++;
			} else {
				php_printf("-> INPUT parameter\n");
				GIArgument *in_value = &in_arg_cvalues[i];

				// GITypeInfo ainfo;
				// GITypeTag type_tag = g_type_info_get_tag(&ainfo);

				zval *src = *(php_args[i]);
				if (!php_gobject_zval_to_giarg(src, &arg_info, in_value TSRMLS_CC)) {
					php_printf("<- ERR\n");
					efree(php_args);
					return;
				}
			}

			php_printf("<- OK\n");
		}
	}
	
	// call underlying gobject c-function
	GIArgument ffi_return_value;
	ffi_call(&(invoker.cif), invoker.native_address, &ffi_return_value, in_arg_pointers);

	// propagate return value
	GITypeInfo return_info;
	g_callable_info_load_return_type((GICallableInfo*) m_info, &return_info);

	php_gobject_giarg_to_zval(&return_info, &ffi_return_value, return_value TSRMLS_CC);

	// TODO: (+ out parameters)

	efree(php_args);

	g_base_info_unref(m_info);

	if (info) {
		g_base_info_unref(info);
	}
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
	char *phpclass = namespaced_name(g_base_info_get_namespace(info), g_base_info_get_name(info), FALSE);

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

	char *php_parent_name = namespaced_name(g_base_info_get_namespace(parent_info), g_base_info_get_name(parent_info), FALSE);
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
		if (phpclass_methods) {
			efree(phpclass_methods);
		}
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
			case GI_INFO_TYPE_INTERFACE:
				php_printf("-> interface %s\n", g_base_info_get_name(b_info));
			break;

			case GI_INFO_TYPE_OBJECT:
				gobject_girepository_load_class(b_info TSRMLS_CC);
			break;

			case GI_INFO_TYPE_TYPE:
				php_printf("-> type %s\n", g_base_info_get_name(b_info));
			break;

			case GI_INFO_TYPE_STRUCT:
				php_printf("-> struct %s\n", g_base_info_get_name(b_info));
			break;

			case GI_INFO_TYPE_FUNCTION:
				php_printf("-> function %s\n", g_base_info_get_name(b_info));
				{
					char *phpname = namespaced_name(g_base_info_get_namespace(b_info), g_base_info_get_name(b_info), FALSE);

					zend_function_entry functions[] = {
						GOBJECT_PHP_NAMED_FE(phpname, PHP_FN(gobject_universal_method), NULL),
						{NULL, NULL, NULL}
					};

					zend_register_functions(NULL, functions, EG(function_table), MODULE_TEMPORARY TSRMLS_CC);
					zend_hash_next_index_insert(&GOBJECT_G(runtime_functions), &phpname, sizeof(char *), NULL);
				}
			break;

			case GI_INFO_TYPE_ENUM:
				php_printf("-> enumeration %s\n", g_base_info_get_name(b_info));
			break;

			case GI_INFO_TYPE_BOXED:
				php_printf("-> boxed %s\n", g_base_info_get_name(b_info));
			break;

			case GI_INFO_TYPE_FLAGS:
				php_printf("-> flags %s\n", g_base_info_get_name(b_info));
			break;

			case GI_INFO_TYPE_CONSTANT:
				php_printf("-> constant %s\n", g_base_info_get_name(b_info));
			break;

			default:
				php_printf("-> ??? %s\n", g_base_info_get_name(b_info));
			break;
		}
		g_base_info_unref(b_info);
	}
	php_printf("\n");
	// TODO

	RETURN_TRUE;
}


void php_gobject_gir_function_dtor(char **func_name_ptr)
{
	TSRMLS_FETCH();
	char *func_name = *func_name_ptr;
	size_t func_name_len = strlen(func_name);

	char *lowercase_name = zend_str_tolower_dup(func_name, func_name_len);

	if (zend_hash_del(EG(function_table), lowercase_name, func_name_len+1) == FAILURE) {
		php_error(E_WARNING, "Couldn't delete temporary function \"%s\"", func_name);
	}

	efree(func_name);
	efree(lowercase_name);
}

PHP_RINIT_FUNCTION(gobject_girepository)
{
	zend_hash_init(&GOBJECT_G(runtime_functions), 100, NULL, (void (*)(void *))php_gobject_gir_function_dtor, 0);

	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(gobject_girepository)
{
	zend_hash_graceful_destroy(&GOBJECT_G(runtime_functions));

	return SUCCESS;
}
