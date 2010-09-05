<?php
$obj = new Glib\GObject\Type;
$obj->name = 'test';
$obj->parent = 'GObject';

$s1 = new Glib\GObject\Signal(
    0,                              // flags
    array('Glib\GObject\GObject'),  // param
    'gchararray',                   // retval
    'var_dump',                     // default closure
    function(&$result, $reply) {
        $result .= $reply;
        // var_dump($result, $reply);
        echo "Accumulator!\n";
    }                           // accu
);
// $s2 = new Glib\Gobject\Signal();

$obj->signals['something1'] = $s1;
// $obj->signals['something2'] = $s2;
$obj->generate();

$obj2 = new test;
$hdl = $obj2->connect(
    'something1',
    function($self, $obj2, $param)
    {
        echo "closure1: Hello ".$param.' (I got '.get_class($self)." and ".get_class($obj2).")\n";
        return 'closure1 ';
    },
    'test'
);

$hdl2 = $obj2->connect(
    'something1',
    function($self, $obj2, $param)
    {
        echo "closure2: Hi. I am the second closure!\n";
        return 'closure2 ';
    },
    'test'
);

// $hdl = $obj2->connect('something2', function($param){
//     echo "Hello ".$param."\n";
// }, 'test');

$tmp = new Glib\GObject\GObject;

echo "=-=-=-=-=-=-=-=-=-\n";
echo " Emitting\n";
echo "=-=-=-=-=-=-=-=-=-\n";
var_dump($obj2->emit('something1', $tmp));
// $obj2->emit('something2');
