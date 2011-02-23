--TEST--
Test import of introspected constants
--SKIPIF--
<?php if (!extension_loaded("gobject")) print "skip"; ?>
--FILE--
<?php
GIRepository\load_ns('GObject');
GIRepository\load_ns('Gio');

var_dump(Gio\VOLUME_MONITOR_EXTENSION_POINT_NAME == 'gio-volume-monitor');
?>
==DONE==
--EXPECT--
bool(true)
==DONE==
