<?php

if (!function_exists('pake_require_version'))
    throw new pakeException('your Pake is too old. Please upgrade');

pake_require_version('1.4.3');
pake_import('phpExtension');


pake_desc('display information about extension');
pake_task('info');

pake_desc('run GDB');
pake_task('debug_local_test');

pake_desc('try to execute test.php script');
pake_task('local_test');

function run_info()
{
    echo pake_sh('php --re gobject');
}

function run_debug_local_test()
{
    pake_sh('gdb --args php test.php', true);
}

function run_local_test()
{
    echo pake_sh("php test.php");
}
