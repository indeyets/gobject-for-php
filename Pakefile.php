<?php

pake_task('configure');
pake_task('clean');

pake_task('build', 'configure');
pake_task('install', 'build');
pake_task('reconfigure', 'clean', 'configure');

pake_task('info');
pake_task('debug');
pake_task('test');

function run_configure()
{
    if (!file_exists('configure'))
        pake_sh('phpize');

    if (!file_exists('Makefile')) {
        pake_sh('CC=/opt/llvm/bin/clang '.realpath('configure'));
        // pake_sh(realpath('configure'));
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
