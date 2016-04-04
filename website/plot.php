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
        <script src="http://code.jquery.com/jquery-latest.min.js" type="text/javascript"></script>
        <script src="menu/script.js"></script>
        <link rel="icon" href="images/pump.png">
        <link rel="stylesheet" href="//code.jquery.com/ui/1.11.4/themes/smoothness/jquery-ui.css">
        <script src="//code.jquery.com/jquery-1.10.2.js"></script>
        <script src="//code.jquery.com/ui/1.11.4/jquery-ui.js"></script>
        <title>Pump Logger - Plot</title>
                
        <!--<meta http-equiv="refresh" content="59">*/-->
        
        <script src="libraries/RGraph.common.core.js"></script>
        <script src="libraries/RGraph.scatter.js"></script>
        <script>
            window.onload = function ()
            {
                var data1 = [ <?php $PumpLog->printLogData(); ?> ];

                var scatter = new RGraph.Scatter({
                    id: 'cvs',
                    data: [data1],
                    options: {
                        xmin: '<?php echo $todayDate ?>',          // Start of year
                        xmax: '<?php echo $todayDate ?> 23:59:59', // End of year
                        labels: [
                        <?php
                        for($i = 1; $i < 24; $i++)
                            echo "\"$i\",";
                        ?>
                        ],
                        line: true,
                        lineLinewidth: 1,
                        lineStepped: [true],
                        lineColors: ['red'],
                        lineShadowColor: '#999',
                        lineShadowBlur: 15,
                        lineShadowOffsetx: 0,
                        lineShadowOffsety: 0,
                        noxaxis: true,
                        backgroundGridAutofitNumvlines: 12
                    }
                }).draw();
            };
            
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
               <li class='active'><a href='plot.php'>Daily plot</a></li>
               <li><a href='#'>Monthly plot</a></li>
               <li><a href='#'>About</a></li>
            </ul>
            </div>
        </nav>
        
        <h2>Time plot for <?php echo date('D j F Y', strtotime($todayDate)); ?> </h2>
        <canvas id="cvs" width="1000" >[No canvas support]</canvas>
        
        <form method="post">
            Date:<input type="text" id="datepicker" value="<?php echo $todayDate ?>" name="date" ></input>
            <input type="submit" value="Refresh">
        </form>
        
        <article>
            <h1>Statistics</h1>
            <table>
                <tr><td>Running</td><td style="text-align:right;"><?php $PumpLog->printRunning(); ?></td></tr>
                <tr><td>Rested</td><td style="text-align:right;"><?php $PumpLog->printRested(); ?></td></tr>
            </table>
        </article>
      
        <footer>
        Welcome to the phat base
        </footer>
    </body>  
</html>