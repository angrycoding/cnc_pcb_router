var FS = require('fs'),
	Path = require('path'),
	SerialPort = require('serialport'),
	CALLBACKS = {}, UID_TIME_LAST = 0, UID_TIME_DIFF = 0;

function message() {
	console.info.apply(console, ['[ MESSAGE ]'].concat(
		Array.prototype.slice.call(arguments)
	));
}

function uniqueId() {
	var partOne = new Date().getTime();
	if (partOne > UID_TIME_LAST) UID_TIME_DIFF = 0;
	else partOne += (++UID_TIME_DIFF);
	UID_TIME_LAST = partOne;
	return (partOne.toString(36) +
		(1 + Math.floor((Math.random()*32767))).toString(36) +
		(1 + Math.floor((Math.random()*32767))).toString(36)
	);
}

function forEachAsync(list, iterator, ret) {
	if (!(list instanceof Object)) return ret();
	var keys, key, length, index = 0;
	var i = -1, calls = 0, looping = false;
	if (list instanceof Array) {
		length = list.length;
	} else {
		keys = Object.keys(list);
		length = keys.length;
	}
	var resume = function() {
		calls += 1;
		if (looping) return;
		looping = true;
		while (calls > 0) {
			calls -= 1, i += 1;
			if (i === length) return ret();
			key = (keys ? keys[i] : i);
			iterator(list[key], function(stop) {
				if (stop === true) ret();
				else resume();
			}, key);
		}
		looping = false;
	};
	resume();
}


function connect(comName, gCodeFileName) {

	var serialPort = new SerialPort.SerialPort(comName, {
		baudrate: 9600,
		parser: SerialPort.parsers.readline("\n")
	});

	message('establishing connection...');

	serialPort.on('open', function() {

		message('waiting for device...');

		serialPort.on('data', function(callbackId) {
			callbackId = callbackId.toString().trim();
			message('RECEIVED', callbackId);
			if (CALLBACKS.hasOwnProperty(callbackId)) {
				CALLBACKS[callbackId]();
			}
		});

		function sendCommand(command, callback) {
			CALLBACKS[command.id] = callback;
			serialPort.write(JSON.stringify(command), function(error) {
				// if (error) {
				// 	console.info('resending', command);
				// 	sendCommand(command, callback);
				// }
			});
		}

		function closeConnection(error) {
			if (error) message('error:', error);
			message('closing connection...');
			serialPort.close(function() {
				process.exit(0);
			});
		}

		message('waiting for server...');
		var waitForConnection = setInterval(function() {
			sendCommand({id: uniqueId(), command: 'connect'}, function() {
				clearInterval(waitForConnection);
				FS.readFile(gCodeFileName, 'UTF-8', function(error, source) {
					if (error) return closeConnection(error);
					source = source.split(/[\n\r]+/);
					forEachAsync(source, function(gcode, ret) {
						gcode = /^([A-Z]+[0-9]+)\s*(.*)/.exec(gcode);
						if (!gcode) return ret();
						var command = {id: uniqueId(), command: gcode[1]};
						if (gcode[2]) gcode[2].replace(/([A-Z]+)(-?[0-9]+\.[0-9]+)/g, function() {
							command[arguments[1].toLowerCase()] = parseFloat(arguments[2]);
						});
						message('processing', command);
						sendCommand(command, ret);
					}, closeConnection);
				});
			});
		}, 500);

	});
}


message('checking arguments...');
var gCodeFileName = process.argv[2];
if (typeof gCodeFileName !== 'string') {
	message('missing required argument: input filename, quitting');
	process.exit(0);
}

message('looking for device...');
SerialPort.list(function(error, ports) {
	for (var c = 0; c < ports.length; c++) {
		if ((ports[c].manufacturer || '').indexOf('Arduino') !== -1) {
			message('found device on', ports[c].comName);
			return connect(ports[c].comName, Path.resolve(process.cwd(), gCodeFileName));
		}
	}
	message('device not found, quitting');
	process.exit(0);
});