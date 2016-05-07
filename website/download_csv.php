<?php
include "pumplog.php";

$todayDate = date('Y/m/d');
if( isset($_POST['date']) && !empty($_POST['date']))
{
    $todayDate = $_POST['date']; 
}

$PumpLog = new pumplog($todayDate);
$events = $PumpLog->getEvents();

$list[] = array("Date", "Port", "State");

for($row = 0; $row < count($events); $row++) 
{
	$list[] = $events[$row];
}

//define headers for CSV 
header('Content-Type: text/csv; charset=utf-8');
header('Content-Disposition: attachment; filename=PumpLog_'. $todayDate . '.csv');
//write data into CSV
$fp = fopen('php://output', 'w');
//convert data to UTF-8 
fprintf($fp, chr(0xEF).chr(0xBB).chr(0xBF));
foreach ($list as $line) {
    fputcsv($fp, $line);
}
fclose($fp);

?>
