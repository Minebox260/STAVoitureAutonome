#ifndef SENDTOARDUINO_H_
#define SENDTOARDUINO_H_

#include "main.h"

int serial_ouvert();

int open_comm_arduino();

void send_to_arduino(int port, int16_t code);

void send_stop_command(int port);

void emptySerial();

void send_next_point_to_arduino(struct PARAMS * params, int port, Point next, Point current);

void close_comm_arduino(int port);

#endif
