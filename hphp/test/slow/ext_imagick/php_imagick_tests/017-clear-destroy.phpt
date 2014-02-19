--TEST--
Clear and destroy aliases
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

$im = new Imagick ();
$im->clear ();
$im->destroy ();

$im = new ImagickDraw ();
$im->clear ();
$im->destroy ();

$im = new ImagickPixel ();
$im->clear ();
$im->destroy ();

$magick = new Imagick ('magick:rose');

$im = new ImagickPixelIterator ($magick);
$im->clear ();
$im->destroy ();

echo 'success';

?>
--EXPECT--
success
