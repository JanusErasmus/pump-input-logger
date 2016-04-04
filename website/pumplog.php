<?php
class pumplog
{
    private $events = array();
    private $onTime = 0;
    private $restTime = 0;
    
    function __construct($todayDate)
    {
        $conn = new mysqli("localhost", "root", "VictorHanny", "pumplog");
        if($conn->connect_error)
        {
            die("Connection failed: " . $conn->connect_error);
        }
        
        $sql = "select * from events where time > \"" . date('Y-m-d', strtotime($todayDate)) . "\" and time < \"" . date('Y-m-d', strtotime($todayDate)) . " 23:59:59\" order by time";
        $events = $conn->query($sql);

        if ($events->num_rows >= 1)
        {          
            $index = 0;
            while($row = $events->fetch_assoc()) 
            {
                $this->events[$index++] = [$row["time"], $row["port"], $row["state"]];
            }
        }        
                       
        $conn->close();    

        $this->calculateTimes();
    }
    
    function printLogData()
    {     
        for($row = 0; $row < count($this->events); $row++) 
        {
            echo "[\"" . $this->events[$row][0] . "\"," . $this->events[$row][2] * 100 . "],";
        }
    }
    
    function printEvents()
    {        
        for($row = 0; $row < count($this->events); $row++) 
        {
            echo  $this->events[$row][0] . " " . $this->events[$row][2] . "<br>";
        }       
    }
    
    function calculateTimes()
    {
        $previousTime = 0;
        $state = 0;
        
        for($row = 0; $row < count($this->events); $row++) 
        {
            $evtTime = strtotime($this->events[$row][0]);
            //echo "evtTime " . $evtTime . "<br>";
            
            if($previousTime == 0)
                $previousTime = $evtTime;            
            //echo "previousTime " . $previousTime . "<br>";
            
            //state changed
            if($state != $this->events[$row][2])
            {
                //low to high
                if($state == 0)
                {
                    $this->restTime += ($evtTime - $previousTime);
                    //echo "rested " . $this->restTime . "<br>";
                }
                else //high to low
                {
                    $this->onTime += ($evtTime - $previousTime);
                    //echo "ran " . $this->onTime . "<br>";
                }
                
                $previousTime = $evtTime;
                $state = $this->events[$row][2];
            }
        } 
    }
    
    function printUpTime($time)
    {
        $days = floor($time / 86400);        
        $calc = $time - ($days * 86400);
        
        $hours = floor($calc / 3600);
        $calc -= $hours * 3600;
        
        $minutes = floor($calc / 60) ;
        $calc -= $minutes * 60;
        
        $seconds = $calc;
        
        if($days)
            echo $days . "days ";
        
        if($hours)
            echo $hours . "hours ";
        
        if($minutes)
            echo $minutes . "minutes ";
         
        echo $seconds . "seconds";
    }
    
    function printRunning()
    {
        if($this->onTime)
            $this->printUpTime($this->onTime);
        else
            echo "-";
    }
    
    function printRested()
    {
        //This is the calculated from event logs 
        if($this->restTime)
            $this->printUpTime($this->restTime);
        else
            echo "-";
        
        //simply take the difference
        //$this->printUpTime(86400 - $this->onTime);
    }
}
?>
