#!/usr/bin/ruby1.8

# ctrl.rb
#
# Copyright (C) 2012 Jacob Bower
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

class MCU
	@@day_map = [:invalid, :sun, :mon, :tue, :wed, :thu, :fri, :sat]

	def initialize
		system("stty -F /dev/ttyUSB0 9600 -parenb -parodd cs8 hupcl -cstopb cread -clocal -crtscts -ignbrk -brkint -ignpar -parmrk -inpck -istrip -inlcr -igncr -icrnl -ixon -ixoff -iuclc -ixany -imaxbel -iutf8 -opost -olcuc -ocrnl -onlcr -onocr -onlret -ofill -ofdel nl0 cr0 tab0 bs0 vt0 ff0 -isig -icanon -iexten -echo -echoe -echok -echonl -noflsh -xcase -tostop -echoprt -echoctl -echoke")
		@stream = File.open("/dev/ttyUSB0", "w+")
	end

	def setHeatingOn(v)
		if v
			putc(0x02)
		else
			putc(0x01)
		end
	end

	def heatingOn?
		putc(0x03)
		return getc == 0 ? false : true
	end

	def setTime(day, minute)
		putc(0x04)
		putc(day)
		putc(0x05)
		putc(minute >> 8)
		putc(minute)
	end

	def getEntry(pos)
		r = {:pos => pos}
		putc(0x0A)
		putc(pos)
		r[:turn_on] = getc == 0 ? false : true;
		r[:day] = getc
		r[:minute] = getc << 8
		r[:minute] |= getc

		return r
	end

	def dumpEntries
		for i in 0..31
			print "#{i} : "
			entry = getEntry(i)
			if entry[:day] == 0
				print "Not active\n"
			else
				print "#{@@day_map[entry[:day]]} #{minuteToS(entry[:minute])} -> #{{false => "off", true => "on"}[entry[:turn_on]]}\n"
			end
		end
	end

	def clearEntry(pos)
		setEntry(pos, false, 0, 0)
	end

	def setEntry(pos, on_off, day, minute)
		putc(0x06)
		putc(pos)
		if on_off
			putc(1)
		else
			putc(0)
		end
		putc(minute >> 8)
		putc(minute)
		putc(day)
	end

	def setTimedOperationEnabled(v)
		if v
			putc(0x08)
		else
			putc(0x07)
		end
	end

	def timedOperationEnabled?
		putc(0x0B)
		return getc == 0 ? false : true
	end

	def readPINA
		putc(0x0C)
		return getc
	end

	def getTime
		r = {}
		putc(0x09)
		r[:day] = getc
		r[:minute] = getc << 8
		r[:minute] |= getc

		return r
	end

	def getTimeString
		time = getTime

		return "#{@@day_map[time[:day]]} #{minuteToS(time[:minute])}"
	end

	def close
		@stream.close
	end

private
	def putc(c)
		@stream.write("%c" % [c & 0xff])
		@stream.flush
	end

	def getc
		return @stream.read(1)[0]
	end

	def minuteToS(minute)
		return "%d:%.2d" % [minute / 60, minute % 60]
	end
end

mcu = MCU.new
begin
	case ARGV[0]
		when "readPINA"
			print "0x%.2x\n" % [mcu.readPINA]

		when "enableTimedOperation"
			mcu.setTimedOperationEnabled(true)
			print "Timed operation enabled: #{mcu.timedOperationEnabled?}\n"

		when "disableTimedOperation"
			mcu.setTimedOperationEnabled(false)
			print "Timed operation enabled: #{mcu.timedOperationEnabled?}\n"

		when "getTime"
			print "Current time: #{mcu.getTimeString}\n"

		when "syncTime"
			time = Time.now
			mcu.setTime(time.wday + 1, time.hour * 60 + time.min)
			mcu.setTimedOperationEnabled(true)
			print "Time is now: #{mcu.getTimeString}\n"
			print "Timed operation enabled: #{mcu.timedOperationEnabled?}\n"

		when "turnHeatingOn"
			mcu.setHeatingOn(true)

		when "turnHeatingOff"
			mcu.setHeatingOn(false)

		when "isHeatingOn"
			print "Heating on: #{mcu.heatingOn?}\n"

		when "dumpEntries"
			mcu.dumpEntries

		when "clearEntry"
			pos = ARGV[1].to_i
			mcu.clearEntry(pos)
			mcu.dumpEntries

		when "clearAllEntries"
			for i in 0..31
				mcu.clearEntry(i)
				sleep(1.0/32)
			end

		when "setEntry"
			pos = ARGV[1].to_i
			day_map = {
				"sun" => 1,
				"mon" => 2,
				"tue" => 3,
				"wed" => 4,
				"thu" => 5,
				"fri" => 6,
				"sat" => 7}
			day = day_map[ARGV[2]]
			if not day
				print "Invalid day: #{ARGV[2]}\n"
				exit 1
			end
			hour = ARGV[3].to_i
			minute = ARGV[4].to_i
			state_map = {
				"on" => true,
				"off" => false}
			state = state_map[ARGV[5]]
			mcu.setEntry(pos, state, day, hour * 60 + minute)

		else
			print <<USAGE
Usage:
	readPINA
	enableTimedOperation
	disableTimedOperation
	getTime
	syncTime
	turnHeatingOn
	turnHeatingOff
	isHeatingOn
	dumpEntries
	clearEntry <pos>
	clearAllEntries
	setEntry <pos> <sun|mon|tue|wed|thu|fri|sat> <hour> <minute> <on|off>
USAGE
	end
ensure
	mcu.close
end
