#!/usr/bin/env ruby
require 'packetfu'

##{incident_number}. ALERT: #{incident} is detected from #{source IP address} (#{protocol}) (#{payload})!
#Example:
#	1. ALERT: Someone looking for phpMyAdmin stuff is detected from 192.168.1.3 (HTTP) 
##		("GET /phpMyAdmin/scripts/setup.php HTTP/1.1")!
class WebLog
  def initialize(incident_count)
    @incident_count = incident_count
  end
  def raiseAlert(incidentType, sourceIP, protocol, payload)
    print "#{@incident_count += 1}. ALERT: #{incidentType} is detected from #{sourceIP}\
	 (#{protocol}) (#{payload})\n"
  end
end

def sniffDNSPacket()
	print "Starting sniff!\n"
	alerter = WebLog.new(0)
	pkt_array = PacketFu::Capture.new(:start => true, :iface => 'eth0', :promisc => true)
	caught = false
	count = 0
	finSeenIP =Array[]
	finCountIP = Array[]
	while caught==false do
		pkt_array.stream.each do |p|
			count+=1
			pkt = PacketFu::Packet.parse(p)
			if pkt.is_ip? and pkt.is_tcp?
				############# NULL SCAN #############
				if pkt.tcp_flags.urg == 0 and pkt.tcp_flags.ack == 0\
				   and pkt.tcp_flags.psh == 0 and pkt.tcp_flags.rst == 0\
				   and pkt.tcp_flags.syn == 0 and pkt.tcp_flags.fin == 0
					alerter.raiseAlert("NULL Scan",pkt.ip_saddr,
								"TCP",pkt.payload)
				end
				############# FIN SCAN ##############
				# For FIN attack, note that ONLY fin is set
				#  We propose that many (> 200) of these constitutes a scan
				if pkt.tcp_flags.urg == 0 and pkt.tcp_flags.ack == 0\
				   and pkt.tcp_flags.psh == 0 and pkt.tcp_flags.rst == 0\
				   and pkt.tcp_flags.syn == 0 and pkt.tcp_flags.fin == 1
					#Check if this IP has already sent ONLY FIN
					ipIndex = finSeenIP.index(pkt.ip_saddr)
					if !ipIndex.nil?
						#We've seen IP before, count to 200, then alert
						finCountIP[ipIndex] += 1
						if finCountIP[ipIndex] > 200
							alerter.raiseAlert("FIN Scan",
								pkt.ip_saddr,"TCP",pkt.payload)
						end
					else  #we haven't seen IP before, log it
						finSeenIP.push(pkt.ip_saddr)
						finCountIP.push(0)
					end
				end
				############# XMAS SCAN #############
				if pkt.tcp_flags.urg == 1 and pkt.tcp_flags.psh == 1\
					 and pkt.tcp_flags.fin == 1
					alerter.raiseAlert("Xmas Scan",pkt.ip_saddr,
								"TCP",pkt.payload)
				end
				######### OTHER NMAP SCAN #############
				if !pkt.payload.scan(/\x4E\x6D\x61\x70/).empty?
					alerter.raiseAlert("NMAP Scan",pkt.ip_saddr,
								"TCP",pkt.payload)
				end
				############# NIKTO SCAN #############
				if !pkt.payload.scan(/\x4E\x69\x6B\x74\x6F/).empty?
					alerter.raiseAlert("Nikto Scan",pkt.ip_saddr,
								"TCP",pkt.payload)
				end
				######### Credit Card  SCAN ############
				if (!pkt.payload.scan(/5\d{3}(\s|-)?\d{4}(\s|-)?\d{4}(\s|-)?\d{4}/) \
				 || !pkt.payload.scan(/6011(\s|-)?\d{4}(\s|-)?\d{4}(\s|-)?\d{4}/) \
				 || !pkt.payload.scan(/3\d{3}(\s|-)?\d{6}(\s|-)?\d{5}/))
					alerter.raiseAlert("Credit Card leaked in the clear",
								pkt.ip_saddr,"TCP", pkt.payload)
				end
			end
		end
	end
end

def ReadWebLog(logFile)
	alerter = WebLog.new(0)
	if !File.file?(logFile)
		puts "File does not exist: #{logFile}\n"
		exit 1
	end
	text=File.open(logFile).read
	text.gsub!(/\r\n?/, "\n")
	text.each_line do |line|
	        if line.downcase.include? "nmap"
			# line.split.drop(5) delivers data following IP&Date for payload
	                alerter.raiseAlert("NMAP", line.split[0], "TCP", line.split.drop(5))
		end
	        if line.downcase.include? "masscan"
	                alerter.raiseAlert("MASSCAN", line.split[0], "UDP/TCP/ICMP", line.split.drop(5))
		end
	        if line.downcase.include? "phpmyadmin"
	                alerter.raiseAlert("Reference to phpMyAdmin made", line.split[0],
						"HTTP", line.split.drop(5))
		end
	end
end
def PrintUsage()
	print "Usage: ruby alarm.rb [-r <web_server_log>]\n"
	exit(1)
end
def StartUp()
	if (ARGV.length == 0)
		sniffDNSPacket()
	elsif ((ARGV.length == 2) && (ARGV[0] == '-r'))
		ReadWebLog(ARGV[1])
	else
		PrintUsage()
	end
end
StartUp()
