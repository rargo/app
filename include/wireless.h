#ifndef WIRELESS_H
#define WIRELESS_H

int wl_command(char *in, char *out, int timeout);
int wl_init(void);
int wl_connect(void);
#endif
