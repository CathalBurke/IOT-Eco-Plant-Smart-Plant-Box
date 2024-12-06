// To create literal strings stored in flash memory enclose your HTML code between 
// F(R"=====( HTML code here )=====");
// If you have 1 reading then you probably have 2 literal strings
// If you have 2 readings then you probably have 3 literal strings etc.

const char* homePagePart1 = R"=====(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Eco Plant Smart Plant Box</title>
    <style>
        /* General styling for the webpage */
        body {
            font-family: 'Roboto', sans-serif;
            background-color: #87674e; /* Earthy brown background */
            margin: 0;
            padding: 0;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
        }

        .container {
            background-color: #83ca87;
            border-radius: 15px;
            box-shadow: 0px 4px 15px rgba(0, 0, 0, 0.1);
            padding: 30px;
            text-align: center;
            max-width: 600px;
            width: 100%;
            border: 1px solid #a5d6a7; /* Green border */
        }

        h1 {
            font-size: 2em;
            color: #4caf50;
            margin-bottom: 20px;
            text-transform: uppercase;
            letter-spacing: 2px;
            font-weight: 700;
        }

        table {
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
            background-color: #244826; /* Table background color */
            border-radius: 10px;
            overflow: hidden;
        }

        table th, table td {
            padding: 15px;
            text-align: center;
            border: 1px solid #388e3c; /* Darker green border */
        }

        table th {
            background-color: #388e3c; /* Button color for header */
            color: white;
            font-size: 1.2em;
            text-transform: uppercase;
        }

        table td {
            color: white;
            font-size: 1.1em;
            font-weight: bold;
        }

        .status {
            margin-top: 20px;
            padding: 15px;
            background-color: #4caf50;
            color: white;
            border-radius: 10px;
            font-size: 1.1em;
        }

        .status.error {
            background-color: #d32f2f;
        }

        /* Login box styling */
        #loginBox {
            position: fixed;
            top: 10px;
            left: 10px;
            background-color: #4caf50;
            padding: 12px;
            border-radius: 10px;
            color: white;
            width: 180px;
            box-shadow: 0px 4px 10px rgba(0, 0, 0, 0.2);
        }

        #loginBox input {
            width: 90%;
            padding: 8px;
            margin: 8px 0;
            border-radius: 5px;
            border: none;
        }

        #loginBox button {
            width: 100%;
            padding: 8px;
            background-color: #388e3c;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 1em;
        }

        #loginBox button:hover {
            background-color: #2e7d32;
        }

        /* Engineering Access table styling */
        #engineeringAccess {
            position: absolute;
            bottom: 20px;
            left: 20px;
            background-color: #81c784; /* Updated background color */
            padding: 15px;
            border-radius: 10px;
            color: white;
            display: none; /* Hidden by default */
            width: 200px;
        }

        #engineeringAccess table {
            width: 100%;
            color: white;
        }

        #engineeringAccess button {
            background-color: #388e3c; /* Updated button color */
            color: white;
            border: none;
            padding: 10px;
            font-size: 1em;
            border-radius: 5px;
            cursor: pointer;
            margin-top: 10px;
        }

        #engineeringAccess button:hover {
            background-color: #2e7d32;
        }

        .pump-status, .fan-status {
            font-weight: bold;
            font-size: 1.1em;
            margin-top: 10px;
        }
    </style>
</head>
<body>
    <!-- Login Box -->
    <div id="loginBox">
        <h2>Engineering Access</h2>
        <input type="text" id="username" placeholder="Username">
        <input type="password" id="password" placeholder="Password">
        <button onclick="login()">Login</button>
    </div>

    <!-- Main Container -->
    <div class="container">
        <h1>Sensor Data</h1>
        <table>
            <thead>
                <tr>
                    <th>Sensor</th>
                    <th>Value</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td>Temperature</td>
                    <td id="tempValue">--</td>
                </tr>
                <tr>
                    <td>Humidity</td>
                    <td id="humidityValue">--</td>
                </tr>
                <tr>
                    <td>Soil Moisture</td>
                    <td id="soilValue">--</td>
                </tr>
                <tr>
                    <td>Water Level</td>
                    <td id="waterLevelValue">--</td>
                </tr>
            </tbody>
        </table>
    </div>

    <!-- Engineering Access table -->
    <div id="engineeringAccess">
        <table>
            <tr>
                <td>Force Water</td>
                <td><button onmousedown="startPump()" onmouseup="stopPump()">Activate to pump</button></td>
            </tr>
            <tr>
                <td>Force Fan</td>
                <td><button onmousedown="startFan()" onmouseup="stopFan()">Activate to run fan</button></td>
            </tr>
        </table>
        <div class="pump-status" id="pumpStatus">Pump Status: OFF</div>
        <div class="fan-status" id="fanStatus">Fan Status: OFF</div>
    </div>

    <script>
        // Login function to validate credentials
        function login() {
            const username = document.getElementById('username').value;
            const password = document.getElementById('password').value;

            if (username === 'admin' && password === '1234') {
                alert("Login successful!");
                document.getElementById("engineeringAccess").style.display = "block";
                document.getElementById("loginBox").style.display = "none";
            } else {
                alert("Invalid credentials. Please try again.");
            }
        }

        // Function to manage Pump Status
        function startPump() {
            document.getElementById('pumpStatus').textContent = "Pumping";
            fetch('/start-pump')
                .catch(error => console.error('Error:', error));
        }

        function stopPump() {
            document.getElementById('pumpStatus').textContent = "Pump Status: OFF";
            fetch('/stop-pump')
                .catch(error => console.error('Error:', error));
        }

        // Function to manage Fan Status
        function startFan() {
            document.getElementById('fanStatus').textContent = "Fan Running";
            fetch('/start-fan')
                .catch(error => console.error('Error:', error));
        }

        function stopFan() {
            document.getElementById('fanStatus').textContent = "Fan Status: OFF";
            fetch('/stop-fan')
                .catch(error => console.error('Error:', error));
     
        }
    
        function fetchSensorData() {
        fetch('/data')
        .then(response => response.json())
        .then(data => {
            document.getElementById('tempValue').textContent = data.temperature + " °C";
            document.getElementById('humidityValue').textContent = data.humidity + " %";
            document.getElementById('soilValue').textContent = data.soil_moisture + " %";
            document.getElementById('waterLevelValue').textContent = data.water_level + " %";
        })
            .catch(error => console.error('Error fetching sensor data:', error));
         }

        // Call fetchSensorData every second
        setInterval(fetchSensorData, 1000);

    
    
    
    
    
    </script>
</body>
</html>
)=====";
