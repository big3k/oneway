<?php

//srand(date('d'));  change daily
srand(date('W'));   //weekly
$pwd=rand(100000, 999999); 
$fn=$pwd . '.png';

header("Content-type: image/png");
$im = @imagecreate(70, 20)
    or die("Cannot Initialize new GD image stream");
$background_color = imagecolorallocate($im, 255, 255, 255);
$text_color = imagecolorallocate($im, 0, 0, 200);
imagestring($im, 5, 5, 5,  $pwd, $text_color);
imagepng($im, $fn);
imagepng($im);
imagedestroy($im);
?>

