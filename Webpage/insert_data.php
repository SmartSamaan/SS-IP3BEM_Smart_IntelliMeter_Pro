<?php
$servername = "localhost";
$username = "root"; // Replace with your username
$password = "test"; // Replace with your password
$dbname = "energy_data"; // Replace with your database name

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

// Enable Event Scheduler
$sqlEnableScheduler = "SET GLOBAL event_scheduler = ON";
if (!$conn->query($sqlEnableScheduler)) {
    die("Failed to enable Event Scheduler: " . $conn->error);
}

// Create Event for Data Retention Policy
$sqlCreateEvent = "
CREATE EVENT IF NOT EXISTS delete_old_data
ON SCHEDULE EVERY 1 MINUTE
DO
DELETE FROM metering_data WHERE timestamp < NOW() - INTERVAL 5 MINUTE;
";
if (!$conn->query($sqlCreateEvent)) {
    die("Failed to create retention policy event: " . $conn->error);
}

// Fetch phase data
$data = [];
for ($i = 1; $i <= 3; $i++) {
    $data[] = [
        'voltage' => isset($_POST["phase{$i}_voltage"]) ? (float)$_POST["phase{$i}_voltage"] : 0.0,
        'current' => isset($_POST["phase{$i}_current"]) ? (float)$_POST["phase{$i}_current"] : 0.0,
        'active_power' => isset($_POST["phase{$i}_active_power"]) ? (float)$_POST["phase{$i}_active_power"] : 0.0,
        'reactive_power' => isset($_POST["phase{$i}_reactive_power"]) ? (float)$_POST["phase{$i}_reactive_power"] : 0.0,
        'apparent_power' => isset($_POST["phase{$i}_apparent_power"]) ? (float)$_POST["phase{$i}_apparent_power"] : 0.0,
        'energy' => isset($_POST["phase{$i}_energy"]) ? (float)$_POST["phase{$i}_energy"] : 0.0,
    ];
}

// Insert data into the database
foreach ($data as $phase => $values) {
    $phaseNo = $phase + 1;
    $sql = "INSERT INTO metering_data (phase, voltage, current, active_power, reactive_power, apparent_power, energy, timestamp)
            VALUES (
                '$phaseNo',
                '{$values['voltage']}',
                '{$values['current']}',
                '{$values['active_power']}',
                '{$values['reactive_power']}',
                '{$values['apparent_power']}',
                '{$values['energy']}',
                NOW()
            )";

    if (!$conn->query($sql)) {
        echo "Error: " . $sql . "<br>" . $conn->error;
    }
}

echo "Data inserted successfully";

// Close connection
$conn->close();
?>
