<?php
include "pumplog.php";

$todayDate = date('Y/m/d');
if( isset($_POST['date']) && !empty($_POST['date']))
{
    $todayDate = $_POST['date']; 
}

$PumpLog = new pumplog($todayDate);

?>

<!DOCTYPE html>
<meta charset="UTF-8">
    <head>    
        <link href="styles.css" rel="stylesheet" type="text/css">
        <link  href="menu/styles.css" rel="stylesheet">
        <script src="libraries/jquery-latest.min.js" type="text/javascript"></script>
        <script src="menu/script.js"></script>
        <link rel="icon" href="images/pump.png">
        <link rel="stylesheet" href="jquery-ui.css">
        <script src="libraries/jquery-1.10.2.js"></script>
        <script src="libraries/jquery-ui.js"></script>
        <title>Pump Logger - Month</title>
                
        <!--<meta http-equiv="refresh" content="59">*/-->
       <script> 
              $(function() {
                $( "#datepicker" ).datepicker();
              });
        </script>
    </head>
    <body>
        <header>
           <?php include "header.html"; ?>            
        </header>
        <nav>
            <div id='cssmenu'>
            <ul>
               <li><a href='index.php'>Home</a></li>
               <li><a href='plot.php'>Daily plot</a></li>
               <li class='active'><a href='month.php'>Monthly</a></li>
            </ul>
            </div>
        </nav>
        
        <form method="post" action='download_month_csv.php'>
            Month:<input type="text" id="datepicker" value="<?php echo $todayDate ?>" name="date" ></input>
            <input type="submit" value="Download">
        </form>


	<footer>
	Logger Daemon May 2016 
        </footer>
    </body>  
</html>
