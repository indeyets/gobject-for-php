<?php
$obj = new Glib\GObject\Type;
$obj->name = 'test';
$obj->parent = 'GObject';

$s1 = new Glib\GObject\Signal(0, array('Glib\GObject\GObject'));
$s2 = new Glib\Gobject\Signal();

$obj->signals['something1'] = $s1;
$obj->signals['something2'] = $s2;
$obj->generate();

$obj2 = new test;
$hdl = $obj2->connect('something1', function($obj, $param){
    echo "Hello ".$param.' (I got '.get_class($obj).")\n";
}, 'test');

$hdl = $obj2->connect('something2', function($param){
    echo "Hello ".$param."\n";
}, 'test');

$tmp = new Glib\GObject\GObject;

$obj2->emit('something1', $tmp);
$obj2->emit('something2');
