String saveFile(unsigned char *buf, unsigned int len, String file);

#include "standard.h"


#define CAMERA_MODEL_AI_THINKER
#include "camera.h"

void setup(void) {
    Serial.begin(115200);

    startWifi();
    timeClient.begin();
    setTime();

    initCamera();
}

void loop() {
    delay(10000);
    capture_and_save();
}


void capture_and_save() {
    camera_fb_t *fb = capture();

    if (fb) {
        String path = "/picture." + String(time(NULL)) + "." + esp_random() + ".jpg";
        path = saveFile(fb->buf, fb->len, path); 

        smtpMotion.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);
        smtpMotion.setSender("ESP32", emailSenderAccount);
        smtpMotion.addRecipient(emailAlertAddress);
        smtpMotion.setPriority("High");
        smtpMotion.setSubject("Motion Detected " + path);
        smtpMotion.setMessage("<div style=\"color:#2f4468;\"><h1>Hello World!</h1><p>- Sent from ESP32 board</p></div>", true);

        if (path == "") {
            Serial.println("Photo failed to save.");
        } else {
            Serial.println("Attaching: " + path);
            smtpMotion.addAttachData(path, "image/jpg", fb->buf, fb->len);
        }


        if (MailClient.sendMail(smtpMotion)) {
            Serial.println("Alert Sent");
        } else {
            Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
        }

        smtpMotion.empty();
        esp_camera_fb_return(fb);
    }
}

