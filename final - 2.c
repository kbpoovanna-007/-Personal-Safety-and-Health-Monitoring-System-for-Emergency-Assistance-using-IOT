#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP_Mail_Client.h>

#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign in credentials */ 
#define AUTHOR_EMAIL "your_email@mail.com" // your email mailID
#define AUTHOR_PASSWORD "your_app_password" // your app password
#define RECIPIENT_EMAIL "recipient_email@mail.com" // the receiver mail id

/* Declare the global used SMTPSession object for SMTP transport */ 
SMTPSession smtp;

/* Callback function to get the Email sending status */ 
void smtpCallback(SMTP_Status status);

const int buttonPin = 2;
int buttonState = 0; // Current state of the button 
int lastButtonState = 0; // Previous state of the button

void setup() {
    Serial.begin(115200);
    Serial.println();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}


/* Set the session config */ 
config.server.host_name = SMTP_HOST; 
config.server.port = SMTP_PORT; 
config.login.email = AUTHOR_EMAIL; 
config.login.password = AUTHOR_PASSWORD; 
config.login.user_domain = "";


void loop() {
    buttonState = digitalRead(buttonPin); // Read the state of the button
    if (buttonState == lastButtonState)
        delay(50);
    if (buttonState == LOW) {
        // Debounce
        sendEmergencyEmail(); // If button is pressed, send the emergency email
    }
    lastButtonState = buttonState;
}



void sendEmergencyEmail() {
    // Declare the message class 
    SMTP_Message message;
    /* Set the message headers */
    message.sender.name = F("ESP");
    message.sender.email = AUTHOR_EMAIL;
    message.subject = F("Please Save my Life!");
    message.addRecipient(F("**"), RECIPIENT_EMAIL);
    /* send email body */
    // Add recipient name here if needed
    String emailBody = "Hello, I hope you are doing well!\n";
    emailBody += "I am currently in a life-threatening situation and may die anytime soon.\n\n";
    emailBody += "Kindly reach my location using the coordinates as shown in the below link: \n";
    emailBody += "LINKI \n\n";
    emailBody += "You can see my heart beats in the graph shown in the below link: \n";
    emailBody += "LINK<2\n\n";
    emailBody += "Kindly, I request you to reach my location as soon as possible to save my life.";
    message.text.content = emailBody.c_str();
    message.text.charset = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
    message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
    message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;
    /* Connect to the server */
    if (!smtp.connect(config)) {
        ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
        return;
    }
    if (!smtp.isLoggedIn()) {
        Serial.println("\nNot yet logged in.");
    } else {
        if (smtp.isAuthenticated())
            Serial.println("\nSuccessfully logged in.");
        else
            Serial.println("\nConnected with no Auth.");
    }
    /* Start sending Email and close the session */
    if (!Mailclient.sendMail(smtp.IMessage))
        ESP_MAIL_PRINTF("Error, Status Code: 20, Error Code: %d, Reason: %s",
                        smtp.statusCode(), smtp.errorCode(),
                        smtp.errorReason().c_str());
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status) {
    /* Print the current status */
    Serial.println(status.info());
    
    /* Print the sending result */
    if (status.success()) {
        Serial.println("--\n");
        ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
        ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
        Serial.println("--\n");
        for (size_t i = 0; i < smtp.sendingResult.size(); i++) {
            /* Get the result item */
            SMTP_Result result = smtp.sendingResult.getItem(i);
            ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
            ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
            ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
            ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
            ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
        }
        Serial.println("--\n");
        // You need to clear sending result as the memory usage will grow up. 
        smtp.sendingResult.clear();
    }
}


