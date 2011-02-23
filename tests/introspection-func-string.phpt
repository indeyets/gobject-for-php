--TEST--
Test import of introspected constants
--SKIPIF--
<?php if (!extension_loaded("gobject")) print "skip"; ?>
--FILE--
<?php
GIRepository\load_ns('GObject');
GIRepository\load_ns('Gio');

var_dump(Gio\content_type_from_mime_type('text/html; charset=utf-8') == 'text/html; charset=utf-8');
?>
==DONE==
--EXPECT--
bool(true)
==DONE==
