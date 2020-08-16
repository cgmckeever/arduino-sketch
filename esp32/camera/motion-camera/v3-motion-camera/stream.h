/**

https://github.com/arkhipenko/esp32-cam-mjpeg-multiclient

**/

// We will try to achieve 25 FPS frame rate
//const int FPS = 14;
const int TICKRATE = 75;

// We will handle web client requests every 50 ms (20 Hz)
const int WSINTERVAL = 5000;

// current buffer and size
volatile char* camBuf;
volatile size_t camSize;

// ESP32 has two cores:
#define cpu0 0
#define cpu1 1

#include <WiFiClient.h>
WebServer *configServer;

void capture(void* pvParameters);
void stream(void * pvParameters);
void handleStream(void);
void handleCapture(void);
char* allocateMemory(char* aPtr, size_t aSize);

// ===== task handles =========================
// Streaming is implemented with 3 tasks:
TaskHandle_t taskMain;    // handles client connections to the webserver
TaskHandle_t taskCapture; // handles getting picture frames from the camera and storing them locally
TaskHandle_t taskStream;  // actually streaming frames to all connected clients

// frameSync semaphore is used to prevent streaming buffer as it is replaced with the next frame
SemaphoreHandle_t frameSync = NULL;

// Queue stores currently connected clients to whom we are streaming
QueueHandle_t streamingClients;


// ======== Server Connection Handler Task ==========================
void mainHandler(void* pvParameters) {
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(WSINTERVAL);

  // Creating frame synchronization semaphore and initializing it
  frameSync = xSemaphoreCreateBinary();
  xSemaphoreGive(frameSync);

  // Creating a queue to track all connected clients
  streamingClients = xQueueCreate(10, sizeof(WiFiClient*));

  //  Creating RTOS task for grabbing frames from the camera
  xTaskCreatePinnedToCore(capture, "captureTask", 4096, NULL, 2, &taskCapture, cpu1);

  //  Creating task to push the stream to all connected clients
  xTaskCreatePinnedToCore(stream, "streamTask", 4096, NULL, 2, &taskStream, cpu1);

  //=== loop() section  ===================
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    configManager.loop();

    // After every server client handling request
    // let other tasks run and then pause
    taskYIELD();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}


// ==== RTOS task to grab frames from the camera =========================
void capture(void* pvParameters) {
  TickType_t xLastWakeTime;

  //  A running interval associated with currently desired frame rate
  const TickType_t xFrequency = pdMS_TO_TICKS(TICKRATE);

  // Mutex for the critical section of swithing the active frames around
  portMUX_TYPE xSemaphore = portMUX_INITIALIZER_UNLOCKED;

  // Pointers to the 2 frames, their respective
  // sizes and index of the current frame
  char* fbs[2] = { NULL, NULL };
  size_t fSize[2] = { 0, 0 };
  int ifb = 0;

  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    // Grab a frame from the camera and query its size
    cam.run();
    size_t s = cam.getSize();

    // If frame size is more that we have previously allocated
    // request  125% of the current frame space
    if (s > fSize[ifb]) {
      fSize[ifb] = s * 4 / 3;
      fbs[ifb] = allocateMemory(fbs[ifb], fSize[ifb]);
    }

    //  Copy current frame into local buffer
    char* b = (char*) cam.getBuffer();
    memcpy(fbs[ifb], b, s);

    //  Let other tasks run and wait until the end of the current frame rate interval (if any time left)
    taskYIELD();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    //  Only switch frames around if no frame is currently being streamed to a client
    //  Wait on a semaphore until client operation completes
    xSemaphoreTake( frameSync, portMAX_DELAY );

    //  Do not allow interrupts while switching the current frame
    portENTER_CRITICAL(&xSemaphore);
    camBuf = fbs[ifb];
    camSize = s;
    ifb++;
    ifb &= 1;  // this should produce 1, 0, 1, 0, 1 ... sequence
    portEXIT_CRITICAL(&xSemaphore);

    // Let anyone waiting for a frame know that the frame is ready
    xSemaphoreGive(frameSync);

    // Technically only needed once: let the streaming task know that we have at least one frame
    // and it could start sending frames to the clients, if any
    xTaskNotifyGive(taskStream);

    // Immediately let other (streaming) tasks run
    taskYIELD();

    // If streaming task has suspended itself (no active clients to stream to)
    // there is no need to grab frames from the camera. We can save some juice
    // by suspedning the tasks passing NULL means "suspend yourself"
    if (eTaskGetState(taskStream) == eSuspended) vTaskSuspend(NULL);
  }
}


// ==== Memory allocator that takes advantage of PSRAM if present =======================
char* allocateMemory(char* aPtr, size_t aSize) {

  //  Since current buffer is too smal, free it
  if (aPtr != NULL) free(aPtr);

  size_t freeHeap = ESP.getFreeHeap();
  char* ptr = NULL;

  // If memory requested is more than 2/3 of the currently
  // free heap, try PSRAM immediately
  if ( aSize > freeHeap * 2 / 3 ) {
    if ( psramFound() && ESP.getFreePsram() > aSize ) {
      ptr = (char*) ps_malloc(aSize);
    }
  } else {
    //  Enough free heap - let's try allocating fast RAM as a buffer
    ptr = (char*) malloc(aSize);

    //  If allocation on the heap failed, let's give PSRAM one more chance:
    if ( ptr == NULL && psramFound() && ESP.getFreePsram() > aSize) {
      ptr = (char*) ps_malloc(aSize);
    }
  }

  // Finally, if the memory pointer is NULL, we were not able to
  // allocate any memory, and that is a terminal condition.
  if (ptr == NULL) ESP.restart();

  return ptr;
}


// ==== Handle connection request from clients =========
const char HEADER[] = "HTTP/1.1 200 OK\r\n" \
                      "Access-Control-Allow-Origin: *\r\n" \
                      "Content-Type: multipart/x-mixed-replace; boundary=123456789000000000000987654321\r\n";
const char BOUNDARY[] = "\r\n--123456789000000000000987654321\r\n";
const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
const int hdrLen = strlen(HEADER);
const int bdrLen = strlen(BOUNDARY);
const int cntLen = strlen(CTNTTYPE);

void handleStream(void) {
  //  Can only acommodate 10 clients. The limit is a default for WiFi connections
  if ( !uxQueueSpacesAvailable(streamingClients) ) return;

  //  Create a new WiFi Client object to keep track of this one
  WiFiClient* client = new WiFiClient();
  *client = configServer->client();

  //  Immediately send this client a header
  client->write(HEADER, hdrLen);
  client->write(BOUNDARY, bdrLen);

  // Push the client to the streaming queue
  xQueueSend(streamingClients, (void *) &client, 0);

  // Wake up streaming tasks, if they were previously suspended:
  if ( eTaskGetState(taskCapture) == eSuspended ) vTaskResume(taskCapture);
  if ( eTaskGetState(taskStream) == eSuspended ) vTaskResume(taskStream);
}


// ==== STREAMING =================
void stream(void * pvParameters) {
  char buf[16];
  TickType_t xLastWakeTime;
  TickType_t xFrequency;

  //  Wait until the first frame is captured and there is something to send
  //  to clients
  ulTaskNotifyTake(pdTRUE,          /* Clear the notification value before exiting. */
                    portMAX_DELAY); /* Block indefinitely. */

  xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    //  Only bother to send anything if there is someone watching
    UBaseType_t activeClients = uxQueueMessagesWaiting(streamingClients);
    if (activeClients) {
      //  pop a client from the the front of the queue
      WiFiClient *client;
      xQueueReceive(streamingClients, (void*) &client, 0);

      if (!client->connected()) {
        //  delete this client reference if disconnected
        delete client;
      } else {
        // Grab a semaphore to prevent frame changes
        // while we are serving this frame
        xSemaphoreTake(frameSync, portMAX_DELAY);

        client->write(CTNTTYPE, cntLen);
        sprintf(buf, "%d\r\n\r\n", camSize);
        client->write(buf, strlen(buf));
        client->write((char*) camBuf, (size_t)camSize);
        client->write(BOUNDARY, bdrLen);

        // Push client to back of queue
        xQueueSend(streamingClients, (void *) &client, 0);

        //  The frame has been served. Release the semaphore and let other tasks run.
        //  If there is a frame switch ready, it will happen now in between frames
        xSemaphoreGive(frameSync);
        taskYIELD();
      }
    } else {
      // Suspend till clients
      vTaskSuspend(NULL);
    }
    //  Let other tasks run (reduced freq per client)
    taskYIELD();

    xFrequency = pdMS_TO_TICKS(TICKRATE);
    if (activeClients) xFrequency /= (activeClients + 1);
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// ==== Serve up one JPEG frame ======
const char JHEADER[] = "HTTP/1.1 200 OK\r\n" \
                       "Content-disposition: inline; filename=capture.jpg\r\n" \
                       "Content-type: image/jpeg\r\n\r\n";
const int jhdLen = strlen(JHEADER);

void handleCapture(void) {
  WiFiClient client = configServer->client();

  if (!client.connected()) return;
  cam.run();
  client.write(JHEADER, jhdLen);
  client.write((char*) cam.getBuffer(), cam.getSize());
}

void streamSetup() {
  xTaskCreatePinnedToCore(mainHandler, "mainTask", 4096, NULL, 2, &taskMain, cpu1);
}
