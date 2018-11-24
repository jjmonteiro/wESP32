#pragma once

//**** PORTS SETUP
#define ANALOG_IN_0     36
#define ANALOG_IN_1     39
#define ANALOG_IN_2     34
#define ANALOG_IN_3     35
#define ANALOG_IN_4     32
#define ANALOG_IN_5     33

#define DIGITAL_OUT_3   25	//buzzer
#define DIGITAL_OUT_2   26
#define DIGITAL_OUT_1   27	//y led
#define DIGITAL_OUT_0   14	//r led

#define DIGITAL_IN_1    13
#define DIGITAL_IN_0    12


//**** DEVICE INFORMATION
#define MAX_OUTPUTS 3

class wDevice {
private:
    char*       deviceType;
    uint8_t     deviceLocation;
    uint16_t    outputTrigger[MAX_OUTPUTS];
    bool        outputStatus[MAX_OUTPUTS];
    bool        generalFault;
    bool        deviceIsolated;
    uint16_t    pollPeriod;

public:
    wDevice();
    uint16_t    getInput(uint8_t Input);

    bool        getOutput(uint8_t Output);
    void        setOutput(uint8_t Output, bool state);

    void        setOutputTrigger(uint8_t Output, uint16_t triggerValue);
    uint16_t    getOutputTrigger(uint8_t Output);
    char*       getType();
    void        setType(char* newType);
    uint8_t     getLocation();
    void        setLocation(uint8_t newLocation);
    uint16_t    getPollPeriod();
    void        setPollPeriod(uint16_t newPeriod);

    void        resetOutputs();

};

wDevice::wDevice(){
    this->deviceType        = "PH850";
    this->deviceLocation    = 1;
    this->generalFault      = false;
    this->pollPeriod        = 10000;
}

char* wDevice::getType() {
    return this->deviceType;
}

void wDevice::setType(char* newType) {
    this->deviceType = newType;
}

uint8_t wDevice::getLocation() {
    return this->deviceLocation;
}

void wDevice::setLocation(uint8_t newLocation) {
    this->deviceLocation = newLocation;
}

uint16_t wDevice::getInput(uint8_t Input){
    return analogRead(Input);
}

void wDevice::setOutput(uint8_t Output, bool state){
    digitalWrite(Output, state);
    this->outputStatus[Output] = state;
}

bool wDevice::getOutput(uint8_t Output){
    return (bool)this->outputStatus[Output];
}

void wDevice::setOutputTrigger(uint8_t Output, uint16_t value) {
    this->outputTrigger[Output] = value;
}

uint16_t wDevice::getOutputTrigger(uint8_t Output) {
    return this->outputTrigger[Output];
}

uint16_t wDevice::getPollPeriod() {
    return this->pollPeriod;
}

void wDevice::setPollPeriod(uint16_t newPeriod) {
    this->pollPeriod = newPeriod;
}

void wDevice::resetOutputs(){
    this->setOutput(DIGITAL_OUT_0, LOW);
    this->setOutput(DIGITAL_OUT_1, LOW);
    this->setOutput(DIGITAL_OUT_2, LOW);
    this->setOutput(DIGITAL_OUT_3, LOW);
}

