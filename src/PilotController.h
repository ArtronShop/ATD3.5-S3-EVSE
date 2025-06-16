#include <stdint.h>
#include <Arduino.h>

#define CP_RX_TO_VOLATGE_FACTOR (1) // Assuming a 12-bit ADC with a reference voltage of 3.3V

#define ADC_GET_CP_STATE_REQ_FLAG (BIT0)

typedef enum {
    STATE_INITIAL = 0,
    STATE_A,
    STATE_B,
    STATE_C,
    STATE_ERROR,
    STATE_ERROR_WAIT_DISCONNECT,
} PilotState_t;

class PilotController {
    public: 
        int cp_tx_pin = -1;
        int cp_rx_pin = -1;
        int max_current = 6; // Default maximum current in Amperes

        PilotState_t state;

        int last_cp_logic = 0;

        int pwm_state = 0;
        EventGroupHandle_t pilot_controller_event_group;

        void setCPLogic(uint8_t value) ;
        int getCPLogic() ;
        
        PilotController() ;
        bool begin(int cp_tx_pin, int cp_rx_pin, int max_current = 6) ;
        void run() ;

};
