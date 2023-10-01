--TEST--
Check for fileselect presence
--SKIPIF--
<?php if (!extension_loaded("Dgbaodev Libs : File selector")) print file_select()."\n"; ?>
--FILE--
<?php 
echo "File select extension is available";
?>
--EXPECT--
File select extension is available
