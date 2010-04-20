<?php

pake_desc('Clean all temporary files');
pake_task('clean');

pake_task('configure');

pake_task('build', 'configure');

pake_desc('configure, build and install extension');
pake_task('install', 'build');

pake_desc('reconfigure (needed if you change config.m4 file)');
pake_task('reconfigure', 'clean', 'configure');

pake_desc('display information about extension');
pake_task('info');

pake_desc('run GDB');
pake_task('debug');

pake_desc('try to execute test.php script');
pake_task('test');

function run_configure()
{
    if (!file_exists('configure'))
        pake_sh('phpize');

    if (!file_exists('Makefile')) {
        // pake_sh('CC=/opt/llvm/bin/clang '.realpath('configure'));
        pake_sh(realpath('configure'));
    }
}

function run_reconfigure() {} // virtual task

function run_build()
{
    pake_sh('make', true);
}

function run_install()
{
    pake_superuser_sh('make install');
}

function run_clean()
{
    if (file_exists('Makefile'))
        pake_sh('make distclean');

    if (file_exists('configure'))
        pake_sh('phpize --clean');
}

function run_info()
{
    echo pake_sh('php --re gobject');
}

function run_debug()
{
    pake_sh('gdb php', true);
}

function run_test()
{
    echo pake_sh("php test.php");
}
