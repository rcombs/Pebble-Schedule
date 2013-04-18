#! /usr/bin/env node
var exports = module.exports = function(events){
	var buf = new Buffer(38 * events.length);
	for(var i = 0; i < events.length; i++){
		buf.writeUInt16LE(events[i].flags || 0x00FF, 38 * i);
		buf.writeUInt16LE(events[i].startTime, 38 * i + 2);
		buf.writeUInt16LE(events[i].endTime, 38 * i + 4);
		var bytes = buf.write(events[i].name, 38 * i + 6, 31, "utf8");
		buf[38 * i + 6 + bytes] = 0x00;
	}
	return buf;
}
