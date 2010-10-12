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

#ifndef PHP_GOBJECT_EXT_H
#define PHP_GOBJECT_EXT_H

#define PHP_GOBJECT_VERSION "0.1.0-dev"
#define GOBJECT_NAMESPACE "GObject"
#define GIR_NAMESPACE "GIRepository"

extern zend_module_entry gobject_module_entry;
#define phpext_gobject_ptr &gobject_module_entry

#ifdef PHP_WIN32
# define PHP_GOBJECT_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
# define PHP_GOBJECT_API __attribute__ ((visibility("default")))
#else
# define PHP_GOBJECT
#endif

#ifdef ZTS
# include "TSRM.h"
#endif

#include <glib-object.h>
#include <girepository.h>


// api
extern zend_class_entry *gobject_ce_gobject;
zend_object_value gobject_gobject_object_new(zend_class_entry *ce TSRMLS_DC);

#endif	/* PHP_GOBJECT_EXT_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
