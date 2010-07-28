--TEST--
Test generation of classes with signals
--SKIPIF--
<?php if (!extension_loaded("gobject")) print "skip"; ?>
--FILE--
<?php
$obj = new Glib\Gobject\Type;
$obj->name = 'test';
$obj->parent = 'GObject';

$s1 = new Glib\Gobject\Signal(0, array('Glib\GObject\GObject'));
$s2 = new Glib\Gobject\Signal();

$obj->signals['something1'] = $s1;
$obj->signals['something2'] = $s2;
$obj->generate();


$tmp = new Glib\GObject\GObject;


$obj2 = new test;
$hdl = $obj2->connect('something1', function($self, $obj, $param) use ($tmp, $obj2) {
    echo "Hello ".$param.' (I got '.get_class($obj).")\n";
    var_dump(spl_object_hash($tmp) === spl_object_hash($obj));
    var_dump(spl_object_hash($self) === spl_object_hash($obj2));
}, 'test');

$hdl = $obj2->connect('something2', function($self, $param){
    echo "Hello ".$param."\n";
}, 'test2');


$obj2->emit('something1', $tmp);
$obj2->emit('something2');
?>
==DONE==
--EXPECT--
Hello test (I got Glib\GObject\GObject)
bool(true)
bool(true)
Hello test2
==DONE==
