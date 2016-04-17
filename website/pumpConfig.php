<?php
class PumpConfig {

    private $config_file = "/etc/loggerd/user.var";

    public $rate;
    public $start;
    public $stop;
    public $on;
    public $rest;
    
    function __construct() {
     
        $fh = fopen($this->config_file, "r") or die("can't open file for reading");
        
        while (($line = fgets($fh, 4096)) !== false) {
                $argv = explode("=", $line);
                
                if(count($argv) > 1)
                {
                    if($argv[0] == "rate")
                       $this->rate = $argv[1];
                   
                    if($argv[0] == "start")
                       $this->start = $argv[1];
                   
                    if($argv[0] == "stop")
                       $this->stop = $argv[1]; 
                   
                    if($argv[0] == "on")
                       $this->on = $argv[1];   
                   
                    if($argv[0] == "rest")
                       $this->rest = $argv[1];
                }
        }
        
        fclose($fh);
    }
    
    function store($rate, $start, $stop, $on, $rest)
    {
        $fh = fopen($this->config_file, "w") or die("can't open file for writing");
        fwrite($fh, "rate=" . $rate . "\n");
        fwrite($fh, "start=" . $start . "\n");
        fwrite($fh, "stop=" . $stop . "\n");
        fwrite($fh, "on=" . $on . "\n");
        fwrite($fh, "rest=" . $rest . "\n");
        
        $this->rate = $rate;
        $this->start = $start;
        $this->stop = $stop;
        $this->on = $on;
        $this->rest = $rest;
        
        fclose($fh);
    }
    
}
?>
