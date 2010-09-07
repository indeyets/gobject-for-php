<?php
use Glib\GObject\ParamSpec as GParamSpec;

$type = new Glib\GObject\Type;
$type->name = 'test';
$type->parent = 'GObject';
$type->properties[] = GParamSpec::string('field1', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::boolean('field2', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::long('field3', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->generate();


$obj = new test;
echo "1:\n";
$obj->field1 = 'test';
echo "2:\n";
$obj->field2 = true;
echo "3:\n";
$obj->field3 = 0xFEEDBABE;

echo "\n\n\n";

echo "3:\n";
var_dump($obj->field3);
echo "2:\n";
var_dump($obj->field2);
echo "1:\n";
var_dump($obj->field1);
