#include "php_gobject.h"

typedef struct {
	GObject std;
	HashTable *glib_properties;
} PhpGObject;

typedef struct _gobject_paramspec_object {
	zend_object std;
	GParamSpec *paramspec;
} gobject_paramspec_object;

typedef struct _gobject_type_object {
	zend_object std;
	GType gtype;
	GType parent;
	zval *name;
	zval *properties; // ArrayObject
	zval *signals;    // ArrayObject
	zval *interfaces; // ArrayObject
	zend_bool is_registered;
} gobject_type_object;

typedef struct _gobject_signal_object {
	zend_object std;
	guint signal_id;
	GSignalFlags flags;
	zval *param_types; // array
	GType return_type;
	zend_fcall_info class_closure_fci;
	zend_fcall_info_cache class_closure_fci_cache;
	zend_fcall_info accumulator_fci;
	zend_fcall_info_cache accumulator_fci_cache;
} gobject_signal_object;

typedef struct _gobject_gobject_object {
	zend_object std;
	GObject *gobject;
	GSList *closures;
} gobject_gobject_object;

#define __php_objstore_object(obj_zval) ((gobject_gobject_object *)zend_object_store_get_object(obj_zval TSRMLS_CC))
#define __php_gobject_ptr(obj_zval) (__php_objstore_object(obj_zval)->gobject)

ZEND_BEGIN_MODULE_GLOBALS(gobject)
	GIRepository *gir;
	HashTable signals_hash;
	HashTable class_closure_hash;
	HashTable runtime_functions;
ZEND_END_MODULE_GLOBALS(gobject)

ZEND_EXTERN_MODULE_GLOBALS(gobject)

#ifdef ZTS
# define GOBJECT_G(v) TSRMG(gobject_globals_id, zend_gobject_globals *, v)
#else
# define GOBJECT_G(v) (gobject_globals.v)
#endif

// extension
PHP_MINIT_FUNCTION(gobject);
PHP_MSHUTDOWN_FUNCTION(gobject);
PHP_RINIT_FUNCTION(gobject);
PHP_RSHUTDOWN_FUNCTION(gobject);
PHP_MINFO_FUNCTION(gobject);
PHP_GINIT_FUNCTION(gobject);

// component init
PHP_MINIT_FUNCTION(gobject_paramspec);
PHP_MINIT_FUNCTION(gobject_type);
PHP_MINIT_FUNCTION(gobject_gobject);
PHP_MINIT_FUNCTION(gobject_closure);
PHP_MINIT_FUNCTION(gobject_signal);

// component shutdown
PHP_MSHUTDOWN_FUNCTION(gobject_type);
PHP_MSHUTDOWN_FUNCTION(gobject_gobject);
PHP_MSHUTDOWN_FUNCTION(gobject_signal);

PHP_RINIT_FUNCTION(gobject_girepository);
PHP_RINIT_FUNCTION(gobject_signal);
PHP_RINIT_FUNCTION(gobject_type);

PHP_RSHUTDOWN_FUNCTION(gobject_girepository);
PHP_RSHUTDOWN_FUNCTION(gobject_signal);
PHP_RSHUTDOWN_FUNCTION(gobject_type);

PHP_FUNCTION(GIRepository_load_ns);

GClosure *php_gobject_closure_new(GObject *gobject, zend_fcall_info fci, zend_fcall_info_cache fci_cache, zval ***params, int params_count TSRMLS_DC);
GClosure *php_gobject_closure_new_class(zend_fcall_info fci, zend_fcall_info_cache fci_cache TSRMLS_DC);
void php_gobject_closure_marshal(GClosure *closure, GValue *return_value, guint n_param_values, const GValue *param_values, gpointer invocation_hint, gpointer marshal_data);
gboolean php_gobject_closure_accumulator(GSignalInvocationHint *ihint, GValue *return_accu, const GValue *handler_return, gpointer data);


gunichar g_utf8_to_unichar(const char *src, int len);

zend_bool zval_to_gvalue(const zval *zvalue, GValue *gvalue, zend_bool init TSRMLS_DC);
zend_bool zval_with_gtype_to_gvalue(GType type, const zval *zvalue, GValue *gvalue, zend_bool init TSRMLS_DC);
zend_bool gvalue_to_zval(const GValue *gvalue, zval *zvalue TSRMLS_DC);
zend_bool gvalue_with_gtype_to_zval(GType g_gtype, const GValue *gvalue, zval *zvalue TSRMLS_DC);

zend_bool php_gobject_zval_to_giarg(zval *zvalue, GIArgInfo *arg_info, GIArgument *giarg TSRMLS_DC);

GType g_type_from_phpname(const char *name TSRMLS_DC);
char* namespaced_name(const char *ns_name, const char *name, zend_bool persistent);
void parse_namespaced_name(const char *src, char **namespace, char **name);
char* phpname_from_gtype(GType type);

zend_bool callback_is_empty(zend_fcall_info *fci);

zend_bool php_gobject_store_signal_association(zval *signal TSRMLS_DC);
zend_bool php_gobject_remove_signal_association(guint signal_id TSRMLS_DC);
zval * php_gobject_signal_get_by_id(guint signal_id TSRMLS_DC);

void php_gobject_gobject_init(PhpGObject *self, gpointer g_class);
void php_gobject_gobject_finalize(PhpGObject *self);
void php_gobject_gobject_set_glib_property(PhpGObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
void php_gobject_gobject_get_glib_property(PhpGObject *object, guint property_id, GValue *value, GParamSpec *pspec);
