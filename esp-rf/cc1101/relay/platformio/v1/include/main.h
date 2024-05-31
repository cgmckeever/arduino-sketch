void APCallback(WebServer *server);
void APICallback(WebServer *server);
void setConfigDefaults();
void printConfig();

char* decode(unsigned long decimal, unsigned int length, unsigned int delay, unsigned int* raw, unsigned int protocol);

void enableRx();
void rtl433Callback(char* message);
void switchTransmit(struct switchCommand command);
void messagePost(String path, char* message);

void enableRx();
void disableRx();
void enableTx();
void disableTx();