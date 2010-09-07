--TEST--
Test generation of namespaced classes
--SKIPIF--
<?php if (!extension_loaded("gobject")) print "skip"; ?>
--FILE--
<?php
var_dump(class_exists('test'));
$type = new Glib\GObject\Type;
$type->name = 'test';
$type->parent = 'GObject';
$type->generate();
var_dump(class_exists('test'));

var_dump(get_parent_class('test') == 'Glib\GObject\GObject');

var_dump(class_exists('super\test'));
$type = new Glib\GObject\Type;
$type->name = 'super__test';
$type->parent = 'test';
$type->generate();
var_dump(class_exists('super\test'));

var_dump(get_parent_class('super\test') == 'test');

var_dump(class_exists('the\most\super\test'));
$type = new Glib\GObject\Type;
$type->name = 'the__most__super__test';
$type->parent = 'super__test';
$type->generate();
var_dump(class_exists('the\most\super\test'));

var_dump(get_parent_class('the\most\super\test') == 'super\test');

?>
==DONE==
--EXPECT--
bool(false)
bool(true)
bool(true)
bool(false)
bool(true)
bool(true)
bool(false)
bool(true)
bool(true)
==DONE==
