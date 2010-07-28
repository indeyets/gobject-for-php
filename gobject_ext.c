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

#include "php_gobject.h"

ZEND_DECLARE_MODULE_GLOBALS(gobject)

/* True global resources - no need for thread safety here */
// static int le_gobject;

/* {{{ gobject_functions[]
 *
 * Every user visible function must have an entry in gobject_functions[].
 */
const zend_function_entry gobject_functions[] = {
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

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(gobject)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	g_type_init();

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
	PHP_MSHUTDOWN_FUNCTION(gobject_gobject);
	PHP_MSHUTDOWN_FUNCTION(gobject_type);
	PHP_MSHUTDOWN_FUNCTION(gobject_signal);

	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(gobject)
{
	PHP_RINIT(gobject_signal)(INIT_FUNC_ARGS_PASSTHRU);

	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(gobject)
{
	PHP_RSHUTDOWN(gobject_signal)(INIT_FUNC_ARGS_PASSTHRU);

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
