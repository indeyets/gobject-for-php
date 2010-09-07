<?php
use Glib\GObject\ParamSpec as GParamSpec;

$type = new Glib\GObject\Type;
$type->name = 'test';
$type->parent = 'GObject';
// $type->signals['something1'] = $s1;
// $type->signals['something2'] = $s2;
$type->properties[] = GParamSpec::string('field1', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::string('field2', GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::string('field3', GParamSpec::READABLE);
$type->properties[] = GParamSpec::string('field4', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->generate();

// $tmp = new Glib\GObject\GObject;

$obj = new test;
echo "1:\n";
$obj->field1 = 'test';
echo "2:\n";
$obj->field2 = 'test2';
// $obj->field3 = 'test3';
echo "4:\n";
$obj->field4 = 'test4';

echo "\n\n\n";

echo "4:\n";
var_dump($obj->field4);
// var_dump($obj->field3);
// var_dump($obj->field2);
echo "1:\n";
var_dump($obj->field1);

// echo "=-=-=-=-=-=-=-=-=-\n";
// echo " Emitting\n";
// echo "=-=-=-=-=-=-=-=-=-\n";
// $obj->emit('something1', $tmp);
// $obj2->emit('something2');
