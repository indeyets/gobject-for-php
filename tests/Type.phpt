--TEST--
Check for gobject presence
--SKIPIF--
<?php if (!extension_loaded("gobject")) print "skip"; ?>
--FILE--
<?php
$obj = new Glib\Gobject\Type;

try {
    $obj->dummy = 'dummy';
    echo "error (shouldn't be able to set non-existant property)\n";
} catch (\OutOfBoundsException $e) {
    echo "ok\n";
}

$obj->name = 'test';
var_dump($obj->name === 'test');

$obj->parent = 'GObject';
var_dump($obj->parent === 'GObject');

$obj->generate();

$obj2 = new test;
var_dump($obj2 instanceof GLib\GObject\GObject);
?>
==DONE==
--EXPECT--
ok
bool(true)
bool(true)
bool(true)
==DONE==