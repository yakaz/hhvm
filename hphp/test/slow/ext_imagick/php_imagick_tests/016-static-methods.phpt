--TEST--
Test static methods
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

echo gettype (Imagick::queryFormats ()) . PHP_EOL;

$im = new Imagick ();
echo gettype ($im->queryFormats ()) . PHP_EOL;

echo gettype (Imagick::queryFonts ()) . PHP_EOL;
echo gettype ($im->queryFonts ()) . PHP_EOL;

echo 'success';

?>
--EXPECT--
array
array
array
array
success
