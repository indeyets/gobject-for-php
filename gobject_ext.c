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
#include <php_ini.h>
#include <ext/standard/info.h>

#include "php_gobject_private.h"

ZEND_DECLARE_MODULE_GLOBALS(gobject)

/* True global resources - no need for thread safety here */
// static int le_gobject;

/* {{{ gobject_functions[]
 *
 * Every user visible function must have an entry in gobject_functions[].
 */
const zend_function_entry gobject_functions[] = {
	PHP_NAMED_FE(girepository\\load_ns, PHP_FN(GIRepository_load_ns), NULL)
	{NULL, NULL, NULL}	/* Must be the last line in gobject_functions[] */
};
/* }}} */

/* {{{ gobject_module_entry
 */
zend_module_entry gobject_module_entry = {
	STANDARD_MODULE_HEADER,
	"gobject",
	gobject_functions,
	PHP_MINIT(gobject),
	PHP_MSHUTDOWN(gobject),
	PHP_RINIT(gobject),
	PHP_RSHUTDOWN(gobject),
	PHP_MINFO(gobject),
	PHP_GOBJECT_VERSION,
	PHP_MODULE_GLOBALS(gobject),
	PHP_GINIT(gobject),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_GOBJECT
ZEND_GET_MODULE(gobject)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("gobject.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_gobject_globals, gobject_globals)
    STD_PHP_INI_ENTRY("gobject.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_gobject_globals, gobject_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_gobject_init_globals
 */
PHP_GINIT_FUNCTION(gobject)
{
}
/* }}} */

static GITypelib *gobj_lib = NULL, *gir_lib = NULL;

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(gobject)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	g_type_init();

	GOBJECT_G(gir) = g_irepository_get_default();

	GError *err = NULL;
	gobj_lib = g_irepository_require(GOBJECT_G(gir), "GObject", "2.0", 0, &err);
	if (err) {
		php_printf("Couldn't load GObject namespace from GIR: %s\n", err->message);
		g_error_free(err);
		return FAILURE;
	}
	gir_lib  = g_irepository_require(GOBJECT_G(gir), "GIRepository", "2.0", 0, &err);
	if (err) {
		php_printf("Couldn't load GIRepository namespace from GIR: %s\n", err->message);
		g_error_free(err);
		return FAILURE;
	}

	PHP_MINIT(gobject_paramspec)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(gobject_type)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(gobject_gobject)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(gobject_closure)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(gobject_signal)(INIT_FUNC_ARGS_PASSTHRU);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(gobject)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/

	g_typelib_free(gir_lib);
	g_typelib_free(gobj_lib);

	PHP_MSHUTDOWN_FUNCTION(gobject_gobject);
	PHP_MSHUTDOWN_FUNCTION(gobject_type);
	PHP_MSHUTDOWN_FUNCTION(gobject_signal);

	return SUCCESS;
}
/* }}} */

void my_gobject_test(TSRMLS_D)
{
	GIRepository *gir = GOBJECT_G(gir);

	GError *err = NULL;
	GITypelib *lib = g_irepository_require(gir, "cairo", "1.0", 0, &err);

	gint n_kids = g_irepository_get_n_infos(gir, "cairo");

	for (gint i = 0; i < n_kids; i++) {
		GIBaseInfo *b_info = g_irepository_get_info(gir, "cairo", i);
		GIInfoType b_info_type = g_base_info_get_type(b_info);

		php_printf("%s   \t%s\n", g_info_type_to_string(b_info_type), g_base_info_get_name(b_info));
	}
	return;

	// GIBaseInfo *b_info = g_irepository_find_by_name(GOBJECT_G(gir), "GObject", "Object");
	// GIObjectInfo *o_info = (GIObjectInfo *)b_info;
	// 
	// php_printf("Object\n");
	// gint n = g_object_info_get_n_methods(o_info);
	// for (gint i = 0; i < n; i++) {
	// 	GIFunctionInfo *f_info = g_object_info_get_method(o_info, i);
	// 
	// 	const gchar *name = g_base_info_get_name(f_info);
	// 	const gchar *f_name = g_function_info_get_symbol(f_info);
	// 
	// 	php_printf("-> %s => %s\n", name, f_name);
	// }
}

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(gobject)
{
	PHP_RINIT(gobject_signal)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_RINIT(gobject_type)(INIT_FUNC_ARGS_PASSTHRU);

	// my_gobject_test(TSRMLS_C);

	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(gobject)
{
	PHP_RSHUTDOWN(gobject_signal)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_RSHUTDOWN(gobject_type)(INIT_FUNC_ARGS_PASSTHRU);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(gobject)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "gobject support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
