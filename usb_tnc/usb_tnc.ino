
// Include LibAPRS
#include <LibAPRS.h>
#include <TimerOne.h>

#define KISS_STATE_DATA 0
#define KISS_STATE_CMD 1
#define KISS_STATE_ESC 2
#define KISS_STATE_END 3

#define FEND 0xC0
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD

extern Afsk *AFSK_modem;
extern AX25Ctx AX25;
uint8_t AFSK_dac_isr(Afsk *afsk);

void setup() {
    // Set up serial port
    Serial.begin(9600);
    Serial1.begin(9600);
    pinMode(13, OUTPUT);    
    pinMode(9, OUTPUT);
    Timer1.initialize(64); // set a timer of length 100000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
    Timer1.pwm(9, 512);
    Timer1.attachInterrupt( timerIsr ); // attach the service routine here

    // Initialise APRS library - This starts the modem
    APRS_init();
  
    // You must at a minimum configure your callsign and SSID
    APRS_setCallsign("OH6EYA", 1);
    APRS_setPreamble(250);
}

void loop() {
    static uint8_t msg[256];
    static uint8_t msg_i = 0;
    static uint8_t state = KISS_STATE_END;
  
    while (Serial.available()) {
	uint8_t c = Serial.read();

	// Send if FEND and we have something to send
	if (c == FEND) {
	    if (state == KISS_STATE_DATA && msg_i != 0) {
		// Something to send
		Serial1.write("\nPKT: ");
		Serial1.write(msg, msg_i);
		ax25_sendRaw(&AX25, msg, msg_i);
		msg_i = 0;
	    }
	    state = KISS_STATE_CMD;
	    continue;
	}

	if (state == KISS_STATE_CMD) {
	    // We currently support only frame at first TNC
	    state = c == 0x00 ? KISS_STATE_DATA : KISS_STATE_END;
	    continue;
	}

	// Must set before decoding escapes
	if (state == KISS_STATE_DATA && c == FESC) {
	    state = KISS_STATE_ESC;
	    continue;
	}
	
	// Decode escape
	if (state == KISS_STATE_ESC) {
	    switch (c) {
	    case TFEND:
		c = FEND;
		break;
	    case TFESC:
		c = FESC;
		break;
	    default:
		// Garbage
		state = KISS_STATE_END;
		continue;
	    }
	    state = KISS_STATE_DATA;
	}

	if (state == KISS_STATE_DATA) {
	    if (msg_i >= sizeof(msg)) {
		// Too long content
		state = KISS_STATE_END;
	    } else {
		msg[msg_i++] = c;
	    }
	}
	// Else: error state; do nothing	
    }
}

void timerIsr()
{
    // Toggle LED
    digitalWrite( 13, digitalRead( 13 ) ^ 1 );
    Timer1.pwm(9, (int)AFSK_dac_isr(AFSK_modem) << 2);
}
