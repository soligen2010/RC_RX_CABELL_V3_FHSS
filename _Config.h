// Uncomment the option to enable security at startup and for the Failsafe. If there is no option, the security is deactivated.
// #define SAFETY_LOW
#define SAFETY_MID


// Personnal config
#define RADIO_IRQ_PIN          8 
#define RADIO_IRQ_PIN_bit      0           //8 = PB0
#define RADIO_IRQ_port         PORTB
#define RADIO_IRQ_ipr          PINB
#define RADIO_IRQ_ddr          DDRB

#define RADIO1_CSN_PIN         7     // AKA A0
#define RADIO1_CE_PIN          4
