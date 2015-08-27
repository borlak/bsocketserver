<?php

$exec_location = "/home/qserver/";
$exec_name = "qserver";

$options = getopt("a:p::l::v::m::");

if(!isset($options["a"])) {
	usage();
	exit;
}

if(isset($options["m"]))
	$emails = $options["m"];
else
	$emails = false;

$executable = $exec_location.$exec_name." -a{$options["a"]}"
	.(isset($options["p"]) ? " -p{$options["p"]}" : "")
	.(isset($options["v"]) ? " -v{$options["v"]}" : "")
	.(isset($options["l"]) ? " > {$options["l"]}" : " > /dev/null");

// check if the daemon is already running... counting this one there will be 2
$daemon_running = (int) `ps aux | grep qserverd.php | grep -v "grep" | wc -l`;
if($daemon_running >= 2) {
	exit("Daemon already running, exiting...");
}

$bad_response = 0;
$bin_position = 0;
$last_bin_position = 0;
$server_starting = false;
$starting_count = 0;
while(1) {
	$server_running = (int) `ps aux | grep qserver | grep -v "grep" | grep -v "qserverd" | wc -l`;
	
	if($server_starting) {
		if($starting_count++ > 5) {
			$subject = "!! ALERT !! Socket Server {$_SERVER["HOSTNAME"]} failed to restart, trying again...";
			$message = "Server hasn't restarted in 5 seconds, trying again...\n";
			echo $message;
			startServer();
			mailPeeps($subject, $message);
		}
	} else if($server_running === 0) {
		echo "Server not running, starting executable: $executable\n";
		startServer();
	} else {
		$server_starting = false;
		echo "Server already running, checking if responding...\n";
		if(checkServer() === false) {
			echo "SERVER FAILED TO RESPOND!  #bad responses [$bad_response]\n";
			$bad_response++;
		} else {
			echo "Server responded with bin position $bin_position, last position $last_bin_position\n";
			$last_bin_position = $bin_position;
			if($bad_response > 0)
				$bad_response--;
		}
		
		if($bad_response > 3) {
			$subject = "!! ALERT !! Socket Server {$_SERVER["HOSTNAME"]} is being restarted!";
			$message = "MORE THAN THREE BAD RESPONSES IN A ROW, RESTARTING SERVER!\n";
			echo $message;
			startServer();

			mailPeeps($subject, $message);
		}
	}
	sleep(1);
}

function startServer() {
	global $executable, $server_starting;
	$server_starting = true;
	exec("killall qserver");
	exec("killall curl");
	exec($executable);
}

function checkServer() {
	global $last_bin_position, $bin_position;
	
	$errno = false;
	$errstr = false;
	$fp = fsockopen("localhost", 80, $errno, $errstr, 1);
	$last_bin_position = $bin_position;
	$bin_position = false;
	if (!$fp) {
		echo "fsockopen error: $errstr ($errno)\n";
		return false;
	} else {
		$out = "GET /QStats1979 HTTP/1.1\r\n";
		$out .= "Connection: Close\r\n\r\n";
		fwrite($fp, $out);
		while (!feof($fp)) {
			$str = fgets($fp, 128);
			if(($pos = strpos($str, "BIN_POSITION")) !== false) {
				$bin_position = (int)substr($str, $pos+strlen("BIN_POSITION")+1);
				break;
			}
		}
		fclose($fp);
	}
	
	return true;
}

function mailPeeps($subject, $message) {
	global $emails, $options;
	
	if($emails !== false) {
		$message .= "<br>\nServer: {$_SERVER["HOSTNAME"]}<br>\n".print_r($options,true);
		mail($emails, $subject, $message);
	}
}

function usage() {
	echo "Usage:
   -a <API IP/base url>
   -l <file to log to, full path>
   -p <http port> [default 80]
   -v <verbose output/debugging>
   -vv <really verbose>
   -vvv <annoyingly verbose>
   -m <email addresses separated by comma>\n";
}

?>
