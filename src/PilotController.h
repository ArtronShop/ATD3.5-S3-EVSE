#include <stdint.h>
#include <Arduino.h>
#include <Wire.h>

typedef enum {
    MAX_CURRENT_6A = 0,
    MAX_CURRENT_12A,
    MAX_CURRENT_18A,
    MAX_CURRENT_24A,
    MAX_CURRENT_30A,
} MaxAvailableCurrent_t;

typedef enum {
    STATE_INITIAL = 0,
    STATE_A,
    STATE_B,
    STATE_C,
    STATE_RESERVED1,
    STATE_RESERVED2,
    STATE_F,
} PilotState_t;

struct __attribute__((__packed__)) {
  struct __attribute__((__packed__)) {
    uint32_t ST : 4; // State of Control Pilot Pin, 0: Init, 1: State A, 2: State B, 3: State C, 4-5: Reserved, 6: State F
    uint32_t STCIF : 1; // State Change Interrupt Flag,  0: No state change, 1: New State
    uint32_t DFDIF : 1; // Diode Failed Detect Interrupt Flag,  0: Failed Detect, 1: No Detect
    uint32_t _RESERVED1 : 2;
  } SR; // Status Register
  struct __attribute__((__packed__)) {
    uint32_t MAC : 3; // Max Available Current, 0: 6A, 1: 12A, 2: 18A, 3: 24A, 4: 30A, 5-7: Reserved
    uint32_t _RESERVED1 : 1;
    uint32_t STCIE : 1; // State Change Interrupt Enable,  0: Disable, 1: Enable
    uint32_t DFDIE : 1; // Diode Failed Detect Interrupt Enable,  0: Disable, 1: Enable
    uint32_t _RESERVED2 : 1;
    uint32_t SWRST : 1; // Software Reset, 0: Run, 1: Reset
  } CR; // Control Register
} pilotController_register;

#define SR_REGISTER (0x00)
#define CR_REGISTER (0x01)

typedef void (*OnStateChange_CB)(PilotState_t, PilotState_t);

class PilotController {
    private:
        bool writeRegister(uint8_t addr, uint8_t * value) ;
        bool readRegister(uint8_t addr, uint8_t * value) ;

        OnStateChange_CB onStateChange_cb = NULL;

    public: 
        int max_current = 6; // Default maximum current in Amperes

        PilotController() ;
        bool begin(MaxAvailableCurrent_t max_current = MAX_CURRENT_6A) ;
        void onStateChange(OnStateChange_CB cb) ;
        bool setMaxCurrent(MaxAvailableCurrent_t max_current) ;
        PilotState_t getLastState() ;
        bool loop() ;

};
