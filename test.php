<?php
use Glib\GObject\ParamSpec as GParamSpec;

$param = GParamSpec::string('field1', GParamSpec::READABLE|GParamSpec::WRITABLE);

$type = new Glib\GObject\Type;
$type->name = 'test';
$type->parent = 'GObject';
// $type->signals['something1'] = $s1;
// $type->signals['something2'] = $s2;
$type->properties[] = $param;
$type->generate();

// $tmp = new Glib\GObject\GObject;

$obj = new test;
$obj->field1 = 'test';
var_dump($obj->field1);


// echo "=-=-=-=-=-=-=-=-=-\n";
// echo " Emitting\n";
// echo "=-=-=-=-=-=-=-=-=-\n";
// $obj->emit('something1', $tmp);
// $obj2->emit('something2');
