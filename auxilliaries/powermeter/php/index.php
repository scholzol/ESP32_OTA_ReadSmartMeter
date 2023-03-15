<?php

/*
 * 2017 - blog.404.at
 *
 * Simple summary and graphs for collected values from the powermeter.
 * 
 * */
define ( "DB_HOST", "homeAutomation" );
define ( "DB_USER", "homeAutomation" );
define ( "DB_PASS", "yourpasswordhere" );
define ( "DB_NAME", "homeAutomation" );



$dataset = isset($_GET['dataset']) ? $_GET['dataset'] : null;


$link = @mysqli_connect(DB_HOST, DB_USER, DB_PASS, DB_NAME);
if ( ! $link ) {
    die ( "Fehler " . mysqli_connect_errno() . " bei der Verbindung zur MySQL-DB: " . mysqli_connect_error() );
}


if (!empty($dataset)) {
	// Return JSON
	
	switch ($dataset) {
	
		case 'power':
		
			$data = array();
			$sql = "SELECT a.* FROM (SELECT id, UNIX_TIMESTAMP(dbTime) * 1000 as rawdate, power FROM homeAutomation.D0 ORDER BY dbTime DESC LIMIT 120) AS a ORDER BY rawdate ASC";
			$result = mysqli_query ( $link, $sql );
			if ( $result === FALSE ) die ("DB-query failed." . mysql_error());		
			while ( ($row = mysqli_fetch_assoc($result)) !== NULL ) {
				$data[] = array( "id" => $row['id'], "date" => $row['rawdate'], "value" => $row['power']);
			}
			print json_encode($data, JSON_NUMERIC_CHECK);
			break;
		
		case 'voltages':
		
			$data = array();
			$sql = "SELECT a.* FROM (SELECT id, UNIX_TIMESTAMP(dbTime) * 1000 as rawdate, U_L1, U_L2, U_L3 FROM homeAutomation.D0 ORDER BY dbTime DESC LIMIT 120) AS a ORDER BY rawdate ASC"; 
			$result = mysqli_query ( $link, $sql );
			if ( $result === FALSE ) die ("DB-query failed." . mysql_error());		
			while ( ($row = mysqli_fetch_assoc($result)) !== NULL ) {
				$data[] = array( "id" => $row['id'], "date" => $row['rawdate'], "value1" => $row['U_L1'],  "value2" => $row['U_L2'],  "value3" => $row['U_L3']);
			}
			print json_encode($data, JSON_NUMERIC_CHECK);
			break;
		
		case 'currentValues':
			
			$sql = "SELECT dbTime
					, electricityID
					, billingPeriod
					, activeFirmware
					, timeSwitchPrgmNo
					, `localTime`
					, localDate
					, round(sumPower1, 1) AS sumPower1
					, round(sumPower2, 1) AS sumPower2
					, round(sumPower3, 1) AS sumPower3
					, round(power, 2) AS power
					, round(U_L1, 1) AS U_L1
					, round(U_L2, 1) AS U_L2
					, round(U_L3, 1) AS U_L3
					, deviceNo
					, freqL1
					, freqL2
					, freqL3
			FROM homeAutomation.D0
			ORDER BY dbTime DESC
			LIMIT 1"; 
			$result = mysqli_query ( $link, $sql );
			if ( $result === FALSE ) die ("DB-query failed." . mysql_error());		
			$row = mysqli_fetch_assoc($result);
			print json_encode($row);
			break;
		
	}
	
}


else {
	// Display page content
	
?>

<!DOCTYPE html>
<html lang="de">
	<head>
		<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
		<title>powermeter</title>
		
		<script type="text/javascript" language="javascript" src="https://code.jquery.com/jquery-3.1.1.min.js"></script>
		
		<script type="text/javascript" language="javascript" src="https://www.amcharts.com/lib/3/amcharts.js"></script>
		<script type="text/javascript" language="javascript" src="https://www.amcharts.com/lib/3/serial.js"></script>
		<script type="text/javascript" language="javascript" src="https://www.amcharts.com/lib/3/plugins/export/export.min.js"></script>
		<script type="text/javascript" language="javascript" src="https://www.amcharts.com/lib/3/themes/light.js"></script>
		<script type="text/javascript" language="javascript" src="https://www.amcharts.com/lib/3/plugins/dataloader/dataloader.min.js"></script>
		
		<link rel="stylesheet" href="https://www.amcharts.com/lib/3/plugins/export/export.css" type="text/css" media="all" />
		
		<script src="res/odometer/odometer.min.js"></script>
		<script type="text/javascript" language="javascript">
			window.odometerOptions = {
	  			format: '(,ddd).dd' // Change how digit groups are formatted, and how many digits are shown after the decimal point
			};
		</script>
			
		<link rel="stylesheet" href="res/odometer/odometer-theme-car.css" />
		
		
		<style>
		
			body {
				padding: 0 2em;
				font-family: Montserrat, sans-serif;
				-webkit-font-smoothing: antialiased;
				text-rendering: optimizeLegibility;
				color: #444;
				background: steelblue;
			}
			table {
				width: 100%;
				/* max-width: 600px; */
				/*border-collapse: collapse;*/
				border-radius: .4em;
				border: 1px solid #38678f;
				margin: 50px auto;
				background: white;
			}		
			tr {
    			border-color: lighten(#34495E, 50%);
    			border-top: 1px solid #ddd;
    			border-bottom: 1px solid #ddd;
			}
			th {
				background: steelblue;
				border-radius: .4em;
				padding: 5px;
				font-weight: lighter;
				text-shadow: 0 1px 0 #38678f;
				color: white;
				border: 1px solid #38678f;
				box-shadow: inset 0px 1px 2px #568ebd;
				transition: all 0.2s;
			}
			td.label {
				text-align:right;
			}
			.serialchart {
				width	: 100%;
				height	: 350px;
				background: #fff;
			}

		</style>

	</head>
	<body>
		<div id="currentValues">
			<table>
				<tr>
					<th colspan="10">IEC 62056-21 Meter Values</th>
				</tr>
				<tr>
					<td class="label">Counter 1 (no rate):</td>
					<td><span id="sumPower1" class="odometer">0</span> kW/h</td>
					<td class="label">Voltage L1:</td>
					<td><span id="U_L1">-</span> V</td>
					<td class="label">Frequency L1:</td>
					<td><span id="freqL1">-</span> Hz</td>
					<td class="label">Local time:</td>
					<td><span id="localTime">-</span></td>
					<td class="label">Time switch prgram number:</td>
					<td><span id="timeSwitchPrgmNo">-</span></td>
				</tr>
				<tr>
					<td class="label">Counter 2 (rate 1):</td>
					<td><span id="sumPower2" class="odometer">0</span> kW/h</td>
					<td class="label">Voltage L2:</td>
					<td><span id="U_L2">-</span> V</td>
					<td class="label">Frequency L2:</td>
					<td><span id="freqL2">-</span> Hz</td>
					<td class="label">Local date:</td>
					<td><span id="localDate">-</span></td>
					<td class="label">Counter Type:</td>
					<td><span id="electricityID">-</span></td>
				</tr>
				<tr>
					<td class="label">Counter 3 (rate 2):</td>
					<td><span id="sumPower3" class="odometer">0</span> kW/h</td>
					<td class="label">Voltage L3:</td>
					<td><span id="U_L3">-</span> V</td>
					<td class="label">Frequency L3:</td>
					<td><span id="freqL3">-</span> Hz</td>
					<td class="label">Billing period:</td>
					<td><span id="billingPeriod">-</span></td>
					<td class="label">Active Firmware:</td>
					<td><span id="activeFirmware">-</span></td>
				</tr>
				<tr>
					<td class="label">Current load:</td>
					<td><span id="power" class="odometer">0</span> kW</td>
					<td colspan="6"></td>
					<td class="label">Device #:</td>
					<td><span id="deviceNo">-</span></td>
				</tr>
			</table>
		</div>
		<div class="serialchart" id="powerChart"></div>
		<br>
		<div class="serialchart" id="voltagesChart"></div>
		
		<script type="text/javascript" language="javascript">

			(function($)
			{
			    $(document).ready(function()
			    {
			       $.getJSON("<?php print $_SERVER['PHP_SELF'];?>?dataset=currentValues", function(data){
					    $.each(data, function (index, value) {
					    	$('#' + index).text(value);
					    });
					});
			        var refreshId = setInterval(function()
			        {
				       $.getJSON("<?php print $_SERVER['PHP_SELF'];?>?dataset=currentValues", function(data){
						    $.each(data, function (index, value) {
						    	$('#' + index).text(value);
						    });
						});
			        }, 5000);
			    });
			})(jQuery);

			var chart = AmCharts.makeChart("powerChart", {
			    "type": "serial",
				"dataLoader": {
    			  "url": "<?php print $_SERVER['PHP_SELF'];?>?dataset=power",
    			  "format": "json",
			      "async": true,
			      "showErrors": true,
			      "showCurtain": true,
			      "reload": 30
				},
				"titles": [{
					"text": "Live power usage"
				}],
			    //"theme": "light",
			    "marginRight": 80,
			    "autoMarginOffset": 20,
			    "marginTop": 7,
			    "valueAxes": [{
			        "axisAlpha": 0.2,
			        "dashLength": 1,
			        "position": "left",
			        "ignoreAxisWidth": true
			    }],
			    "mouseWheelZoomEnabled": true,
			    "valueAxes": [{
       		       "gridAlpha": 0.07,
			       "title": "Power [kW]"
			    }],
			    "graphs": [{
			        "id": "g1",
			        "balloonText": "[[value]]",
			        "bullet": "round",
			        "bulletBorderAlpha": 1,
			        "bulletColor": "#FFFFFF",
			        "hideBulletsCount": 50,
			        "title": "red line",
			        "valueField": "value",
			        "useLineColorForBulletBorder": true
			    }],
			    "chartScrollbar": {
			        "autoGridCount": true,
			        "graph": "g1",
			        "scrollbarHeight": 40
			    },
			    "chartCursor": {
					"categoryBalloonDateFormat":"DD.MM.YYYY JJ:NN",
					"limitToGraph":"g1"
			    },
			    "categoryField": "date",
			    "categoryAxis": {
			        "parseDates": true,
			        "minPeriod": "mm",
			        "axisColor": "#DADADA",
			        "dashLength": 1,
			        "minorGridEnabled": true
			    },
			    "export": {
			        "enabled": true
			    }
			});
			
			
			var chart = AmCharts.makeChart("voltagesChart", {
			    "type": "serial",
				"dataLoader": {
    			  "url": "<?php print $_SERVER['PHP_SELF'];?>?dataset=voltages",
    			  "format": "json",
			      "async": true,
			      "showErrors": true,
			      "showCurtain": true,
			      "reload": 30
				},
				"titles": [{
						"text": "Live Line Voltages"
				}],
				"theme": "light",
			    "marginRight": 80,
			    "autoMarginOffset": 20,
			    "marginTop": 7,
			    "valueAxes": [{
			        "axisAlpha": 0.2,
			        "dashLength": 1,
			        "position": "left",
			        "ignoreAxisWidth": true
			    }],
				"legend": {
    				"useGraphSettings": true,
    				"align": "center"
  				},
			    "mouseWheelZoomEnabled": true,
			    "valueAxes": [{
       		       "gridAlpha": 0.07,
			       "title": "Voltage [V]"
			    }],
			    "graphs": [{
			        "id": "g1",
			        "balloonText": "L1: [[value1]]V",
			        "bullet": "round",
			        "bulletBorderAlpha": 1,
			        "bulletColor": "#FFFFFF",
			        "hideBulletsCount": 50,
			        "title": "L1",
			        "valueField": "value1",
			        "useLineColorForBulletBorder": true
			    },
			    {
			        "id": "g2",
			        "balloonText": "L2: [[value2]]V",
			        "bullet": "round",
			        "bulletBorderAlpha": 1,
			        "bulletColor": "#FFFFFF",
			        "hideBulletsCount": 50,
			        "title": "L2",
			        "valueField": "value2",
			        "useLineColorForBulletBorder": true
			    },
			    {
			        "id": "g3",
			        "balloonText": "L3: [[value3]]V",
			        "bullet": "round",
			        "bulletBorderAlpha": 1,
			        "bulletColor": "#FFFFFF",
			        "hideBulletsCount": 50,
			        "title": "L3",
			        "valueField": "value3",
			        "useLineColorForBulletBorder": true
			    } 
			    ],
			    "chartScrollbar": {
			        "autoGridCount": true,
			        "graph": "g1",
			        "scrollbarHeight": 40
			    },
			    "chartCursor": {
					"categoryBalloonDateFormat":"DD.MM.YYYY JJ:NN",
					"limitToGraph":"g1"
			    },
			    "categoryField": "date",
			    "categoryAxis": {
			        "parseDates": true,
			        "minPeriod": "mm",
			        "axisColor": "#DADADA",
			        "dashLength": 1,
			        "minorGridEnabled": true
			    },
			    "export": {
			        "enabled": true
			    }
			});
			
		</script>
	</body>
</html>
<?php

}


mysqli_close ( $link );

?>