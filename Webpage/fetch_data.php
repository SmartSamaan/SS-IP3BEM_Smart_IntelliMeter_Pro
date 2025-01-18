<?php
// fetch_data.php

$servername = "localhost";
$username = "root"; // Replace with your DB username
$password = "test"; // Replace with your DB password
$dbname = "energy_data"; // Replace with your DB name

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) { 
    die("Connection failed: " . $conn->connect_error);
}

// Query the latest data from the database
$sql = "SELECT phase, voltage, current, active_power, reactive_power, apparent_power, energy
    FROM metering_data
    WHERE timestamp = (SELECT MAX(timestamp) FROM metering_data WHERE phase = metering_data.phase)
    ORDER BY phase ASC";
$result = $conn->query($sql);

$data = [];
if ($result->num_rows > 0) {
    while ($row = $result->fetch_assoc()) {
		$row['energy'] = number_format($row['energy'], 3); // Format energy to 3 decimal places
        $data[] = $row;
    }
}

// Return data as JSON
header('Content-Type: application/json');
echo json_encode($data);
$conn->close();
