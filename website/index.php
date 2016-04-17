<!DOCTYPE html>

<?php
    include ("pumpConfig.php");
    
    $redStyle = "";
    $loggersInfo = array();
    $cfg = new PumpConfig();   

    if( isset($_POST['rate']) && !empty($_POST['rate']) &&
        isset($_POST['start']) && !empty($_POST['start']) &&
        isset($_POST['stop']) && !empty($_POST['stop']) &&
        isset($_POST['on']) && !empty($_POST['on']) &&
        isset($_POST['rest']) && !empty($_POST['rest'])    
        )
    {
        $cfg->store($_POST['rate'], $_POST['start'], $_POST['stop'], $_POST['on'], $_POST['rest']); 
    }    
    
    
    //read last dialed time stamp
     $conn = new mysqli("localhost", "root", "VictorHanny", "pumplog");
        if($conn->connect_error)
        {
            die("Connection failed: " . $conn->connect_error);
        }
        
        $sql = "select * from loggers";
        $loggers = $conn->query($sql);

        if ($loggers->num_rows >= 1)
        {          
            $index = 0;
            while($row = $loggers->fetch_assoc()) 
            {
                $loggersInfo[$index++] = [$row["mac"], $row["last_logged"], $row["rssi"]];                
            }
        }        
                      
        $conn->close();  
        
        if((strtotime($loggersInfo[0][1]) + ($cfg->rate * 60)) < (time()))
            $redStyle = "style=\"background-color:#FF5858\";";

?>

<html>
<meta charset="UTF-8">
    <head>    
        <link href="styles.css" rel="stylesheet" type="text/css">
        <link  href="menu/styles.css" rel="stylesheet">
        <script src="http://code.jquery.com/jquery-latest.min.js" type="text/javascript"></script>
        <script src="menu/script.js"></script>
        <link rel="icon" href="images/pump.png">
        <title>Pump Input Logger</title>
    </head>

    <body>
        <header>            
            <?php include "header.html"; ?>
        </header>
         <nav>
            <div id='cssmenu'>
            <ul>
               <li class='active'><a href='index.php'>Home</a></li>
               <li><a href='plot.php'>Daily plot</a></li>
               <li><a href='#'>Monthly plot</a></li>
               <li><a href='#'>About</a></li>
            </ul>
            </div>
        </nav>

        <article <?php echo $redStyle; ?>>        
            <h1>Logger Info</h1>
            <?php echo $loggersInfo[0][0] . " last dialed " . $loggersInfo[0][1] . " with RSSI: " . $loggersInfo[0][2]; ?>
            <form method="post" <?php echo $redStyle; ?>>            
                <table <?php echo $redStyle; ?>>
                    <tr><td>Report rate: (minutes)</td><td><input type="text" value="<?php echo $cfg->rate; ?>" name="rate" ></input></td></tr>
                    <tr><td>Start Hour:</td><td><input type="text" value="<?php echo $cfg->start; ?>" name="start" ></input></td></tr>
                    <tr><td>Stop Hour:</td><td><input type="text" value="<?php echo $cfg->stop; ?>" name="stop" ></input></td></tr>
                    <tr><td>On Time: (minutes)</td><td><input type="text" value="<?php echo $cfg->on; ?>" name="on" ></input></td></tr>
                    <tr><td>Rest Time: (minutes)</td><td><input type="text" value="<?php echo $cfg->rest; ?>" name="rest" ></input></td></tr>
                    <tr><td></td><td><input type="submit" value="Apply"></td></tr>
                </table>
            </form>        
        </article>
        	
         <footer>        
            <b>Nice ne</b>        
        </footer>        
    </body>
</html>
