<?php
use Glib\GObject\ParamSpec as GParamSpec;

$type = new Glib\GObject\Type;
$type->name = 'test';
$type->parent = 'GObject';
$type->properties[] = GParamSpec::string ('field1', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::boolean('field2', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::long   ('field3', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::double ('field4', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::int    ('field5', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::float  ('field6', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::char   ('char',   GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::uchar  ('uchar',  GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->generate();


$obj = new test;
echo "1:\n";
$obj->field1 = 'test';
echo "2:\n";
$obj->field2 = true;
echo "3:\n";
$obj->field3 = 0xFEEDBABE;
echo "4:\n";
$obj->field4 = -0.1234567890;
echo "5:\n";
$obj->field5 = 0xFEEDBABE;
echo "6:\n";
$obj->field6 = -0.1234567890;

echo "char:\n";
$obj->char = 1000;
echo "uchar:\n";
$obj->uchar = 1000;

echo "\n\n\n";

echo "uchar:\n";
var_dump($obj->uchar);
echo "char:\n";
var_dump($obj->char);
echo "6:\n";
var_dump($obj->field6);
echo "5:\n";
var_dump($obj->field5);
echo "4:\n";
var_dump($obj->field4);
echo "3:\n";
var_dump($obj->field3);
echo "2:\n";
var_dump($obj->field2);
echo "1:\n";
var_dump($obj->field1);
