#include "send_to_arduino.h"

#define ARDUINO_SERIAL_DEVICE "/dev/ttyUSB0"

int serial_ouvert()
{
    int fd;
    struct termios options;
    
    fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
    
    if (fd == -1) {
        perror("Error: Unable to open serial port");
        return 1;
    }

    tcgetattr(fd, &options);
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 10;

    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        perror("Error: Unable to set serial port options");
        return 1;
    }
    sleep(1);

    return fd;
}

int open_comm_arduino() {
	int arduino_fd;
	if ((arduino_fd = serialOpen(ARDUINO_SERIAL_DEVICE, 115200)) < 0 ) {
		fprintf(stderr, "Unable to open serial device: %s\n", ARDUINO_SERIAL_DEVICE);
		return 1;
	}
	printf("setting up serial comm\n");
	sleep(5);
	return arduino_fd;
}

void emptySerial(int port) {
	char _;
	while (serialDataAvail(port) > 0) {
			printf("\nemptied\n\n");
			_ = serialGetchar(port);
	}
}
	
//Call seperateley for each integer you want to send
//code 1: send next position
//code 2: stop 
int send_code_to_arduino(int port, int16_t code) {
	char received_data[64];
	char received_char;
	int ack = 0;
	time_t start, current;
	double duration;
	int timeout = 5;
	
	//send code
	int16_t* codeptr = &code;
	for(int j = 0; j < sizeof(int16_t); j++) {
		//send each byte as a char seperately
		serialPutchar(port, *(codeptr+j));
	}
	serialPutchar(port, '\0');
	printf("sent code %d ARDUINO\n", code);
	
	//wait for acknowldegment
	start = time(NULL);
	
	while(!ack) {
		while (serialDataAvail(port) > 0) {
			received_char = serialGetchar(port);
			if (received_char != -1) {
				strncat(received_data, &received_char, 1);
			}
		}
		if (strstr(received_data, "ACK") != NULL) {
			ack = 1;
			
			printf("RECEIVED ACK\n");
			return 1;
		}
		
		current = time(NULL);
		duration = difftime(current,start);
		if (duration >= timeout) {
			emptySerial(port);
			return 2;
		}
	}
}

void send_stop_command(int port) {
	printf("\n---STOP CMD TO ARDUINO---\n");

	int codeOutput = send_code_to_arduino(port, 2);

	if(codeOutput==0) {
		printf("no ACK received ERROR\n");
		return;
	} else if (codeOutput==2) {
		printf("ACK receive timed out ERROR\n");
		return;
	}
}

void send_next_point_to_arduino(struct PARAMS * params, int port, Point next, Point current) {
	printf("\n---START COMM ARDUINO---\n");

	if (current.x == 0 && current.y == 0) {
		printf("0: ABORTING COMM\n");
		return;
	}
	
	int codeOutput = send_code_to_arduino(port, 1);

	if (codeOutput==2) {
		printf("ACK receive timed out ERROR\n");
		return;
	}
	printf("CURRENT X: %d, CURRENT Y: %d\n",current.x,current.y);
	//testing straight line
	if (DEBUG_STRAIGHT == 1) {
		//current.x = START_OF_LINE_X;
		//current.y = START_OF_LINE_Y;
		//next.x = params->END_OF_LINE_X;//886;
		//next.y = params->END_OF_LINE_Y;//3323;
		//printf("NEXT X: %d, NEXT Y: %d\n",next.x,next.y);
	}
	
	int32_t data[4] = {(int32_t)current.x, (int32_t)current.y, (int32_t)next.x, (int32_t)next.y};
	//printf("current: %d %d\n",data[0], data[1]);
	printf("---POINTS SENT---\n");
	for(int32_t i=0; i < 4; i++) {
		char * intptr = (char*)&data[i];
		for(int32_t j = 0; j < sizeof(int32_t); j++) {
			//send each byte as a char seperately
			printf("%02X ", *(intptr+j));
			serialPutchar(port, *(intptr+j));
		}
		//serialPutchar(port, '\0'); not necessary with bytewise comm
		//printf("\n");	
		/*
		char * intptr = (char*)&data[i];
		for(int j=0; j < sizeof(int32_t); j++) {
			//send each byte as a char seperately
			printf("%02X ", ((data[i] >> 8*(3-j)) && 0xFF));
			serialPutchar(port, ((data[i] >> 8*(3-j)) && 0xFF));
		}
		printf("\n");*/
	}
	printf("\n");	
	//printf("sent points to ARDUINO\n");
	
	
	//wait for debug return
	long duration; //in milliseconds
	time_t start, now;
	start = time(NULL);
	int timeout = 1;
	int numInt = 6;
	uint8_t buffer[numInt*sizeof(int32_t)];
	for (int i; i < numInt*sizeof(int32_t); i++) {
		buffer[i] = 0;
	}
	
	int count = 0;
	printf("---POINTS RECEIVED FOR VERIFICATION---\n");
	while(1) {
		if (serialDataAvail(port) > 0) {
			char received_byte = serialGetchar(port);
			printf("%02X ", received_byte);
			buffer[count] = received_byte;
			count++;
		}
		now = time(NULL);
		duration = difftime(now,start);
		if (duration >= timeout) {
			//printf("\nTIMEOUT\n");
			break;
		}
	}
	printf("\n");
	int32_t ndata[numInt];
	int firstIx;
	for(int i = 0; i<numInt; i++) {
		firstIx = 4*i;
		ndata[i] = (((int32_t)buffer[firstIx+3] << 24) + ((int32_t)buffer[firstIx+2] << 16)\
				+ ((int32_t)buffer[firstIx+1] << 8) + ((int32_t)buffer[firstIx]));
        }
    firstIx = 4*sizeof(int32_t);
    //float on raspberry 4 bytes, double on arduino also 4 bytes
	int32_t angle = (((int32_t)buffer[firstIx+3] << 24) + ((int32_t)buffer[firstIx+2] << 16)\
				+ ((int32_t)buffer[firstIx+1] << 8) + ((int32_t)buffer[firstIx]));

    firstIx = 5*sizeof(int32_t);
    //float on raspberry 4 bytes, double on arduino also 4 bytes
	int32_t angleGyro = (((int32_t)buffer[firstIx+3] << 24) + ((int32_t)buffer[firstIx+2] << 16)\
				+ ((int32_t)buffer[firstIx+1] << 8) + ((int32_t)buffer[firstIx]));

  
	printf("VERIFICATION\ncurrent - x: %d, y: %d\n", ndata[0], ndata[1]);
	printf("next - x: %d, y: %d\n", ndata[2], ndata[3]);
	printf("targetAngle: %d\n", angle);
	printf("gyro: %d\n", angleGyro);
	
}
/*
void send_next_point_to_arduino(int port, Point next, Point current) {
	printf("\n---START COMM ARDUINO---\n");

	if (current.x == 0 && current.y == 0) {
		return;
	}
	
	int codeOutput = send_code_to_arduino(port, 1);

	if(codeOutput==0) {
		printf("no ACK received ERROR\n");
		return;
	} else if (codeOutput==2) {
		printf("ACK receive timed out ERROR\n");
		return;
	}
	//testing straight line
	if (DEBUG_STRAIGHT == 1) {
		next.x = END_OF_LINE_X;
		next.y = END_OF_LINE_Y;
		printf("NEXT X: %d, NEXT Y: %d\n",next.x,next.y);
	}
	
	int32_t data[4] = {(int32_t)current.x, (int32_t)current.y, (int32_t)next.x, (int32_t)next.y};
	//printf("current: %d %d\n",data[0], data[1]);
	printf("---POINTS SENT---\n");
	for(int32_t i=0; i < 4; i++) {
		char * intptr = (char*)&data[i];
		for(int32_t j = 0; j < sizeof(int32_t); j++) {
			//send each byte as a char seperately
			printf("%02X ", *(intptr+j));
			serialPutchar(port, *(intptr+j));
		}
		//serialPutchar(port, '\0'); not necessary with bytewise comm
		//printf("\n");	
		
		char * intptr = (char*)&data[i];
		for(int j=0; j < sizeof(int32_t); j++) {
			//send each byte as a char seperately
			printf("%02X ", ((data[i] >> 8*(3-j)) && 0xFF));
			serialPutchar(port, ((data[i] >> 8*(3-j)) && 0xFF));
		}
		printf("\n");
	}
	printf("\n");	
	//printf("sent points to ARDUINO\n");
	
	
	//wait for debug return
	long duration; //in milliseconds
	time_t start, now;
	start = time(NULL);
	int timeout = 1;
	uint8_t buffer[16];
	for (int i; i < 16; i++) {
		buffer[i] = 0;
	}
	int count = 0;
	printf("---POINTS RECEIVED FOR VERIFICATION---\n");
	while(1) {
		if (serialDataAvail(port) > 0) {
			char received_byte = serialGetchar(port);
			printf("%02X ", received_byte);
			buffer[count] = received_byte;
			count++;
		}
		now = time(NULL);
		duration = difftime(now,start);
		if (duration >= timeout) {
			//printf("\nTIMEOUT\n");
			break;
		}
	}
	printf("\n");
	int32_t ndata[4];
	for(int i = 0; i<4; i++) {
		int firstIx = 4*i;
		ndata[i] = (((int32_t)buffer[firstIx+3] << 24) + ((int32_t)buffer[firstIx+2] << 16)\
			 + ((int32_t)buffer[firstIx+1] << 8) + ((int32_t)buffer[firstIx]));
        }
      
	printf("VERIFICATION\ncurrent - x: %d, y: %d\n", ndata[0], ndata[1]);
	printf("VERIFICATION\nnext - x: %d, y: %d\n", ndata[2], ndata[3]);

}*/

void close_comm_arduino(int port) {
	serialClose(port);
}
