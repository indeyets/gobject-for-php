<?php
$obj = new Glib\GObject\GObject;
$hdl = $obj->connect('php_test', function($param){
    echo "Hello ".$param."\n";
}, 'test');

$obj->emit('php_test');
$obj->emit('php_test');

$obj->disconnect($hdl);
echo "disconnected\n";

unset($obj);

echo "bye!\n";

// $obj->emit('php_test');
// $obj->emit('php_test');
// 
// $obj->emit('php_test2');
