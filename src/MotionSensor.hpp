#include <Wire.h>

class MotionSensor
{
private:
    const char *topic;
    PCF8574 *sensorsBlock = nullptr;
    uint8_t pinMotion;
    uint8_t pinTamper;
    bool initialized = false;

    bool lastMotion = false;
    bool currentMotion = false;

    bool lastTamper = false;
    bool currentTamper = false;

    bool getTamperValue();
    bool getMotionValue();

public:
    MotionSensor();
    MotionSensor(const char *topic, PCF8574 *sensorsBlock, uint8_t pinMotion, uint8_t pinTamper);
    ~MotionSensor();

    bool readValues();
    bool isChanged();
    bool isInitialized();

    const char *getTopic();
    String generatePayloadJSON(DateTime time);
};

MotionSensor::MotionSensor(const char *topic, PCF8574 *sensorsBlock, uint8_t pinMotion, uint8_t pinTamper)
{
    this->topic = topic;
    this->sensorsBlock = sensorsBlock;
    this->pinMotion = pinMotion;
    this->pinTamper = pinTamper;

    sensorsBlock->pinMode(this->pinMotion, INPUT_PULLUP);
    sensorsBlock->pinMode(this->pinTamper, INPUT_PULLUP);

    this->initialized = true;
}

MotionSensor::MotionSensor()
{
}

MotionSensor::~MotionSensor()
{
}

const char *MotionSensor::getTopic()
{
    return this->topic;
}

bool MotionSensor::readValues () {
    this->lastMotion = this->currentMotion;
    this->lastTamper = this->currentTamper;

    this->currentMotion = this->getMotionValue();
    this->currentTamper = this->getTamperValue();

    Serial.print(this->topic);
    Serial.print(" - Tamper: ");
    Serial.print(this->currentTamper);

    Serial.print(" - Motion: ");
    Serial.println(this->currentMotion);

    return true;
}

bool MotionSensor::isChanged () {
    return (this->lastMotion != this->currentMotion) || (this->lastTamper != this->currentTamper);
}

bool MotionSensor::isInitialized () {
    return this->initialized;
}

String MotionSensor::generatePayloadJSON(DateTime time)
{
    DynamicJsonDocument doc(512);

    String json;
    doc["motion"] = this->currentMotion;
    doc["tamper"] = this->currentTamper;
    doc["time"] = time.timestamp();
    serializeJson(doc, json);

    return json;
}

bool MotionSensor::getMotionValue()
{
    if (!this->sensorsBlock) {
        return false;
    }

    return (bool) this->sensorsBlock->digitalRead(this->pinMotion);
}

bool MotionSensor::getTamperValue()
{
    if (!this->sensorsBlock) {
        return false;
    }

    return (bool) this->sensorsBlock->digitalRead(this->pinTamper);
}
