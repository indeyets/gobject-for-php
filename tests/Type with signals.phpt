--TEST--
Test generation of classes with signals
--SKIPIF--
<?php if (!extension_loaded("gobject")) print "skip"; ?>
--FILE--
<?php
$obj = new Glib\Gobject\Type;
$obj->name = 'test';
$obj->parent = 'GObject';

$s1 = new Glib\Gobject\Signal();
$s2 = new Glib\Gobject\Signal();

$obj->signals['something1'] = $s1;
$obj->signals['something2'] = $s2;
$obj->generate();

$obj2 = new test;
$hdl = $obj2->connect('something1', function($param){
    echo "Hello ".$param."\n";
}, 'test');

$obj2->emit('something1');
?>
==DONE==
--EXPECT--
Hello test
==DONE==