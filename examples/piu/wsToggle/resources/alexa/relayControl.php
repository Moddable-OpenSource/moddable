<?php
/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

$file = fopen("/home/acarle/relay/status", "r") or die("Unable to open file for reading!");
$data = fread($file, 2);
fclose($file);

$one = $data[0];
$two = $data[1];
$update = false;

$os = $_GET["one"];
$ts = $_GET["two"];

if ($os){
  if (!(strcmp($os, "on"))){
    if ($one != "1"){
      $one = "1";
      $update = true;
    }
  }elseif(!(strcmp($os, "off"))){
    if ($one != "0"){
      $one = "0";
      $update = true;
    }
  }elseif(!(strcmp($os, "toggle"))){
    if ($one == "0"){
      $one = "1";
    }else{
      $one = "0";
    }
    $update = true;
  }
}

if ($ts){
  if (!(strcmp($ts, "on"))){
    if ($two != "1"){
      $two = "1";
      $update = true;
    }
  }elseif(!(strcmp($ts, "off"))){
    if ($two != "0"){
      $two = "0";
      $update = true;
    }
  }elseif(!(strcmp($ts, "toggle"))){
    if ($two == "0"){
      $two = "1";
    }else{
      $two = "0";
    }
    $update = true;
  }
}

if($update){
  $file = fopen("/home/acarle/relay/status", "w") or die("Unable to open file for writing!");
  fwrite($file, $one . $two);
  fclose($file);
}

echo($one . $two);
?>
