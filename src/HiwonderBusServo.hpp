/*
 * This file is part of HiwonderRPI library
 * 
 * HiwonderRPI is free software: you can redistribute it and/or modify 
 * it under ther terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * HiwonderRPI is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License 
 * along with HiwonderRPI. If not, see <https://www.gnu.org/licenses/>.
 * 
 * Author: Adrian Maire escain (at) gmail.com
 */

#ifndef HIWONDER_RPI
#define HIWONDER_RPI

#include <array>
#include <cstdint>
#include <exception>
#include <functional>

#include <wiringPi.h>
#include <wiringSerial.h>

namespace HiwonderRpi
{
	
// Literals
constexpr uint16_t operator ""_uint16( unsigned long long v) { return static_cast<uint16_t>(v);}
constexpr int16_t operator ""_int16( unsigned long long v) { return static_cast<int16_t>(v);}
constexpr uint8_t operator ""_uint8( unsigned long long v) { return static_cast<uint8_t>(v);}
constexpr int8_t operator ""_int8( unsigned long long v) { return static_cast<int8_t>(v);}

// This class is header-only, for ease of usage.

/// This class represent a Hiwonder servo, and implement
/// basic UART communication from a Raspberry PI.
/// For convenience, command names are keep similar to the documentation, but
///     parameters are in a more user-friendly format than bytes.
///     Methods in this class and servo commands match 1 to 1.
class HiwonderBusServo
{
	using Buffer = std::array<uint8_t,10>;
	
public:
	struct MoveTime
	{
		uint16_t position;
		uint16_t time;
	};
	
	struct Limit
	{
		int16_t minLimit=0;
		int16_t maxLimit=1000;
	};
	
	enum class Mode: uint8_t
	{
		Servo = 0,
		Motor = 1
	};
	
	enum class LoadMode: uint8_t
	{
		Unload = 0,
		Load = 1
	};
	
	enum class PowerLed: uint8_t
	{
		Off = 1,
		On = 0
	};
	
	struct ModeRead
	{
		Mode mode=Mode::Servo;
		int16_t speed = 0;
	};
	
	struct LedError
	{
		bool overTemperature;
		bool overVoltage;
		bool stall;
	};
	
	/// Constructor, accept the servo ID. 
	/// Id=254 is the broadcast ID
	HiwonderBusServo( uint8_t id=254 );
	HiwonderBusServo( const HiwonderBusServo&& );
	/// Servo object can not be copied (UART access is unique)
	HiwonderBusServo( const HiwonderBusServo& ) = delete;
	HiwonderBusServo& operator=( const HiwonderBusServo&) = delete;
	
	virtual ~HiwonderBusServo();

	/// Immediately start moving the servo to the given position
	///     trying to reach target position in the given time (ms)
	/// @arg position: target absolute position in multiples of 0.24deg
	/// @arg time: time to reach the target position in ms (if too short, max-speed is used)
	void moveTimeWrite( int16_t position, uint16_t time=0);
	
	/// Read the values set by moveTimeWrite
	MoveTime moveTimeRead() const;
	
	/// Those functions aren't yet implemented in the servo?
		///
		void moveTimeWaitWrite( int16_t position, uint16_t time=0);
		///
		MoveTime moveTimeWaitRead() const;
		///
		void moveStart();
		///
		void moveStop();
	
	/// Set the ID of the servo to <newId>
	///     If the current servo ID is unknown, use broadcast constructor
	/// @arg newId: the new ID to be set.
	void idWrite(uint8_t newId);
	
	/// Read the id from the servo. This alway uses broadcast (why to read the id if you know it?)
	uint8_t idRead() const;
	
	/// The angle offset is an adjustment on the position (homing).
	/// This function set (volatile memory until servo reset) this angle adjustment.
	/// @arg angle: Absolute adjustment, in multiples of 0.24deg, 0 is the central position.
	void angleOffsetAdjust( int8_t angle );
	
	/// Return the current offset angle
	int8_t angleOffsetRead() const;
	
	/// Save permanently(over reset, in flash memory) the current offset in the servo.
	void angleOffsetWrite();
	
	/// Set (persistently over shutdown) angle limits; movements are limited to them.
	/// @arg minLimit: minimal angle in [0,999]
	/// @arg maxLimit: maximal angle in [minLimit+1, 1000]
	void angleLimitWrite( int16_t minLimit, int16_t maxLimit);
	
	/// Retrieve current angle limits
	Limit angleLimitRead() const;
	
	/// Set (persistently over shutdown) voltage limits; Outside, the servo will output no torque
	///     and the LED will blink for warnings (if configured/available).
	/// @arg minLimit: minimal voltage in [4500,11999]
	/// @arg maxLimit: maximal voltage in [minLimit+1, 12000]
	void vinLimitWrite( int16_t minLimit, int16_t maxLimit);
	
	/// Retrieve current voltage limits
	Limit vinLimitRead() const;
	
	/// Set (persistently over shutdown) temperature limits; Outside, the servo will output no torque
	///     and the LED will blink for warnings (if configured/available).
	/// @arg maxTemp: maximum temperature in [50, 100]
	void tempMaxLimitWrite( uint8_t maxTemp=85);
	
	/// Retrieve current max temperature limit
	uint8_t tempMaxLimitRead() const;
	
	/// Read the current servo temperature in deg celsius
	uint8_t tempRead() const;
	
	/// Read the input voltage to the servo, in mV
	uint16_t vinRead() const;
	
	/// Read the current servo position in multiple of 0.24 deg (1000 = 240deg)
	/// Note: the servo can be easily 0.5deg away of it command, that way it can be in negative angle.
	int16_t posRead() const;
	
	/// Set (volatile) the mode of the device: Servo or Motor (position or speed)
	/// In case of motor mode, the speed can be specified: 0=stopped, negative/positive for each direction.
	/// @arg mode: Servo or Motor
	/// @arg speed: In case of Motor-mdoe, the speed [-1000,1000]
	void servoOrMotorModeWrite( Mode mode=Mode::Servo, int16_t speed=0 );
	
	/// Read the Servo or Motor mode, and in case of motor, the speed (0 for servo).
	ModeRead servoOrMotorModeRead() const;
	
	/// Set the servo to "Unload":free-rotation (it will not apply torque to keep a position), or 
	/// "Load": normal mode, where the servo tries to hold a given position
	/// @arg loadMode: Load or Unload mode to set
	void loadOrUnloadWrite( LoadMode loadMode = LoadMode::Load );
	
	/// Retrieve the Load or Unload mode from the servo
	LoadMode loadOrUnloadRead() const;
	
	/// Set if the Power LED is always ON, or always OFF
	/// @arg powerLed: On or Off
	void ledCtrlWrite(PowerLed powerLed = PowerLed::On);
	
	/// Read if the Power LED is ON or OFF
	PowerLed ledCtrlRead() const;
	
	/// Set the different errors to be warned by the LED
	/// @arg overTemperature: if to warn over temperature with the LED
	/// @arg overVoltage: if to warn over voltage with the LED
	/// @arg stall: if to warn when the servo is locked/stall with the LED
	void ledErrorWrite( bool overTemperature=true, bool overVoltage=true, bool stall=true);
	
	/// Read the LED errors set
	LedError ledErrorRead() const;

private:
	
	/// Message prefix/frame header
	constexpr static uint8_t FrameHeader = 0x55;
	/// Used to pre-fill buffers before real data is set in
	constexpr static uint8_t _pholder = 0;
	
	/// return the lower byte of an uint16_t
	inline static uint8_t getLowByte( const uint16_t in);
	
	/// return the higher byte of an uint16_t
	inline static uint8_t getHighByte( const uint16_t in);
	
	/// Return the checksum for a given message
	inline static uint8_t checksum(const Buffer& buf);

	/// Send a buffer of data to the servo
	inline void sendBuf(const Buffer& buf) const;
	
	/// Get a message from the servo (this function is blocking).
	/// @throw runtime_error if the message does not arrive until timeout (< 1 ms)
	/// Timeout is a busy loop, avoiding long waiting of re-scheduling
	inline const Buffer& getMessage() const;
	
	/// Basic check on a message: 
	///    - If the checksum match
	///    - If the size of the message is the expected (expect at pos 3)
	///    - If the commandId is the expected
	/// Return false in case of error
	inline static bool checkMessage( const Buffer& buf, uint8_t commandId, size_t expectedSize);
	
	/// Set all variable elements in buf (Id, and checksum), and send the request, 
	///     then it read the result, check it validity and return the buffer.
	/// Used internally to reuse common code between all the xxxxREAD commmands
	/// @arg buf: Buffer of the request (id, and checksum are computed internally)
	/// @arg replySize: expected size of the reply (for checks).
	inline const Buffer& genericRead( Buffer& buf, uint8_t replySize ) const;

	// Access to the device
	int fd = -1;
	// Id of the servo
	int id = 1;
};




//*********************************************************
//                   IMPLEMENTATION
//*********************************************************

uint8_t HiwonderBusServo::getLowByte( const uint16_t in)
{
	return static_cast<uint8_t>(in);
}

uint8_t HiwonderBusServo::getHighByte( const uint16_t in)
{
	return static_cast<uint8_t>(in>>8);
}

uint8_t HiwonderBusServo::checksum(const Buffer& buf)
{
	uint16_t temp = 0;
	for (size_t i=2; i<buf[3]+2u; ++i)
	{
		temp += buf[i];
	}
	temp = ~temp;
	return getLowByte(temp);
}

void HiwonderBusServo::sendBuf(const Buffer& buf) const
{
	for (size_t i=0; i< buf[3]+3u; ++i)
	{
		serialPutchar(fd, buf[i]);
	}
}
	
const HiwonderBusServo::Buffer& HiwonderBusServo::getMessage() const
{
	static Buffer res;
	
	constexpr static size_t MaxBusyLoop = 20000;
	
	// To avoid timeout (too long), poll until we get enough bytes
	for(size_t i=0; i<MaxBusyLoop && serialDataAvail(fd)<4; ++i) continue; //noop

	
	if (serialDataAvail(fd)<4)
	{
		res[3]=res[2]=0;
		throw std::runtime_error("Unable to retrieve message header from servo");
		return res;
	}
	
	res[0] = serialGetchar(fd); //frame header 1
	res[1] = serialGetchar(fd); //frame header 2
	res[2] = serialGetchar(fd); //servo id
	res[3] = serialGetchar(fd); //size
	
	for(size_t i=0; i<MaxBusyLoop && serialDataAvail(fd)<res[3]-1; ++i) continue; //noop
	
	if (serialDataAvail(fd)<res[3]-1)
	{
		res[3]=res[2]=0;
		throw std::runtime_error("Unable to retrieve message content from servo");
		return res;
	}
	
	for (size_t i=0; i<res[3]-1u; ++i)
	{
		res[i+4] = serialGetchar(fd);
	}
	
	return res;
}
	
	
bool HiwonderBusServo::checkMessage( const Buffer& buf, uint8_t commandId, size_t expectedSize)
{
	if (buf[3] != expectedSize || 
			buf[4] != commandId || 
			buf[expectedSize+2] != checksum(buf))
	{
		return false;
	}
	return true;
}

HiwonderBusServo::HiwonderBusServo(uint8_t id): id(id)
{
	fd = serialOpen("/dev/ttyAMA0", 115200);
	auto setupResult = wiringPiSetup();
	if (0>fd || -1==setupResult)
	{
		throw std::runtime_error("Unable to setup UART device.");
	}
}

HiwonderBusServo::~HiwonderBusServo()
{
	serialClose(fd);
}

const HiwonderBusServo::Buffer& HiwonderBusServo::genericRead( Buffer& buf, uint8_t replySize ) const
{
	buf[2] = id;
	buf[buf[3]+2] = checksum(buf);
	
	serialFlush(fd);
	sendBuf(buf);
	
	// Read result
	const Buffer& res= getMessage();
	
	if (!checkMessage(res, buf[4], replySize))
	{
		throw std::runtime_error("Corrupted message received");
	}
	
	return res;
}

void HiwonderBusServo::moveTimeWrite( int16_t position, uint16_t time)
{
	constexpr static uint8_t MoveTimeWriteId = 1;
	constexpr static uint8_t MoveTimeWriteSize = 7;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		MoveTimeWriteSize,
		MoveTimeWriteId,
		_pholder,
		_pholder,
		_pholder,
		_pholder,
		_pholder
	};
	
	if (position<0) position=0;
	if (position>1000) position=1000;
	
	buf[2] = id;
	buf[5] = getLowByte(position);
	buf[6] = getHighByte(position);
	buf[7] = getLowByte(time);
	buf[8] = getHighByte(time);
	buf[9] = checksum(buf);
	
	sendBuf(buf);
}

HiwonderBusServo::MoveTime HiwonderBusServo::moveTimeRead() const
{
	constexpr static uint8_t MoveTimeReadId = 2;
	constexpr static uint8_t MoveTimeReadSize = 3;
	constexpr static uint8_t MoveTimeReplySize = 7;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		MoveTimeReadSize,
		MoveTimeReadId,
		_pholder
	};
	
	const Buffer& resultBuf = genericRead(buf, MoveTimeReplySize);
	
	MoveTime result;
	result.position = resultBuf[5]+(resultBuf[6]<<8);
	result.time = resultBuf[7]+(resultBuf[8]<<8);
	return result;
}

void HiwonderBusServo::moveTimeWaitWrite( int16_t position, uint16_t time)
{
	constexpr static uint8_t MoveTimeWaitWriteId = 7;
	constexpr static uint8_t MoveTimeWaitWriteSize = 7;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		MoveTimeWaitWriteSize,
		MoveTimeWaitWriteId,
		_pholder,
		_pholder,
		_pholder,
		_pholder,
		_pholder
	};
	
	if (position<0) position=0;
	if (position>1000) position=1000;
	
	buf[2] = id;
	buf[5] = getLowByte(position);
	buf[6] = getHighByte(position);
	buf[7] = getLowByte(time);
	buf[8] = getHighByte(time);
	buf[9] = checksum(buf);
	
	sendBuf(buf);
}

HiwonderBusServo::MoveTime HiwonderBusServo::moveTimeWaitRead() const
{
	constexpr static uint8_t MoveTimeWaitReadId = 8;
	constexpr static uint8_t MoveTimeWaitReadSize = 3;
	constexpr static uint8_t MoveTimeWaitReplySize = 7;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		MoveTimeWaitReadSize,
		MoveTimeWaitReadId,
		_pholder
	};
	
	const Buffer& resultBuf = genericRead(buf, MoveTimeWaitReplySize);
	
	MoveTime result;
	result.position = resultBuf[5]+(resultBuf[6]<<8);
	result.time = resultBuf[7]+(resultBuf[8]<<8);
	return result;
}

void HiwonderBusServo::moveStart()
{
	constexpr static uint8_t MoveStartId = 11;
	constexpr static uint8_t MoveStartSize = 3;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		MoveStartSize,
		MoveStartId,
		_pholder
	};
	buf[5] = checksum(buf);
	
	sendBuf(buf);
}

void HiwonderBusServo::moveStop()
{
	constexpr static uint8_t MoveStopId = 12;
	constexpr static uint8_t MoveStopSize = 3;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		MoveStopSize,
		MoveStopId,
		_pholder
	};
	buf[5] = checksum(buf);
	
	sendBuf(buf);
}

void HiwonderBusServo::idWrite(uint8_t newId)
{
	constexpr static uint8_t IdWriteId = 13;
	constexpr static uint8_t IdWriteSize = 4;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		IdWriteSize,
		IdWriteId,
		_pholder,
		_pholder
	};
	
	buf[2] = id;
	buf[5] = newId;
	buf[6] = checksum(buf);
	
	sendBuf(buf);
}

uint8_t HiwonderBusServo::idRead() const
{
	constexpr static uint8_t idReadId = 14;
	constexpr static uint8_t idReadSize = 3;
	constexpr static uint8_t idReplySize = 4;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		idReadSize,
		idReadId,
		_pholder
	};
	
	buf[2] = 254;
	buf[buf[3]+2] = checksum(buf);
	
	serialFlush(fd);
	sendBuf(buf);
	
	// Read result
	const Buffer& res= getMessage();
	if (!checkMessage(res, buf[4], idReplySize))
	{
		throw std::runtime_error("Corrupted message received");
	}
	
	return res[5];
}

void HiwonderBusServo::angleOffsetAdjust( int8_t angleDelta )
{
	constexpr static uint8_t AngleOffsetAdjustId = 17;
	constexpr static uint8_t AngleOffsetAdjustSize = 4;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		AngleOffsetAdjustSize,
		AngleOffsetAdjustId,
		_pholder,
		_pholder
	};
	
	buf[2] = id;
	buf[5] = static_cast<uint8_t>(angleDelta);
	buf[6] = checksum(buf);
	
	sendBuf(buf);
}

void HiwonderBusServo::angleOffsetWrite()
{
	constexpr static uint8_t AngleOffsetWriteId = 18;
	constexpr static uint8_t AngleOffsetWriteSize = 3;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		AngleOffsetWriteSize,
		AngleOffsetWriteId,
		_pholder
	};
	
	buf[2] = id;
	buf[5] = checksum(buf);
	
	sendBuf(buf);
}

int8_t HiwonderBusServo::angleOffsetRead() const
{
	constexpr static uint8_t AngleOffsetReadId = 19;
	constexpr static uint8_t AngleOffsetReadSize = 3;
	constexpr static uint8_t AngleOffsetReplySize = 4;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		AngleOffsetReadSize,
		AngleOffsetReadId,
		_pholder
	};
	
	const Buffer& resultBuf = genericRead(buf, AngleOffsetReplySize);
	
	return static_cast<int8_t>(resultBuf[5]);
}

void HiwonderBusServo::angleLimitWrite( int16_t minLimit, int16_t maxLimit)
{
	constexpr static uint8_t AngleLimitWriteId = 20;
	constexpr static uint8_t AngleLimitWriteSize = 7;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		AngleLimitWriteSize,
		AngleLimitWriteId,
		_pholder,
		_pholder,
		_pholder,
		_pholder,
		_pholder
	};
	
	minLimit = std::max(minLimit,0_int16);
	minLimit = std::min(minLimit,999_int16); // Min cannot be over 999 (<1000)
	maxLimit = std::min(maxLimit,1000_int16);
	maxLimit = std::max(maxLimit,static_cast<int16_t>(minLimit+1_int16)); // Max>min
	
	buf[2] = id;
	buf[5] = getLowByte(minLimit);
	buf[6] = getHighByte(minLimit);
	buf[7] = getLowByte(maxLimit);
	buf[8] = getHighByte(maxLimit);
	buf[9] = checksum(buf);
	
	sendBuf(buf);
}

HiwonderBusServo::Limit HiwonderBusServo::angleLimitRead() const
{
	constexpr static uint8_t AngleLimitReadId = 21;
	constexpr static uint8_t AngleLimitReadSize = 3;
	constexpr static uint8_t AngleLimitReplySize = 7;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		AngleLimitReadSize,
		AngleLimitReadId,
		_pholder
	};
	
	const Buffer& resultBuf = genericRead(buf, AngleLimitReplySize);
	
	Limit limit;
	limit.minLimit = resultBuf[5]+(resultBuf[6]<<8);
	limit.maxLimit = resultBuf[7]+(resultBuf[8]<<8);
	return limit;
}

void HiwonderBusServo::vinLimitWrite( int16_t minLimit, int16_t maxLimit)
{
	constexpr static uint8_t VinLimitWriteId = 22;
	constexpr static uint8_t VinLimitWriteSize = 7;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		VinLimitWriteSize,
		VinLimitWriteId,
		_pholder,
		_pholder,
		_pholder,
		_pholder,
		_pholder
	};
	
	minLimit = std::max(minLimit,4500_int16);
	minLimit = std::min(minLimit,11999_int16);
	maxLimit = std::min(maxLimit,12000_int16);
	maxLimit = std::max(maxLimit,static_cast<int16_t>(minLimit+1_int16)); // Max>min
	
	buf[2] = id;
	buf[5] = getLowByte(minLimit);
	buf[6] = getHighByte(minLimit);
	buf[7] = getLowByte(maxLimit);
	buf[8] = getHighByte(maxLimit);
	buf[9] = checksum(buf);
	
	sendBuf(buf);
}	
	
HiwonderBusServo::Limit HiwonderBusServo::vinLimitRead() const
{
	constexpr static uint8_t VinLimitReadId = 23;
	constexpr static uint8_t VinLimitReadSize = 3;
	constexpr static uint8_t VinLimitReplySize = 7;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		VinLimitReadSize,
		VinLimitReadId,
		_pholder
	};
	
	const Buffer& resultBuf = genericRead(buf, VinLimitReplySize);
	
	Limit limit;
	limit.minLimit = resultBuf[5]+(resultBuf[6]<<8);
	limit.maxLimit = resultBuf[7]+(resultBuf[8]<<8);
	return limit;
}	
	
void HiwonderBusServo::tempMaxLimitWrite( uint8_t maxTemp)
{
	constexpr static uint8_t TempMaxLimitWriteId = 24;
	constexpr static uint8_t TempMaxLimitWriteSize = 4;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		TempMaxLimitWriteSize,
		TempMaxLimitWriteId,
		_pholder,
		_pholder
	};
	
	maxTemp = std::max(maxTemp,50_uint8);
	maxTemp = std::min(maxTemp,100_uint8);
	
	buf[2] = id;
	buf[5] = maxTemp;
	buf[6] = checksum(buf);
	
	sendBuf(buf);
}	

uint8_t HiwonderBusServo::tempMaxLimitRead() const
{
	constexpr static uint8_t TempMaxLimitReadId = 25;
	constexpr static uint8_t TempMaxLimitReadSize = 3;
	constexpr static uint8_t TempMaxLimitReplySize = 4;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		TempMaxLimitReadSize,
		TempMaxLimitReadId,
		_pholder
	};
	
	const Buffer& resultBuf = genericRead(buf, TempMaxLimitReplySize);
	
	return resultBuf[5];
}

uint8_t HiwonderBusServo::tempRead() const
{
	constexpr static uint8_t TempReadId = 26;
	constexpr static uint8_t TempReadSize = 3;
	constexpr static uint8_t TempReplySize = 4;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		TempReadSize,
		TempReadId,
		_pholder
	};
	
	const Buffer& resultBuf = genericRead(buf, TempReplySize);
	
	return resultBuf[5];
}
	
uint16_t HiwonderBusServo::vinRead() const
{
	constexpr static uint8_t VInReadId = 27;
	constexpr static uint8_t VInReadSize = 3;
	constexpr static uint8_t VInReplySize = 5;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		VInReadSize,
		VInReadId,
		_pholder
	};
	
	const Buffer& resultBuf = genericRead(buf, VInReplySize);
	
	return resultBuf[5]+(resultBuf[6]<<8);
}

int16_t HiwonderBusServo::posRead() const
{
	constexpr static uint8_t posReadId = 28;
	constexpr static uint8_t posReadSize = 3;
	constexpr static uint8_t posReplySize = 5;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		posReadSize,
		posReadId,
		_pholder
	};
	
	const Buffer& resultBuf = genericRead(buf, posReplySize);
	
	return resultBuf[5]+(resultBuf[6]<<8);
}

void HiwonderBusServo::servoOrMotorModeWrite( Mode mode, int16_t speed )
{
	constexpr static uint8_t ServoOrMotorModeWriteId = 29;
	constexpr static uint8_t ServoOrMotorModeWriteSize = 7;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		ServoOrMotorModeWriteSize,
		ServoOrMotorModeWriteId,
		_pholder,
		_pholder,
		_pholder,
		_pholder,
		_pholder
	};
	
	speed = std::max(speed,static_cast<int16_t>(-1000));
	speed = std::min(speed,1000_int16);
	
	buf[2] = id;
	buf[5] = static_cast<uint8_t>(mode);
	buf[6] = 0_uint8;
	buf[7] = getLowByte(speed);
	buf[8] = getHighByte(speed);
	buf[9] = checksum(buf);
	
	sendBuf(buf);
}
	
HiwonderBusServo::ModeRead HiwonderBusServo::servoOrMotorModeRead() const
{
	constexpr static uint8_t servoOrMotorModeReadId = 30;
	constexpr static uint8_t servoOrMotorModeReadSize = 3;
	constexpr static uint8_t servoOrMotorModeReplySize = 7;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		servoOrMotorModeReadSize,
		servoOrMotorModeReadId,
		_pholder
	};
	
	const Buffer& resultBuf = genericRead(buf, servoOrMotorModeReplySize);
	
	ModeRead result;
	result.mode = static_cast<Mode>(resultBuf[5]);
	result.speed = resultBuf[7]+(resultBuf[8]<<8);
	return result;
}

void HiwonderBusServo::loadOrUnloadWrite( LoadMode loadMode )
{
	constexpr static uint8_t LoadOrUnloadWriteId = 31;
	constexpr static uint8_t LoadOrUnloadWriteSize = 4;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		LoadOrUnloadWriteSize,
		LoadOrUnloadWriteId,
		_pholder,
		_pholder
	};
	
	buf[2] = id;
	buf[5] = static_cast<uint8_t>(loadMode);
	buf[6] = checksum(buf);
	
	sendBuf(buf);
}

HiwonderBusServo::LoadMode HiwonderBusServo::loadOrUnloadRead() const
{
	constexpr static uint8_t LoadOrUnloadReadId = 32;
	constexpr static uint8_t LoadOrUnloadReadSize = 3;
	constexpr static uint8_t LoadOrUnloadReplySize = 4;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		LoadOrUnloadReadSize,
		LoadOrUnloadReadId,
		_pholder
	};
	
	const Buffer& resultBuf = genericRead(buf, LoadOrUnloadReplySize);
	
	return static_cast<LoadMode>(resultBuf[5]);
}

void HiwonderBusServo::ledCtrlWrite(PowerLed powerLed)
{
	constexpr static uint8_t LedCtrlWriteId = 33;
	constexpr static uint8_t LedCtrlWriteSize = 4;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		LedCtrlWriteSize,
		LedCtrlWriteId,
		_pholder,
		_pholder
	};
	
	buf[2] = id;
	buf[5] = static_cast<uint8_t>(powerLed);
	buf[6] = checksum(buf);
	
	sendBuf(buf);
}
	
HiwonderBusServo::PowerLed HiwonderBusServo::ledCtrlRead() const
{
	constexpr static uint8_t LedCtrlReadId = 34;
	constexpr static uint8_t LedCtrlReadSize = 3;
	constexpr static uint8_t LedCtrlReplySize = 4;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		LedCtrlReadSize,
		LedCtrlReadId,
		_pholder
	};
	
	const Buffer& resultBuf = genericRead(buf, LedCtrlReplySize);
	
	return static_cast<PowerLed>(resultBuf[5]);
}

void HiwonderBusServo::ledErrorWrite( bool overTemperature, bool overVoltage, bool stall)
{
	constexpr static uint8_t LedErrorWriteId = 35;
	constexpr static uint8_t LedErrorWriteSize = 4;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		LedErrorWriteSize,
		LedErrorWriteId,
		_pholder,
		_pholder
	};
	
	buf[2] = id;
	buf[5] = static_cast<uint8_t>((overTemperature?0x1:0x0) + (overVoltage?0x2:0x0) + (stall?0x4:0x0));
	buf[6] = checksum(buf);
	
	sendBuf(buf);
}

HiwonderBusServo::LedError HiwonderBusServo::ledErrorRead() const
{
	constexpr static uint8_t LedErrorReadId = 36;
	constexpr static uint8_t LedErrorReadSize = 3;
	constexpr static uint8_t LedErrorReplySize = 4;
	
	static Buffer buf
	{
		FrameHeader, 
		FrameHeader,
		_pholder,
		LedErrorReadSize,
		LedErrorReadId,
		_pholder
	};
	
	const Buffer& resultBuf = genericRead(buf, LedErrorReplySize);
	
	LedError result;
	result.overTemperature = resultBuf[5] & 0x1;
	result.overVoltage = resultBuf[5] & 0x2;
	result.stall = resultBuf[5] & 0x4;
	return result;
}

}
#endif //HIWONDER_RPI



