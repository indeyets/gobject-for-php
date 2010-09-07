<?php
use Glib\GObject\ParamSpec as GParamSpec;

$type = new Glib\GObject\Type;
$type->name = 'test';
$type->parent = 'GObject';
$type->properties[] = GParamSpec::string ('field1', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::boolean('field2', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::long   ('long',   GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::double ('field4', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::int    ('int',    GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::float  ('field6', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::char   ('char',   GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::uchar  ('uchar',  GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::uint   ('uint',   GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->generate();


$obj = new test;
echo "1:\n";
$obj->field1 = 'test';
echo "2:\n";
$obj->field2 = true;
echo "3:\n";
$obj->long = 0xfeedbabefeedbabe;
echo "4:\n";
$obj->field4 = -0.1234567890;
echo "5:\n";
$obj->int = 0xfeedbabefeedbabe;
echo "6:\n";
$obj->field6 = -0.1234567890;

echo "char:\n";
$obj->char = 1000;
echo "uchar:\n";
$obj->uchar = 1000;
echo "uint:\n";
$obj->uint = 0xfeedbabefeedbabe;

echo "\n\n\n";

echo "uint: ";
var_dump($obj->uint);
echo "uchar: ";
var_dump($obj->uchar);
echo "char: ";
var_dump($obj->char);
echo "6: ";
var_dump($obj->field6);
echo "int: ";
var_dump($obj->int);
echo "4: ";
var_dump($obj->field4);
echo "long: ";
var_dump($obj->long);
echo "2: ";
var_dump($obj->field2);
echo "1: ";
var_dump($obj->field1);
