--TEST--
Test generation of classes with signals and accumulator
--SKIPIF--
<?php if (!extension_loaded("gobject")) print "skip"; ?>
--FILE--
<?php
$accu = function(&$result, $reply) {
    $result += $reply;
};

$s1 = new Glib\GObject\Signal(0, array(), 'gint', 'var_dump', $accu);

$type = new Glib\GObject\Type;
$type->name = 'test';
$type->parent = 'GObject';
$type->signals['something1'] = $s1;
$type->generate();

$obj = new test;
$obj->connect('something1', function($self) {
    return 1;
});

$obj->connect('something1', function($self) {
    return 2;
});

$obj->connect('something1', function($self) {
    return 3;
});

$result = $obj->emit('something1');

var_dump($result === 6);
?>
==DONE==
--EXPECT--
bool(true)
==DONE==
