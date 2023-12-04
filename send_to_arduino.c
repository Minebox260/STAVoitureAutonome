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
	if ((arduino_fd = serialOpen(ARDUINO_SERIAL_DEVICE, 9600)) < 0 ) {
		fprintf(stderr, "Unable to open serial device: %s\n", ARDUINO_SERIAL_DEVICE);
		return 1;
	}
	
	return arduino_fd;
}

//Call seperateley for each integer you want to send
//code 1: send next position
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
			
			printf("\nRECEIVED ACK\n\n");
			return 1;
		}
		
		current = time(NULL);
		duration = difftime(current,start);
		if (duration >= timeout) {
			printf("TIMEOUT ack failed\n");
			return -1;
		}
	}
	return 0;
}

void send_next_point_to_arduino(int port, Point next, Point current) {
	int codeOutput = end_code_to_arduino(port, 1);
	if(codeOutput==0) {
		printf("no ACK received ERROR\n");
		return;
	} else if (codeOutput==-1) {
		printf("ACK receive timed out ERROR\n");
		return;
	}
		
	
	int data[4] = {(int)current.x, (int)current.y, (int)next.x, (int)next.y};
	
	for(int i=0; i < 4; i++) {
		char * intptr = (char*)&data[i];
		for(int j = 0; j < sizeof(int); j++) {
			//send each byte as a char seperately
			serialPutchar(port, *(intptr+j));
		}
		serialPutchar(port, '\0');	
	}
	
	printf("sent points to ARDUINO\n");

}


void close_comm_arduino(int port) {
	serialClose(port);
}
