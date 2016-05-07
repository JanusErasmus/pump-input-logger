<?php

$todayDate = date('Y/m/d');
if( isset($_POST['date']) && !empty($_POST['date']))
{
    $todayDate = $_POST['date']; 
}

$conn = new mysqli("localhost", "root", "VictorHanny", "pumplog");
if($conn->connect_error)
{
	die("Connection failed: " . $conn->connect_error);
}
$events = array();
$sql = "select * from events where time > \"" . date('Y-m-01', strtotime($todayDate)) . "\" and time < \"" . date('Y-m-31', strtotime($todayDate)) . " 23:59:59\" order by time";
$result = $conn->query($sql);
if ($result->num_rows >= 1)
        {
            $index = 0;
            while($row = $result->fetch_assoc()) 
            {
                $events[$index++] = [$row["time"], $row["port"], $row["state"]];
            }
        }
$conn->close();

$list[] = array("Date", "Port", "State");

for($row = 0; $row < count($events); $row++) 
{
	$list[] = $events[$row];
}

//define headers for CSV 
header('Content-Type: text/csv; charset=utf-8');
header('Content-Disposition: attachment; filename=PumpLog_'. date('Y-m', strtotime($todayDate)) . '.csv');
//write data into CSV
$fp = fopen('php://output', 'w');
//convert data to UTF-8 
fprintf($fp, chr(0xEF).chr(0xBB).chr(0xBF));
foreach ($list as $line) {
    fputcsv($fp, $line);
}
fclose($fp);

?>
