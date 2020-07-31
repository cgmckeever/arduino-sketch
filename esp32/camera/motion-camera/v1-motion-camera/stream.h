#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

void end_stream();
void start_stream();

static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  start_stream();
  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);

  if(res != ESP_OK) {
    end_stream();
    return res;
  }

  while(true){
    fb = esp_camera_fb_get();

    if (!fb) {
      Serial.println("Camera Stream capture failed");
      res = ESP_FAIL;
    } else {
      if(fb->width > 400) {
        if(fb->format != PIXFORMAT_JPEG) {
          Serial.println('JPEG');
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
        
          if(!jpeg_converted) {
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }

    if(res == ESP_OK) {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);

      if(res == ESP_OK) {
        res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);

        if(res == ESP_OK){
          res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
      }
    }
    
    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }

    if(res != ESP_OK) break;
  }

  end_stream();
  return res;
}

void registerStream() {
  httpd_uri_t uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };
  
  httpd_register_uri_handler(server, &uri);
}

void start_stream() {
  is_capture = false;
  Serial.println("Steaming Mode.");
  sensor_t * sensor = esp_camera_sensor_get();
  sensor->set_pixformat(sensor, PIXFORMAT_JPEG);
  sensor->set_framesize(sensor, FRAMESIZE_UXGA);
}

void end_stream() {
  is_capture = true;
  Serial.println("Exit Steaming Mode.");
}