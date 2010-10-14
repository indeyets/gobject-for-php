<?php
use GObject\ParamSpec as GParamSpec;

$type = new GObject\Type;
$type->name = 'MFS__test';
$type->parent = 'GObject';
$type->properties[] = GParamSpec::string ('string',  GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::boolean('boolean', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::long   ('long',    GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::double ('double',  GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::int    ('int',     GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::float  ('float',   GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::char   ('char',    GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::uchar  ('uchar',   GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::uint   ('uint',    GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->properties[] = GParamSpec::unichar('unichar', GParamSpec::READABLE|GParamSpec::WRITABLE);
$type->generate();


$obj = new MFS\test;
$obj->string = 'test';
$obj->boolean = true;
$obj->long = 0xfeedbabefeedbabe;
$obj->double = -0.1234567890;
$obj->int = 0xfeedbabefeedbabe;
$obj->float = -0.1234567890;
$obj->char = 1000;
$obj->uchar = 1000;
$obj->uint = 0xfeedbabefeedbabe;
$obj->unichar = 'Я';

echo "uint:    "; var_dump($obj->uint);
echo "uchar:   "; var_dump($obj->uchar);
echo "char:    "; var_dump($obj->char);
echo "float:   "; var_dump($obj->float);
echo "int:     "; var_dump($obj->int);
echo "double:  "; var_dump($obj->double);
echo "long:    "; var_dump($obj->long);
echo "boolean: "; var_dump($obj->boolean);
echo "string:  "; var_dump($obj->string);
echo "unichar: "; var_dump($obj->unichar);


var_dump(GIRepository\load_ns('GObject'));
var_dump(GIRepository\load_ns('Gio'));

// echo "### reflection\n";
// $rc = new ReflectionClass('Gio\InputStream');
// foreach ($rc->getMethods() as $rm) {
//     echo $rm."\n";
// }

// echo "### creating object\n";
// $obj2 = new Gio\InputStream;
// var_dump($obj2);
// 
// echo "### calling method\n";
// var_dump($obj2->read(1, 2, 3));

echo "### calling function\n";
var_dump(Gio\content_type_from_mime_type('text/html; charset=utf-8'));
