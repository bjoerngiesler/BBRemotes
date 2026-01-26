#include <limits.h> // for ULONG_MAX
#include <inttypes.h> // for uint64_t format string
#include <vector>

#include "BBRMXBProtocol.h"
#include "BBRTypes.h"

// ACTION PLAN
// 1. Remove all bb Subsystem dependencies - CHECK
// 2. Make compile-able - CHECK
// 3. make work - CHECK
// 4. Remove non-API mode stuff

using namespace bb;
using namespace bb::rmt;

static std::vector<unsigned int> baudRatesToTry = { 230400, 115200, 9600, 57600, 19200, 28800, 38400, 76800 }; // start with 115200, then try 9600

bb::rmt::MXBProtocol::MXBProtocol() {
	uart_ = &Serial1;
	debug_ = (MXBProtocol::DebugFlags)(DEBUG_PROTOCOL);
	timeout_ = 1000;
	atmode_ = false;
	atmode_millis_ = 0;
	atmode_timeout_ = 10000;
	currentBPS_ = 0;
	memset(packetBuf_, 0, sizeof(packetBuf_));
	packetBufPos_ = 0;
	apiMode_ = false;
	initialized_ = false;
}

void bb::rmt::MXBProtocol::setChannel(uint16_t channel) {
	chan_ = channel;
}

void bb::rmt::MXBProtocol::setPAN(uint16_t pan) {
	pan_ = pan;
}

bool MXBProtocol::init(const std::string& nodeName, uint16_t chan, uint16_t pan, HardwareSerial *uart) {
	if(initialized_) {
		printf("Already initialized\n");
		return true;
	}

	if(uart == nullptr) {
		printf("UART is nullptr -- that won't work\n");
		return false;
	}
	
	uart_ = uart;
	nodeName_ = nodeName;

	// empty uart
	while(uart_->available()) uart_->read();
	printf("Initializing transmitter!\n");

	currentBPS_ = 0;
	if(currentBPS_ == 0) {
		printf("Auto-detecting BPS: ");
		
		for(size_t i=0; i<baudRatesToTry.size(); i++) {
			printf("%d... ", baudRatesToTry[i]); 
			
			delay(100);
			uart_->begin(baudRatesToTry[i]);
			
			if(enterATModeIfNecessary() == true) {
				printf("Success.\n");
				currentBPS_ = baudRatesToTry[i];
				break; 
			}
		}

		if(currentBPS_ == 0) {
			printf("failed.\n");
			return false;
		}
	} else {
		printf("Using %dbps.", currentBPS_);
		uart_->begin(currentBPS_);
		if(enterATModeIfNecessary() != true) {
			return false;
		}
	}

	String addrH = sendStringAndWaitForResponse("ATSH"); addrH.trim();
	String addrL = sendStringAndWaitForResponse("ATSL"); addrL.trim();
	String fw = sendStringAndWaitForResponse("ATVR"); fw.trim();
	if(addrH == "" || addrL == "" || fw == "") return false;

	hwAddress_.fromXBeeAddress(strtol(addrH.c_str(), 0, 16), strtol(addrL.c_str(), 0, 16));
	printf("Found XBee at address: %s:%s (0x%lx:%lx) Firmware version: %s\n", addrH.c_str(), addrL.c_str(), hwAddress_.addrHi(), hwAddress_.addrLo(), fw.c_str());

	// request command timeout
	String retval = sendStringAndWaitForResponse("ATCT"); 
	if(retval != "") {
		atmode_timeout_ = strtol(retval.c_str(), 0, 16) * 100;
	}

	// perform energy detect
	retval = sendStringAndWaitForResponse("ATED"); 
	if(retval != "") {
		printf("Energy detect: %s\n", retval.c_str());
	}

	// FIXME Things to try for range:
	// Play with MAC mode (MM - retries, ACKs, Digi header)
	// Play with Clear Channel Assessment setting (CA)
	// Play with energy detect, and possibly auto-channel association
	// Play with transmit power (PL)

	// MAC mode: 0 is Digi mode w/ACKs, 1 802.15.4 no ACKS, 2 802.15.4 with ACKS, 3 is Digi mode no ACKS
	// We probably want Digi mode with ACKs; FIXME.
	retval = sendStringAndWaitForResponse("ATMM=3");
	if(retval != "OK") {
		printf("MM=3 response: %s\n", retval.c_str());
	}


	retval = sendStringAndWaitForResponse("ATRR");
	if(retval != "OK") {
		printf("RR response: %s\n", retval.c_str());
	}

	if(setConnectionInfo(chan, pan, true) != true) {
		printf("Setting connection info failed.\n");
		return false;
	}

	// check output power
	retval = sendStringAndWaitForResponse("ATPP");; 
	if(retval != "") {
		printf("Output power: %s\n", retval.c_str());
	}

	chan_ = chan;
	pan_ = pan;

	if(nodeName.size() != 0) {
		String str = String("ATNI") + nodeName.c_str();
		if(sendStringAndWaitForOK(str) == false) {
			printf("Error setting name \"%s\"\n", nodeName.c_str());
			return false;
		}
	} else {
		printf("Zero-length name, not setting\n");
	} 

	if(sendStringAndWaitForOK("ATNT=64") == false) {
		printf("Error setting node discovery timeout\n");
		return false;
	} 

	bool changedBPS = false;

	int targetBPS = 230400;
	if(currentBPS_ != targetBPS) {
		printf("Changing BPS from current %d to %d...\n", currentBPS_, targetBPS);
		if(changeBPSTo(targetBPS, true) != true) {
			printf("Setting BPS failed.\n"); 
			currentBPS_ = 0;
			return false;
		} else {
			printf("Setting BPS to %d successful.\n", targetBPS); 
			changedBPS = true;
		}
	}

	if(sendStringAndWaitForOK("ATWR") == false) {
		printf("Couldn't write config!\n");
		return false;
	}

	// we have changed the BPS successfully?
	if(changedBPS) {
		printf("Closing and reopening serial at %dbps\n", targetBPS);
		leaveATMode();
		uart_->end();
		uart_->begin(targetBPS);
		enterATModeIfNecessary();
		if(sendStringAndWaitForOK("AT") == false) return false;
		currentBPS_ = targetBPS;
	}

	setAPIMode(true);

	leaveATMode();

	initialized_ = true;

	return true;
}

bool MXBProtocol::step() {
	int packetsHandled = 0;
	while(available()) {
		if(apiMode_) {
			NodeAddr srcAddr;
			uint8_t rssi;
			MPacket packet;
			if(receiveAPIMode(srcAddr, rssi, packet) == false) {
				//printf("receiveAPIMode(): Failure\n");
				continue;
			}
			//printf("Received packet from %lx:%lx type %d\n", srcAddr.addrHi(), srcAddr.addrLo(), packet.type);
			MProtocol::incomingPacket(srcAddr, packet);
			packetsHandled++;
		} else {
			printf("Stuff available but not in API mode\n");
		}
	}

	return MProtocol::step();
}


bool MXBProtocol::setAPIMode(bool onoff) {
	if(onoff == true) {
		if(apiMode_ == true) {
			return false;
		}

		enterATModeIfNecessary();
		if(sendStringAndWaitForOK("ATAP=2") == true) {
			apiMode_ = true;
			leaveATMode();
			return true;
		} else {
			leaveATMode();
		}
	} else {
		if(apiMode_ == false) {
			printf("Not in API mode\n");
			return false;			
		}

		uint32_t arg = 0;
		if(sendAPIModeATCommand(1, "AP", arg) == true) {
			apiMode_ = false;
			return true;
		}
	}
	return false;
}

bool MXBProtocol::enterATModeIfNecessary() {
	debug_ = DEBUG_PROTOCOL;
	if(isInATMode()) {
		return true;
	}

	int numDiscardedBytes = 0;

	for(int timeout = 0; timeout < 1000; timeout++) {
		if(uart_->available()) {
			printf("discarding ");
			while(uart_->available())  {
				uart_->read();
				printf(".");
				numDiscardedBytes++;
			}
		}
		delay(1);
	}

	uart_->write("+++");
	delay(1000);

	bool success = false;

	for(int timeout = 0; timeout < 1200 && !success; timeout++) {
		unsigned char c;

		while(uart_->available()) {
			c = uart_->read();
			if(c == 'O') {
				if(!uart_->available()) delay(1);
				if(!uart_->available()) continue;
				c = uart_->read();
				if(c == 'K') {
					if(!uart_->available()) delay(1);
					if(!uart_->available()) continue;
					c = uart_->read();
					if(c == '\r') {
						success = true;
						break;
					} else {
						numDiscardedBytes++;
					}
				} else {
					numDiscardedBytes++;
				}
			} else {
				numDiscardedBytes++;
			}
		}

		if(!success) delay(1);
	}


	if(success) {
		if(numDiscardedBytes) {
			printf(" discarded %d bytes while entering AT mode...", numDiscardedBytes);
		}
		atmode_millis_ = millis();
		atmode_ = true;
				
		return true;
	}

	printf("no response entering AT mode. ");
	return false;
}

bool MXBProtocol::isInATMode() {
	if(atmode_ == false) return false;
	unsigned long m = millis();
	if(atmode_millis_ < m) {
		if(m - atmode_millis_ < atmode_timeout_) return true;
	} else {
		if(ULONG_MAX - m + atmode_millis_ < atmode_timeout_) return true;
	}
	atmode_ = false;
	return false;
}

bool MXBProtocol::leaveATMode() {
	if(!isInATMode()) {
		return true;
	}
	if(debug_ & DEBUG_PROTOCOL) printf("Sending ATCN to leave AT Mode\n");
	if(sendStringAndWaitForOK("ATCN") == true) {
		if(debug_ & DEBUG_PROTOCOL) printf("Successfully left AT Mode\n");
		atmode_ = false;
		delay(1100);
		return true;
	}

	if(debug_ & DEBUG_PROTOCOL) printf("no response to ATCN\n");
	return false;
}

bool MXBProtocol::changeBPSTo(uint32_t bps, bool stayInAT) {
	uint32_t paramVal;
	switch(bps) {
	case 1200:   paramVal = 0x0; break;
	case 2400:   paramVal = 0x1; break;
	case 4800:   paramVal = 0x2; break;
	case 9600:   paramVal = 0x3; break;
	case 19200:  paramVal = 0x4; break;
	case 38400:  paramVal = 0x5; break;
	case 57600:  paramVal = 0x6; break;
	case 115200: paramVal = 0x7; break;
	case 230400: paramVal = 0x8; break;
	case 460800: paramVal = 0x9; break;
	case 921600: paramVal = 0xa; break;
	default:     paramVal = bps; break;
	}

	printf("Sending ATBD=%x\n", paramVal);
	if(sendStringAndWaitForOK(String("ATBD=")+String(paramVal, HEX)) == false) return false;

	if(stayInAT == false) leaveATMode();
	currentBPS_ = bps;

	return true;
}

bool MXBProtocol::setConnectionInfo(uint8_t chan, uint16_t pan, bool stayInAT) {
	chan_ = chan;
	pan_ = pan;

	enterATModeIfNecessary();

	if(sendStringAndWaitForOK(String("ATCH=")+String(chan, HEX)) == false) {
		if(debug_ & DEBUG_PROTOCOL) printf("ERROR: Setting channel to 0x%x failed!\n", chan);
		if(!stayInAT) leaveATMode();
		return false;
	} 
	if(sendStringAndWaitForOK(String("ATID=")+String(pan, HEX)) == false) {
		if(debug_ & DEBUG_PROTOCOL) printf("ERROR: Setting PAN to 0x%x failed!\n", pan);
		if(!stayInAT) leaveATMode();
		return false;
	}
	if(sendStringAndWaitForOK("ATMY=FFFE") == false) {
		if(debug_ & DEBUG_PROTOCOL) printf("ERROR: Setting MY to 0xfffe failed!\n");
		if(!stayInAT) leaveATMode();
		return false;
	}

	if(!stayInAT) leaveATMode();

	return true;
}

bool MXBProtocol::getConnectionInfo(uint8_t& chan, uint16_t& pan, bool stayInAT) {
	enterATModeIfNecessary();

	String retval;

	retval = sendStringAndWaitForResponse("ATCH");
	if(retval == "") return false;
	chan = strtol(retval.c_str(), 0, 16);
	delay(1);

	retval = sendStringAndWaitForResponse("ATID");
	if(retval == "") return false;
	pan = strtol(retval.c_str(), 0, 16);
	delay(1);

	if(!stayInAT) leaveATMode();
	return true;
}

void MXBProtocol::setDebugFlags(DebugFlags debug) {
	debug_ = debug;
}


bool MXBProtocol::sendBroadcastPacket(MPacket& packet, bool bumpSeqnum) {
	NodeAddr addr;
	addr.fromXBeeAddress(0,0xffff);
	return sendPacket(addr, packet, bumpSeqnum);
}

bool MXBProtocol::sendPacket(const NodeAddr& dest, MPacket& packet, bool bumpS) {
	uint8_t buf[11+sizeof(packet)];
	bool ack = false;

	packet.seqnum = seqnum_;
	packet.source = source_;
	packet.crc = packet.calculateCRC();

	buf[0] = 0x0;  // transmit request - 64bit frame. This is deprecated.
	buf[1] = 0x0;  // no response frame
	buf[2] = (dest.addrHi() >> 24) & 0xff;
	buf[3] = (dest.addrHi() >> 16) & 0xff;
	buf[4] = (dest.addrHi() >> 8) & 0xff;
	buf[5] = dest.addrHi() & 0xff;
	buf[6] = (dest.addrLo() >> 24) & 0xff;
	buf[7] = (dest.addrLo() >> 16) & 0xff;
	buf[8] = (dest.addrLo() >> 8) & 0xff;
	buf[9] = dest.addrLo() & 0xff;
	if(ack == false) {
		buf[10] = 1;						// disable ACK
	} else {
		buf[10] = 0;						// Use default value of TO
	}

	memcpy(&(buf[11]), &packet, sizeof(packet));
	//printf("Sending %d bytes with 0x%x to 0x%lx:%lx - ", sizeof(packet)+11, buf[0], dest.addrHi(), dest.addrLo());
	//for(int i=2; i<=9; i++) printf("%02x", buf[i]);
	//printf("\n");

	APIFrame frame(buf, 11+sizeof(packet));
	if(send(frame) == true) {
		if(bumpS) bumpSeqnum();
		return true;
	}
	return false;
}

bool MXBProtocol::available() {
	if(isInATMode()) leaveATMode();
	return uart_->available();
}

bool MXBProtocol::receiveAPIMode(NodeAddr& srcAddr, uint8_t& rssi, MPacket& packet) {
	if(!apiMode_) {
		printf("Wrong mode.\n");
		return false;
	} 

	APIFrame frame;

	if(receive(frame) != true) {
		//printf("receive(): failure\n");
		return false;
	}

	//printf("Received frame of length %d, first char 0x%x\n", frame.length(), frame.data()[0]);

	if(frame.is16BitRXPacket()) { // 16bit address frame
		printf("16bit address packet!\n");
		if(frame.length() != sizeof(MPacket) + 5) {
			printf("Invalid API Mode 16bit addr packet size %d (expected %d)\n", frame.length(), sizeof(MPacket) + 5);
			return false;
		}
		srcAddr.fromXBeeAddress(0, uint32_t(frame.data()[1] << 8) | frame.data()[2]);
		rssi = frame.data()[3];
		memcpy(&packet, &(frame.data()[5]), sizeof(packet));
	} else if(frame.is64BitRXPacket()) { // 64bit address frame
		if(frame.length() != sizeof(MPacket) + 11) {
			printf("Invalid API Mode 64bit addr packet size %d (expected %d -- MPacket is %d)\n", frame.length(), sizeof(MPacket) + 11, sizeof(MPacket));
			return false;
		}
		srcAddr.fromXBeeAddress((uint32_t(frame.data()[1]) << 24) | (uint32_t(frame.data()[2]) << 16) |
				                (uint32_t(frame.data()[3]) <<  8) | uint32_t(frame.data()[4]),
					        	(uint32_t(frame.data()[5]) << 24) | (uint32_t(frame.data()[6]) << 16) |
				         		(uint32_t(frame.data()[7]) <<  8) | uint32_t(frame.data()[8]));
		rssi = frame.data()[9];
		memcpy(&packet, &(frame.data()[11]), sizeof(packet));
#if 0
		printf("Source addr: 0x%0lx:%0lx \n", srcAddr.addrHi, srcAddr.addrLo);
		printf("Source: %d Type: %d Seqnum: %d\n", packet.source, packet.type, packet.seqnum);
#endif
	} else {
		printf("Unknown frame type 0x%x\n", frame.data()[0]);
		return false;
	}

	uint8_t crc = packet.calculateCRC();
	if(packet.calculateCRC() != packet.crc) {
		if(debug_ & DEBUG_XBEE_COMM) {
			printf("Error: Wrong CRC 0x%x, expected 0x%x\n", crc, packet.crc);
		}
		return false;
	}

	return true;
}

bool MXBProtocol::waitForPacket(std::function<bool(const MPacket&, const NodeAddr&)> fn, 
                                NodeAddr& addr, MPacket& packet, 
                                bool handleOthers, float timeout) {
    bool retval = false;

    while(true) {
		if(!available()) {
			timeout -= .01;
	        if(timeout < 0) break;
    	    delay(10);
		}

		uint8_t rssi;
		if(receiveAPIMode(addr, rssi, packet) == false) {
			//printf("available, but Receive() failed\n");
			continue;
		}
		
		if(fn(packet, addr) == true) {
			retval = true;
		} else if(handleOthers == true) {
			incomingPacket(addr, packet);
        }
        if(retval == true) return true;
    }
    return false;	
}


String MXBProtocol::sendStringAndWaitForResponse(const String& str, int predelay, bool cr) {
  	if(debug_ & DEBUG_XBEE_COMM) {
    	printf("Sending \"%s\"...", str.c_str());
  	}

    uart_->print(str);
  	if(cr) {
  		uart_->print("\r");
  	}


    if(predelay > 0) delay(predelay);

    String retval;
    if(readString(retval)) {
    	if(debug_ & DEBUG_XBEE_COMM) {
    		printf(retval.c_str());
    		printf(" ");
    	}
    	return retval;
    }

    if(debug_ & DEBUG_PROTOCOL) {
    	printf("Nothing.\n");
    }
    return "";
}
  
bool MXBProtocol::sendStringAndWaitForOK(const String& str, int predelay, bool cr) {
	printf("Sending \"%s\"\n", str.c_str());
  	String result = sendStringAndWaitForResponse(str, predelay, cr);
  	if(result.equals("OK\r")) return true;
      
  	if(debug_ & DEBUG_PROTOCOL) {
    	printf("Expected \"OK\", got \"%s\"... \n", result.c_str());
  	}
  return false;
}

bool MXBProtocol::readString(String& str, unsigned char terminator) {
	while(true) {
		int to = 0;
		while(!uart_->available()) {
			delay(1);
			to++;
			if(debug_ & DEBUG_XBEE_COMM) printf(".");
			if(to >= timeout_) {
				if(debug_ & DEBUG_PROTOCOL) printf("Timeout!\n");
				return false;
			}
		}
		unsigned char c = (unsigned char)uart_->read();
		str += (char)c;
		if(c == terminator) return true;
	}
}

bool MXBProtocol::sendAPIModeATCommand(uint8_t frameID, const char* cmd, uint32_t& argument, bool request) {
	if(apiMode_ == false) return false;
	if(strlen(cmd) != 2) return false;

	uint8_t len = request ? 4 : 5;
	uint8_t buf[5];
	buf[0] = 0x08; // AT command
	buf[1] = frameID;
	buf[2] = cmd[0];
	buf[3] = cmd[1];
	if(!request)
		buf[4] = uint8_t(argument);

	APIFrame frame(buf, len);
	
	if(send(frame) != true) return false;

	bool received = false;
	for(int i=0; i<10 && received == false; i++) {
		if(uart_->available()) {
			if(receive(frame) != true) return false;
			else if(!frame.isATResponse()) {
				printf("Ignoring frame of type 0x%x while waiting for AT response\n", frame.data()[0]);
				continue;
			} 
			received = true;
		} else delay(1);
	}
	if(!received) {
		printf("Timed out waiting for frame reply\n");
		return false;
	}

	uint16_t length = frame.length();
	const uint8_t *data = frame.data();

	printf("API reply (%d bytes): ", length);
	for(unsigned int i=0; i<length; i++) printf("%x ", data[i]);
	printf("\n");

	if(length < 5 || (request == true && length < 6)) return false;
	if(data[0] != 0x88 || data[1] != frameID || data[2] != cmd[0] || data[3] != cmd[1]) return false;

	switch(data[4]) {
	case 0:
		printf("API Response: OK.\n");
		break;
	case 1:
		printf("API Response: ERROR.\n");
		return false;
		break;
	case 2:
		printf("API Response: Invalid Command.\n");
		return false;
		break;
	case 3:
		printf("API Response: Invalid Parameter.\n");
		return false;
		break;
	default:
		printf("API Response: Unknown error.\n");
		return false;
		break;
	}

	if(request) {
		argument = 0;
		for(unsigned int i=5; i<5+sizeof(argument)&&i<length; i++) {
			argument <<= 8;
			argument |= data[i];
		}
	}
	return true;
}

static uint32_t total = 0;

//#define MEMDEBUG

static uint8_t *allocBlock(uint32_t size, const char *loc="unknown") {
#if defined(MEMDEBUG)
	bb::printf("Allocing block size %d from \"%s\"...", size, loc);
#endif
	uint8_t *block = new uint8_t[size];
	total += size;
#if defined(MEMDEBUG)
	bb::printf("result: 0x%x, total %d.\n", block, total);
#endif
	return block;
}

static void freeBlock(uint8_t *block, uint32_t size, const char *loc="unknown") {
	delete block;
	total -= size;
#if defined(MEMDEBUG)
	bb::printf("Deleted block 0x%x, size %d, total %d from \"%s\"\n", block, size, total, loc);
#endif
}

MXBProtocol::APIFrame::APIFrame() {
	data_ = NULL;
	length_ = 0;
	calcChecksum();
}

MXBProtocol::APIFrame::APIFrame(const uint8_t *data, uint16_t length) {
	//data_ = new uint8_t[length];
	data_ = allocBlock(length, "APIFrame(const uint8_t*,uint16_t)");
	length_ = length;
	memcpy(data_, data, length);
	calcChecksum();
}

MXBProtocol::APIFrame::APIFrame(const MXBProtocol::APIFrame& other) {
	//printf("APIFrame 2\n");
	if(other.data_ != NULL) {
		length_ = other.length_;
		data_ = allocBlock(length_, "APIFrame(const APIFrame&)");
//		data_ = new uint8_t[length_];
		memcpy(data_, other.data_, length_);
	} else {
		data_ = NULL;
		length_ = 0;
	}
	checksum_ = other.checksum_;
}

MXBProtocol::APIFrame::APIFrame(uint16_t length) {
	//printf("APIFrame 1\n");
	//data_ = new uint8_t[length];
	data_ = allocBlock(length, "APIFrame(uint16_t)");
	length_ = length;
}

MXBProtocol::APIFrame& MXBProtocol::APIFrame::operator=(const APIFrame& other) {
	//printf("operator=\n");
	if(other.data_ == NULL) {           	// they don't have data
		if(data_ != NULL) {
			freeBlock(data_, length_, "operator=(const APIFrame&) 1");
			//delete data_; 	// ...but we do - delete
		}
		data_ = NULL;						// now we don't have data either
		length_ = 0;
	} else {  								// they do have data
		if(data_ != NULL) {					// and so do we
			if(length_ != other.length_) {	// but it's of a different size
				//delete data_;				// reallocate
				freeBlock(data_, length_, "operator=(const APIFrame&) 2");
				length_ = other.length_;
				//data_ = new uint8_t[length_];
				data_ = allocBlock(length_, "operator=(const APIFrame&) 3");
			}
			memcpy(data_, other.data_, length_); // and copy
		} else {							// and we don't
			length_ = other.length_;		
			//data_ = new uint8_t[length_];
			data_ = allocBlock(length_, "operator=(const APIFrame&) 4");
			memcpy(data_, other.data_, length_);
		}
	}
	checksum_ = other.checksum_;

	return *this;
}

MXBProtocol::APIFrame::~APIFrame() {
	if(data_ != NULL) {
		//delete data_;
		freeBlock(data_, length_, "~APIFrame");
	}
}

void MXBProtocol::APIFrame::calcChecksum() {
	checksum_ = 0;
	if(data_ != NULL) {
		for(uint16_t i=0; i<length_; i++) {
			checksum_ += data_[i];
		}
	}

	checksum_ = 0xff - (checksum_ & 0xff);
}

bool MXBProtocol::APIFrame::verifyChecksum(uint8_t checksum) {
	uint16_t sum = checksum;
	if(data_ != NULL) {
		for(uint16_t i=0; i<length_; i++) {
			sum += data_[i];
		}
	}
	if((sum & 0xff) == 0xff) return true;
	return false;
}

MXBProtocol::APIFrame MXBProtocol::APIFrame::atRequest(uint8_t frameID, uint16_t command) {
	APIFrame frame(4);

	frame.data_[0] = ATREQUEST; // local AT request
	frame.data_[1] = frameID;
	frame.data_[2] = (command>>8) & 0xff;
	frame.data_[3] = command & 0xff;
	frame.calcChecksum();

	return frame;
}

bool MXBProtocol::APIFrame::isATRequest() {
	return data_[0] == ATREQUEST && length_ > 4;
}

bool MXBProtocol::APIFrame::isATResponse() {
	return data_[0] == ATRESPONSE && length_ > 4;
}

bool MXBProtocol::APIFrame::is16BitRXPacket() {
	return data_[0] == RECEIVE16BIT && length_ > 5;
}

bool MXBProtocol::APIFrame::is64BitRXPacket() {
	return data_[0] == RECEIVE64BIT && length_ > 11;
}

bool MXBProtocol::APIFrame::unpackATResponse(uint8_t &frameID, uint16_t &command, uint8_t &status, uint8_t** data, uint16_t &length) {
	if(data_[0] != ATRESPONSE) {
		return false;
	} 
	if(length_ < 5) {
		printf("AT response too short (%d, need min 5)\n", length_);
		return false;
	}

	frameID = data_[1];
	command = (data_[2]<<8)|data_[3];
	status = data_[4];

	if(length_ > 5) {
		*data = &(data_[5]);
		length = length_-5;
	} else {
		*data = NULL;
		length = 0;
	}

	return true;
}

static inline int writeEscapedByte(HardwareSerial* uart, uint8_t byte) {
	int sent = 0;
	if(byte == 0x7d || byte == 0x7e || byte == 0x11 || byte == 0x13) {
		sent += uart->write(0x7d);
		sent += uart->write(byte ^ 0x20);
	} else {
		sent += uart->write(byte);
	}
	return sent;
}

static inline uint8_t readEscapedByte(HardwareSerial* uart) {
	uint8_t byte = uart->read();
	if(byte != 0x7d) return byte;
	return (uart->read()^0x20);
}

bool MXBProtocol::send(const APIFrame& frame) {
	if(apiMode_ == false) return false;
	
	uint16_t length = frame.length();
	const uint8_t *data = frame.data();

	uint8_t lengthMSB = (length >> 8) & 0xff;
	uint8_t lengthLSB = length & 0xff;
	
	uart_->write(0x7e); // start delimiter
	writeEscapedByte(uart_, lengthMSB);
	writeEscapedByte(uart_, lengthLSB);

#if 0
	printf("Writing %d bytes: ", length);
	for(uint16_t i=0; i<length; i++) {
		printf("%x ", data[i]);
	}
	printf("\n");
	printf("Writing checksum %x\n", frame.checksum());
#endif

	for(uint16_t i=0; i<length; i++) {
		writeEscapedByte(uart_, data[i]);
	}
	writeEscapedByte(uart_, frame.checksum());

	return true;
}


static bool waitfor(std::function<bool(void)> fn, uint8_t timeout) {
	while(fn() == false && timeout>0) {
		delayMicroseconds(1);
		timeout = timeout - 1;
	}
	//printf("remaining: %d\n", timeout);
	if(timeout > 0) return true;
	return false;
}

bool MXBProtocol::receive(APIFrame& frame) {
	uint8_t byte = 0xff;
	unsigned long garbageBytes = 0;

	while(uart_->available()) {
		byte = uart_->read();
		if(byte == 0x7e) {
			if(garbageBytes != 0) {
				printf("Read %d garbage bytes\n", garbageBytes);
				garbageBytes = 0;
			}
			break;
		} else {
			garbageBytes++;
			if(garbageBytes > 1000) {
				printf("Read 1000 garbage bytes\n");
				garbageBytes = 0;
			}
		}
	}

	if(byte != 0x7e) {
		//printf("No start delimiter\n");
		return false;
	}
	//printf("Start delimiter found\n");

	//if(!uart_->available()) delayMicroseconds(200);
	//if(!uart_->available()) return false;	
	if(waitfor([this]()->bool {return uart_->available();}, 200) == false) return false;
	uint8_t lengthMSB = readEscapedByte(uart_);
	if(waitfor([this]()->bool {return uart_->available();}, 200) == false) return false;
	//if(!uart_->available()) delayMicroseconds(200);
	//if(!uart_->available()) return false;	
	uint8_t lengthLSB = readEscapedByte(uart_);

	if(lengthLSB == 0x7e || lengthMSB == 0x7e) {
		printf("Extra start delimiter found\n");
		return false;		
	}

	uint16_t length = (lengthMSB << 8) | lengthLSB;
	frame = APIFrame(length);
	uint8_t *buf = frame.data();
	for(uint16_t i=0; i<length; i++) {
		//printf("Reading byte %d of %d\n", i, length);
		if(waitfor([=]()->bool {return uart_->available();}, 200) == false) return false;
		//if(!uart_->available()) delayMicroseconds(200);
		buf[i] = readEscapedByte(uart_);
	}
	frame.calcChecksum();

	if(waitfor([=]()->bool {return uart_->available();}, 200) == false) return false;
	//if(!uart_->available()) delayMicroseconds(200);
	uint8_t checksum = readEscapedByte(uart_);

	if(frame.verifyChecksum(checksum) == false) {
		//printf("Checksum invalid - expected 0x%x, got 0x%x\n", frame.checksum(), checksum);
		return false;
	}

	return true;
}