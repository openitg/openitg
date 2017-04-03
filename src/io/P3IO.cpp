#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"	// for ssprintf, arraylen

#include "io/P3IO.h"
#include "arch/USB/USBDriver_Impl.h"

PSTRING P3IO::m_sInputError;
int P3IO::m_iInputErrorCount = 0;
static int connect_index = -1; //used for determining if we find the mouse
static uint8_t packet_sequence = 0;
static uint8_t packets_since_keepalive = 0;
// message timeout, in microseconds (so, 1 ms)
const int REQ_TIMEOUT = 1000;

uint8_t pchunk[3][4];
// p3io correlated pid/vid
//first set is what a chimera uses, second set is a real ddr io (from an hd black cab), the last set is actually a mouse I use for testing!

//has mouse debugger
//const uint16_t P3IO_VENDOR_ID[3] = { 0x0000, 0x1CCF, 0x046D};
//const uint16_t P3IO_PRODUCT_ID[3] = { 0x5731, 0x8008, 0xC077};


const uint16_t P3IO_VENDOR_ID[2] = { 0x0000, 0x1CCF };
const uint16_t P3IO_PRODUCT_ID[2] = { 0x5731, 0x8008 };
uint8_t p_light_payload[] = { 0, 0, 0, 0, 0 };
const unsigned NUM_P3IO_CHECKS_IDS = ARRAYLEN(P3IO_PRODUCT_ID);
int interrupt_ep = 0x83;
int bulk_write_to_ep = 0x02;
int bulk_read_from_ep = 0x81;
bool m_bConnected = false;

uint8_t P3IO::acio_request[256];
uint8_t P3IO::acio_response[256];
int bulk_reply_size=0;
bool baud_pass=false;
bool hdxb_ready=false;



bool P3IO::DeviceMatches(int iVID, int iPID)
{

	for (unsigned i = 0; i < NUM_P3IO_CHECKS_IDS; ++i)
		if (iVID == P3IO_VENDOR_ID[i] && iPID == P3IO_PRODUCT_ID[i])
			return true;	

	return false;
}

bool P3IO::Open()
{
	packet_sequence = 0;
	/* we don't really care which PID works, just if it does */
	for (unsigned i = 0; i < NUM_P3IO_CHECKS_IDS; ++i)
		if (OpenInternal(P3IO_VENDOR_ID[i], P3IO_PRODUCT_ID[i]))
		{
			LOG->Info("P3IO Driver:: Connected to index %d", i);
			connect_index = i;
			if (i == 2)
			{
				interrupt_ep = 0x81; //debug mouse
				LOG->Info("init p3io watch dog ");
				InitHDAndWatchDog();
			}
			else
			{
				LOG->Info("init p3io watch dog ");
				InitHDAndWatchDog();
			}
			m_bConnected = true;
			openHDXB();
			sendUnknownCommand();
			baud_pass=spamBaudCheck();
			
			// say the baud check passed anyway
			baud_pass=true;

			
			if(baud_pass)
			{
				initHDXB(); 
				pingHDXB();
				HDXBAllOnTest();
				hdxb_ready=true;
			}
			
			return m_bConnected;
		}
	m_bConnected = false;
	return m_bConnected;
}

void P3IO::HDXBAllOnTest()
{
	uint8_t all_on[0x0d]={
	0x00,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0xff,
	0x7f,
	0x7f,
	0x7f,
	0x7f,
	0x7f,
	0x7f
	};
	LOG->Info("HDXB ALL ON TEST:");
	writeHDXB(all_on, 0xd, HDXB_SET_LIGHTS);
}



//ok seriously what was I drinking / smoking here
//i dont even know how or why this works but it does
//basically we need to get all 3 4-byte chunks read in for this to be COMPLETE
//but the maximum payload size is 16 bytes?  so I guess theres a circumstance when it could poop out 16?
//using libusb1.x I could only get 4x chunks, and 0.x seems to give 12 byte chunks.. I just dont even know anymore
//someone smarter than me make this better please
bool P3IO::interruptRead(uint8_t* dataToQueue)
{
	m_iInputErrorCount = 0;
	int iExpected = 16; //end point size says max packet of 16, but with libusb1 I only ever got 12 in chunks of 4 every consequtive read
	uint8_t chunk[4][4] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	uint8_t tchunk[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	bool interruptChunkFlags[3] = { false, false, false }; //i guess only 12 is needed? lets work with that
	bool allChunks = false;
	int currChunk = 0;
	while (!allChunks)
	{
		int iResult = m_pDriver->InterruptRead(interrupt_ep, (char*)tchunk, iExpected, REQ_TIMEOUT);

		//LOG->Warn( "p3io read, returned %i: %s\n", iResult, m_pDriver->GetError() );
		if (iResult>0)
		{
			m_iInputErrorCount = 0;
			//LOG->Info("P3IO Got %d bytes of data: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",iResult, tchunk[0],tchunk[1],tchunk[2],tchunk[3],tchunk[4],tchunk[5],tchunk[6],tchunk[7],tchunk[8],tchunk[9],tchunk[10],tchunk[11]);
			if (iResult >= 12 || connect_index == 2)
			{
				//we have EVERYTHING
				interruptChunkFlags[0] = true;
				interruptChunkFlags[1] = true;
				interruptChunkFlags[2] = true;
				for (int i = 0; i<iResult; i++)
				{
					chunk[i / 4][i % 4] = tchunk[i]; //array data is continuous
				}
			}
			else
			{
				if (iResult == 4)
				{
					//if we get the beginning, reset ourselves
					if (tchunk[0] == 0x80)
					{
						interruptChunkFlags[0] = false;
						interruptChunkFlags[1] = false;
						interruptChunkFlags[2] = false;
						currChunk = 0;
					}
					for (int i = 0; i<4; i++)
					{
						chunk[currChunk][i] = tchunk[i];
					}
					interruptChunkFlags[currChunk] = true;
					currChunk++;
				}
				else
				{
					//LOG->Info("P3IO driver got weird number of bytes (%d) on an interrupt read!",iResult);
				}
			}


			//check all chunks are accounted for
			if (interruptChunkFlags[0] == true)
			{
				if (interruptChunkFlags[1] == true)
				{
					if (interruptChunkFlags[2] == true)
					{
						allChunks = true;
					}
				}
			}


		}
		else
		{
			//we got nothing, increment error and then spit out last state
			m_iInputErrorCount++;
		}
		if (m_iInputErrorCount>2)
		{
			break;
		}
	}

	//if we didnt get what we need, return the previous data
	if (allChunks == false)
	{
		for (int i = 0; i<3; i++)
		{
			for (int j = 0; j<4; j++)
			{
				chunk[i][j] = pchunk[i][j];
			}
		}
	}
	else //copy our current state to the previous state
	{
		for (int i = 0; i<3; i++)
		{
			for (int j = 0; j<4; j++)
			{
				pchunk[i][j] = chunk[i][j];
			}
		}
	}

	//format the data in the way we expect -- harkens back to the UDP p3io streamer
	uint8_t DDR_P1_PAD_UP = checkInput(pchunk[0][1], DDR_PAD_UP);
	uint8_t DDR_P1_PAD_DOWN = checkInput(pchunk[0][1], DDR_PAD_DOWN);
	uint8_t DDR_P1_PAD_LEFT = checkInput(pchunk[0][1], DDR_PAD_LEFT);
	uint8_t DDR_P1_PAD_RIGHT = checkInput(pchunk[0][1], DDR_PAD_RIGHT);

	uint8_t DDR_P1_CP_UP = checkInput(pchunk[0][3], DDR_CP_UP_P1);
	uint8_t DDR_P1_CP_DOWN = checkInput(pchunk[0][3], DDR_CP_DOWN_P1);
	uint8_t DDR_P1_CP_LEFT = checkInput(pchunk[0][1], DDR_CP_LEFT);
	uint8_t DDR_P1_CP_RIGHT = checkInput(pchunk[0][1], DDR_CP_RIGHT);
	uint8_t DDR_P1_CP_SELECT = checkInput(pchunk[0][1], DDR_CP_SELECT);

	uint8_t DDR_P2_PAD_UP = checkInput(pchunk[0][2], DDR_PAD_UP);
	uint8_t DDR_P2_PAD_DOWN = checkInput(pchunk[0][2], DDR_PAD_DOWN);
	uint8_t DDR_P2_PAD_LEFT = checkInput(pchunk[0][2], DDR_PAD_LEFT);
	uint8_t DDR_P2_PAD_RIGHT = checkInput(pchunk[0][2], DDR_PAD_RIGHT);

	uint8_t DDR_P2_CP_UP = checkInput(pchunk[0][3], DDR_CP_UP_P2);
	uint8_t DDR_P2_CP_DOWN = checkInput(pchunk[0][3], DDR_CP_DOWN_P2);
	uint8_t DDR_P2_CP_LEFT = checkInput(pchunk[0][2], DDR_CP_LEFT);
	uint8_t DDR_P2_CP_RIGHT = checkInput(pchunk[0][2], DDR_CP_RIGHT);
	uint8_t DDR_P2_CP_SELECT = checkInput(pchunk[0][2], DDR_CP_SELECT);

	uint8_t DDR_OP_TEST = checkInput(pchunk[0][3], DDR_TEST);
	uint8_t DDR_OP_SERVICE = checkInput(pchunk[0][3], DDR_SERVICE);
	uint8_t DDR_OP_COIN1 = checkInput(pchunk[0][3], DDR_COIN);
	//uint8_t DDR_OP_COIN2=checkInput(pchunk[0][3],DDR_COIN);

	//init the memory... yeah I'm lazy
	dataToQueue[0] = 0;
	dataToQueue[1] = 0;
	dataToQueue[2] = 0;
	if (DDR_P1_PAD_UP) dataToQueue[0] |= BYTE_BIT_M_TO_L(0);
	if (DDR_P1_PAD_DOWN) dataToQueue[0] |= BYTE_BIT_M_TO_L(1);
	if (DDR_P1_PAD_LEFT) dataToQueue[0] |= BYTE_BIT_M_TO_L(2);
	if (DDR_P1_PAD_RIGHT) dataToQueue[0] |= BYTE_BIT_M_TO_L(3);
	if (DDR_P2_PAD_UP) dataToQueue[0] |= BYTE_BIT_M_TO_L(4);
	if (DDR_P2_PAD_DOWN) dataToQueue[0] |= BYTE_BIT_M_TO_L(5);
	if (DDR_P2_PAD_LEFT) dataToQueue[0] |= BYTE_BIT_M_TO_L(6);
	if (DDR_P2_PAD_RIGHT) dataToQueue[0] |= BYTE_BIT_M_TO_L(7);

	if (DDR_P1_CP_UP) dataToQueue[1] |= BYTE_BIT_M_TO_L(0); //up and down buttons constantly pressed when reading with libusbk....
	if (DDR_P1_CP_DOWN) dataToQueue[1] |= BYTE_BIT_M_TO_L(1); //need to get captures of these inputs with oitg to figure out whats up
	if (DDR_P1_CP_LEFT) dataToQueue[1] |= BYTE_BIT_M_TO_L(2);
	if (DDR_P1_CP_RIGHT) dataToQueue[1] |= BYTE_BIT_M_TO_L(3);
	if (DDR_P2_CP_UP) dataToQueue[1] |= BYTE_BIT_M_TO_L(4);//so for now I commented them out
	if (DDR_P2_CP_DOWN) dataToQueue[1] |= BYTE_BIT_M_TO_L(5);//so the game is playable
	if (DDR_P2_CP_LEFT) dataToQueue[1] |= BYTE_BIT_M_TO_L(6);
	if (DDR_P2_CP_RIGHT) dataToQueue[1] |= BYTE_BIT_M_TO_L(7);

	if (DDR_OP_SERVICE) dataToQueue[2] |= BYTE_BIT_M_TO_L(0);
	if (DDR_OP_TEST) dataToQueue[2] |= BYTE_BIT_M_TO_L(1);
	if (DDR_OP_COIN1) dataToQueue[2] |= BYTE_BIT_M_TO_L(2);
	//if (DDR_OP_COIN2) dataToQueue[2] |= BYTE_BIT_M_TO_L(3); //my second coin switch is broken, I am just assuming this is the right constant...
	if (DDR_P1_CP_SELECT) dataToQueue[2] |= BYTE_BIT_M_TO_L(4);
	if (DDR_P2_CP_SELECT) dataToQueue[2] |= BYTE_BIT_M_TO_L(5);

	if (!allChunks){
		Close();
	}
	return allChunks;

}

bool P3IO::sendUnknownCommand()
{
	uint8_t unknown[] = {
	0xaa,
	0x04,
	0x0a,
	0x32,
	0x01,
	0x00
	};
	//in packet captures this needs about a second before execution...
	usleep(1000000);
	LOG->Info("UNKNOWN COMMAND:");
	return WriteToBulkWithExpectedReply( unknown, false);
}


//sent every 78125us...
bool P3IO::pingHDXB()
{
	uint8_t hdxb_ping_command[] = {
		0xaa,
		0x0b,
		0x02,
		0x3a,
		0x00,
		0x07,
		0xaa,//ACIO_PACKET_START
		0x03,//HDXB
		0x01,//type?
		0x10,//opcode
		0x00,
		0x00,
		0x14//CHECKSUM
	};
	LOG->Info("HDXB PING:");
	return WriteToBulkWithExpectedReply(hdxb_ping_command, false);

}

bool P3IO::spamBaudCheck()
{
	bool found_baud=false;
	uint8_t com_baud_command[] = {
	0xaa, // packet start
	0x0f, // length
	0x05, // sequence
	0x3a, // com write
	0x00, // virtual com port
	0x0b, // num bytes
	0xaa, // baud check, escaped 0xaa
	0xaa,	0xaa,	0xaa,	0xaa,	0xaa,	0xaa,	0xaa,	0xaa,	0xaa,	0xaa	};

	for (int i=0;i<50;i++)
	{
		LOG->Info("HDXB BAUD:");
		WriteToBulkWithExpectedReply(com_baud_command, false);
		usleep(16000); // 16 ms wait
		readHDXB(0x40);
		if (bulk_reply_size>20)// we got it!
		{
			usleep(16000); // 16 ms wait
			readHDXB(0x40); // now flush it
			//break;
		}
	}
	return found_baud;
	

}

bool P3IO::initHDXB()
{
	//should build this but.. meh... lazy, manually define it
	uint8_t breath_of_life[]={
		0xaa,
		0x0d,
		0x01,
		0x3a,
		0x00,
		0x09,
		0xaa,//ACIO START
		0x03,//HDXB
		0x01,//TYPE?
		0x28,//OPCODE
		0x00,
		0x02,
		0x00,
		0x00,
		0x2e
	};
	LOG->Info("HDXB INIT:");
	WriteToBulkWithExpectedReply(breath_of_life, false);
	return readHDXB(0x40); // now flush it
}

bool P3IO::openHDXB()
{
	uint8_t com_open_command[] = {
	0xaa, // packet start
	0x05, // length
	0x06, // sequence
	0x38, // com open opcode
	0x00, // virtual com port
	0x00, // @ baud
	0x03  // choose speed: ?, ?, 19200, 38400, 57600
	};
	LOG->Info("HDXB OPEN:");
	return WriteToBulkWithExpectedReply(com_open_command, false);
}

bool P3IO::readHDXB(int len)
{

	uint8_t com_read_command[] = {
		0xaa, // packet start
		0x04, // length
		0x0f, // sequence
		0x3b, // com read opcode
		0x00, // virtual com port
		0x7e  // 7e is 126 bytes to read
	};
	com_read_command[5]=len&0xFF; // plug in passed in len
	LOG->Info("HDXB Read:");
	return WriteToBulkWithExpectedReply(com_read_command, false);
}

//pass in a raw payload only
bool P3IO::writeHDXB(uint8_t* payload, int len, uint8_t opcode)
{
	//frame: 03:01:12:00:0d
	//payload consists of 00:ff:ff:ff:ff:ff:ff:7f:7f:7f:7f:7f:7f
	//must also accomodate acio packet of: aa:03:01:28:00:02:00:00:2e
	acio_request[0]=0x03; // hdxb device id
	acio_request[1]=0x01; // type?
	acio_request[2]=opcode; // set lights?
	acio_request[3]=0x00; //
	acio_request[4]=len & 0xFF; // length
	
	//len should be 0x0d
	memcpy( &acio_request[5],  payload, len * sizeof( uint8_t ) );

	//03:01:12:00:0d:00:ff:ff:ff:ff:ff:ff:7f:7f:7f:7f:7f:7f
	len++; // add checksum to length of payload
	//len is now 0x0e

	//uint8_t unescaped_acio_length = len & 0xFF; // but we dont need this, we need number of bytes to write to acio bus

	//get escaped packet and length
	len+=5; //acio frame bytes added
	len = ACIO::prep_acio_packet_for_transmission(acio_request,len); 

	//we are now ready to transmit over p3io channels...
	uint8_t p3io_acio_message[256];
	p3io_acio_message[0]=0xaa;
	p3io_acio_message[1]=(len+4) & 0xFF;
	p3io_acio_message[2]=0;
	p3io_acio_message[3]=0x3a; //com port write
	p3io_acio_message[4]=0; // actual virtual com port number to use
	p3io_acio_message[5]=len&0xFF; //num bytes to write on the acio bus
	memcpy( &p3io_acio_message[6],  acio_request, len * sizeof( uint8_t ) ); // stuff in a p3io wrapper
	LOG->Info("HDXB Write:");
	return WriteToBulkWithExpectedReply(p3io_acio_message, false);
}

bool P3IO::writeLights(uint8_t* payload)
{
	packets_since_keepalive++;

	//if not connected do nothing
	if (!m_bConnected)
	{
		//LOG->Info("P3IO io driver is in disconnected state! Not doing anything in write lights!");
		return false;
	}

	//arbitrary keep alive signal. maybe can dial this back? send one evry 4 attempts
	//lights are CONSTANTLY sending in this architecture
	packets_since_keepalive %= 6;
	

	
	//compare last payload with this payload -- prevent useless chatter on USB so input has best poll rate
	if ( (memcmp(p_light_payload, payload, sizeof(p_light_payload))!=0) || (packets_since_keepalive == 0) )
	{	//if they dont match...
		
		//copy new payload to previous payload so I can check it next round
		memcpy(p_light_payload, payload, sizeof(p_light_payload));
	
		//make a light message
		uint8_t light_message[] = { 0xaa, 0x07, 0x00, 0x24, ~payload[4], payload[3], payload[2], payload[1], payload[0] };
		//LOG->Info("P3io driver Sending light message: %02X %02X %02X %02X %02X %02X %02X %02X %02X", light_message[0],light_message[1],light_message[2],light_message[3],light_message[4],light_message[5],light_message[6],light_message[7],light_message[8]);

		//send message
		return WriteToBulkWithExpectedReply(light_message, false);
	}
	return true;
}



bool P3IO::WriteToBulkWithExpectedReply(uint8_t* message, bool init_packet)
{
	//LOG->Info("p3io driver bulk write: Applying sequence");
	//first figure out what our message parameters should be based on what I see here and our current sequence
	packet_sequence++;
	packet_sequence %= 16;
	if (!init_packet)
	{
		message[2] = packet_sequence;
	}
	else
	{
		packet_sequence = message[2];
	}

	//max packet size is 64 bytes * 2 for escaped packets
	uint8_t response[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	int message_length = 5;
	int expected_response_size = 5; // includes AA at beginning

	message_length = message[1]+2;

	//LOG->Info("Determining message type...");
	//LOG->Info("(%02X)",message[3]);

	//may not need this except as a lexicon -- com read throws it out the window
	/*
	switch (message[3])
	{


		case 0x01: // version query
			expected_response_size = 11;
			break;

		case 0x31: //coin query
			expected_response_size = 9;
			break;
	
		case 0x2f:// some query
			expected_response_size = 5;
			break;

		case 0x27: //cabinet type query
			expected_response_size = 5;
			break;


		case 0x24: //set light state?
			expected_response_size = 6;
			break;

		case 0x25: //get dongle
			expected_response_size = 45;
			break;
		case 0x3a: // com write
			expected_response_size = 5; /// basically an ack
			break;
	
		default:
			expected_response_size = 5;
	}
	*/
	//LOG->Info("Expected response size is %d", expected_response_size);
	//actually send our message with the parameters we determined
	//but first we need to escape our shit

	//LOG->Info("Escaping our shit");
	
	//CONFIRMED CORRECT
	int bytes_to_write=message_length;
	uint8_t message2[258];
	message2[0]=message[0];
	int old_stream_position=1;
	for (int new_stream_position=1;new_stream_position<bytes_to_write;new_stream_position++)
	{
		if (message[old_stream_position]==0xAA || message[old_stream_position]==0xFF)
		{
			message2[new_stream_position]=0xFF;
			new_stream_position++;
			bytes_to_write++;
			message2[new_stream_position]=~message[old_stream_position];
		}
		else
		{
			message2[new_stream_position]=message[old_stream_position];
		}
		old_stream_position++;
	}
	//END CONFIRMED CORRECT
	LOG->Info("Send: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",message2[0],message2[1],message2[2],message2[3],message2[4],message2[5],message2[6],message2[7],message2[8],message2[9],message2[10],message2[11],message2[12],message2[13],message2[14],message2[15],message2[16],message2[17],message2[18],message2[19],message2[20],message2[21],message2[22],message2[23],message2[24],message2[25],message2[26],message2[27],message2[28],message2[29],message2[30],message2[31],message2[32],message2[33],message2[34],message2[35],message2[36],message2[37],message2[38],message2[39],message2[40],message2[41],message2[42],message2[43],message2[44],message2[45],message2[46],message2[47],message2[48],message2[49],message2[50],message2[51],message2[52],message2[53],message2[54],message2[55],message2[56],message2[57],message2[58],message2[59]);
	int iResult = m_pDriver->BulkWrite(bulk_write_to_ep, (char*)message2, bytes_to_write, REQ_TIMEOUT);

	//return false if we don't send it all
	//if (iResult != bytes_to_write) return false;

	//get ready for the response
	LOG->Info("Asking for ack...");
	iResult = GetResponseFromBulk(response, expected_response_size); //do a potentially fragmented read
	bulk_reply_size=iResult;
	bool success = false;
	if (iResult != expected_response_size)  //if we don't have all the data, return false
	{
		//return false;
	}
	 success = true;

	LOG->Info("Receive: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",response[0],response[1],response[2],response[3],response[4],response[5],response[6],response[7],response[8],response[9],response[10],response[11],response[12],response[13],response[14],response[15],response[16],response[17],response[18],response[19],response[20],response[21],response[22],response[23],response[24],response[25],response[26],response[27],response[28],response[29],response[30],response[31],response[32],response[33],response[34],response[35],response[36],response[37],response[38],response[39],response[40],response[41],response[42],response[43],response[44],response[45],response[46],response[47],response[48],response[49],response[50],response[51],response[52],response[53],response[54],response[55],response[56],response[57],response[58],response[59]);
	return success;
}

//sometimes responses are split across 2 reads... could there be a THIRD read?
//this is a giant hack because data can be escaped which really fucks with length
int P3IO::GetResponseFromBulk(uint8_t* response, int expected_response_length)
{
	int totalBytesRead=0;
	int num_reads=0;
	//int responseLength = m_pDriver->BulkRead(bulk_read_from_ep, (char*)response, 128, REQ_TIMEOUT);
	LOG->Info("begin while loop");
	while (totalBytesRead < expected_response_length) //sometimes a response is fragmented over multiple requests, try a another time
	{
		LOG->Info("GETTING BULK OF EXPECTED SIZE: %02X/%02x",totalBytesRead,expected_response_length);
		num_reads++;

		uint8_t response2[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int bytesReadThisRound = m_pDriver->BulkRead(bulk_read_from_ep, (char*)response2, 64, REQ_TIMEOUT / 2);
		LOG->Info("P3IO read %d bytes.",bytesReadThisRound);
			//Details - R1[0]=%02X R2...= %02X %02X %02X %02X %02X",iResult2,response[0],response2[0],response2[1],response2[2],response2[3],response2[4]);
		if (bytesReadThisRound<1 && num_reads>1)
		{
			LOG->Info("P3IO must have given up sending data we need (%d/%d bytes)",totalBytesRead,expected_response_length);
			break;
		}

		//unescape the bytes
		int i=0;
		if (totalBytesRead==0 && bytesReadThisRound>0)
		{
			//LOG->Info("P3io not escaping sop");
				i=1;
		}
		
		for (i;i<bytesReadThisRound;i++)
		{
			//if we hit an escaped character
			if (response2[i]==0xAA || response2[i]==0xFF)
			{
				//LOG->Info("P3io  escape char at %d",i);
				//decrease the actual number of bytes we read since we escaped it
				bytesReadThisRound--;

				//shift all bytes in the array after this char to the left, overwriting the escaped character
				for (int j=i;j<bytesReadThisRound;j++)
				{
					response2[j]=response2[j+1];
				}

				//zero out the last character
				response2[bytesReadThisRound]=0;

				//unescape the escaped character
				response2[i]=~response2[i];
			}

			
		}

		//everything is unescaped. now copy it into the response.
		//todo: bounds checking
		//LOG->Info("P3IO MEMCOP %d",num_reads);
		for(int j=0;j<bytesReadThisRound;j++)
		{
			response[totalBytesRead+j]=response2[j];
		}
		//memcpy(response+totalBytesRead, response2, bytesReadThisRound);

		if (bytesReadThisRound>0)
		{
		

			//if we have first 2 bytes..
			if (totalBytesRead>1)
			{
				//becuase number of bytes READ is what changes, not the target number of bytes
				expected_response_length=response[1];
			}


			//LOG->Info("P3io addibg %d bytes to %d",bytesReadThisRound,totalBytesRead);
			totalBytesRead+=bytesReadThisRound;
		}


	
	}
		

		//LOG->Info("Got escaped message of %d bytes from %d reads:",totalBytesRead,num_reads);
		

	/*
	if(needed_second)
	{
	LOG->Info("P3IO add. read final data %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",response[0],response[1],response[2],response[3],response[4],response[5],response[6],response[7],response[8],response[9]);
	}
	*/
	LOG->Info("End P3IO get bulk  message");
	return totalBytesRead;
}


void P3IO::Reconnect()
{
	LOG->Info("Attempting to reconnect P3IO.");
	//the actual input handler will ask for a reconnect, i'd rather it happen there for more control
	/* set a message that the game loop can catch and display */
	char temp[256];
	sprintf(temp, "I/O error: %s", m_pDriver->GetError());
	//m_sInputError = ssprintf( "I/O error: %s", m_pDriver->GetError() );
	m_sInputError = m_sInputError.assign(temp);
	Close();
	m_bConnected = false;
	baud_pass=false;
	hdxb_ready=false;
	Open();
	m_sInputError = "";
}


uint8_t P3IO::checkInput(uint8_t x, uint8_t y)
{
	//there is a better way to do this but I'll optimize later, I wanted to make sure the logic was RIGHT
	uint8_t v = x;
	uint8_t w = y;

	//enable these 2 for p3io
	v = ~v;
	w = ~w;

	//printf("comparing %02X : %02X\n",v,w);
	return v & w;
}


static uint8_t p3io_init_hd_and_watchdog[21][45] = {
	{ 0xaa, 0x02, 0x00, 0x01 }, //0x01 is get version -- replies with 47 33 32 00 02 02 06 (G32)
	{ 0xaa, 0x02, 0x00, 0x2f },//?
	{ 0xaa, 0x03, 0x01, 0x27, 0x01 }, //0x27 is get cabinet type
	{ 0xaa, 0x02, 0x02, 0x31 },//coins?
	{ 0xaa, 0x02, 0x03, 0x01 }, //0x01 is get version -- replies with 47 33 32 00 02 02 06 (G32)
	{ 0xaa, 0x03, 0x04, 0x27, 0x00 }, //0x27 is get cabinet type

	//read sec plug
	{ 0xaa, 0x2b, 0x05, 0x25, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xaa, 0x2b, 0x06, 0x25, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xaa, 0x2b, 0x07, 0x25, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xaa, 0x2b, 0x08, 0x25, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xaa, 0x2b, 0x09, 0x25, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xaa, 0x2b, 0x0a, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xaa, 0x2b, 0x0b, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xaa, 0x2b, 0x0c, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xaa, 0x2b, 0x0d, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xaa, 0x2b, 0x0e, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	//end read sec plug
	
	{ 0xaa, 0x03, 0x0f, 0x05, 0x00 },//these next 4 clearly set some parameters. no idea what they are
	{ 0xaa, 0x03, 0x00, 0x2b, 0x01 },
	{ 0xaa, 0x03, 0x01, 0x29, 0x05 },
	{ 0xaa, 0x03, 0x02, 0x05, 0x30 }, //is thi sa 48 second watch dog?
	{ 0xaa, 0x03, 0x03, 0x27, 0x00 }, //0x27 is get cabinet type
};
const unsigned NUM_P3IO_INIT_MESSAGES = ARRAYLEN(p3io_init_hd_and_watchdog);

//the watchdog application needs to be running on the pc I think.
//On a nerfed pcb this likely wont do anything but on an  real cabinet,
//if you don't send a keep alive... KABOOM!
//Also init HD controls. always do so. Harmless to SD cabinets.
void P3IO::InitHDAndWatchDog()
{
	for (int i = 0; i<NUM_P3IO_INIT_MESSAGES; i++)
	{
		WriteToBulkWithExpectedReply(p3io_init_hd_and_watchdog[i], true);
		usleep(16666);
	}
}