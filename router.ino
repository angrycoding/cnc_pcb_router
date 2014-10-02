#include "utility/stringbuffer.h"
#include "aJSON.h"

#define X_STEP_PIN 8
#define X_DIR_PIN 9
#define Y_STEP_PIN 10
#define Y_DIR_PIN 11
#define Z_STEP_PIN 12
#define Z_DIR_PIN 13
#define STEP_DELAY 3
#define STEPS_PER_MM 143

aJsonStream serial_stream(&Serial);

float STEPPER_X = 0;
float STEPPER_Y = 0;
float STEPPER_Z = -0.200 * STEPS_PER_MM;

void xStep(short dir) {
	if (dir == 1) {
		digitalWrite(X_DIR_PIN, HIGH);
		digitalWrite(X_STEP_PIN, HIGH);
		digitalWrite(X_STEP_PIN, LOW);
	} else if (dir == -1) {
		digitalWrite(X_DIR_PIN, LOW);
		digitalWrite(X_STEP_PIN, HIGH);
		digitalWrite(X_STEP_PIN, LOW);
	}
}

void yStep(short dir) {
	if (dir == 1) {
		digitalWrite(Y_DIR_PIN, HIGH);
		digitalWrite(Y_STEP_PIN, HIGH);
		digitalWrite(Y_STEP_PIN, LOW);
	} else if (dir == -1) {
		digitalWrite(Y_DIR_PIN, LOW);
		digitalWrite(Y_STEP_PIN, HIGH);
		digitalWrite(Y_STEP_PIN, LOW);
	}
}

void zStep(short dir) {
	if (dir == 1) {
		digitalWrite(Z_DIR_PIN, HIGH);
		digitalWrite(Z_STEP_PIN, HIGH);
		digitalWrite(Z_STEP_PIN, LOW);
	} else if (dir == -1) {
		digitalWrite(Z_DIR_PIN, LOW);
		digitalWrite(Z_STEP_PIN, HIGH);
		digitalWrite(Z_STEP_PIN, LOW);
	}
}

void lineTo(float x, float y, float z) {

	float deltaX = x - STEPPER_X;
	float deltaY = y - STEPPER_Y;
	float deltaZ = z - STEPPER_Z;

	float dirX = (deltaX > 0 ? 1 : deltaX < 0 ? -1 : 0);
	float dirY = (deltaY > 0 ? 1 : deltaY < 0 ? -1 : 0);
	float dirZ = (deltaZ > 0 ? 1 : deltaZ < 0 ? -1 : 0);

	deltaX = abs(deltaX);
	deltaY = abs(deltaY);
	deltaZ = abs(deltaZ);

	float stepsX = 0, stepsY = 0, stepsZ = 0;

	for (float c = 0; c < max(max(deltaX, deltaY), deltaZ); c++) {

		if (stepsX < deltaX) {
			stepsX++;
			xStep(dirX);
		}

		if (stepsY < deltaY) {
			stepsY++;
			yStep(dirY);
		}

		if (stepsZ < deltaZ) {
			stepsZ++;
			zStep(dirZ);
		}

		delay(STEP_DELAY);

	}

	STEPPER_X = x;
	STEPPER_Y = y;
	STEPPER_Z = z;
}


void processMessage(aJsonObject *packet) {

	aJsonObject *id = aJson.getObjectItem(packet, "id");
	aJsonObject *command = aJson.getObjectItem(packet, "command");

	String idStr = String(id->valuestring);
	String commandStr = String(command->valuestring);

	if (commandStr == "G00" || commandStr == "G01") {

		aJsonObject *x = aJson.getObjectItem(packet, "x");
		aJsonObject *y = aJson.getObjectItem(packet, "y");
		aJsonObject *z = aJson.getObjectItem(packet, "z");

		volatile float xPos = STEPPER_X, yPos = STEPPER_Y, zPos = STEPPER_Z;

		if (x->type == aJson_Float)
			xPos = ((float)x->valuefloat) * STEPS_PER_MM;
		else if (x->type == aJson_Int)
			xPos = ((float)x->valueint) * STEPS_PER_MM;


		if (y->type == aJson_Float)
			yPos = ((float)y->valuefloat) * STEPS_PER_MM;
		else if (y->type == aJson_Int)
			yPos = ((float)y->valueint) * STEPS_PER_MM;

		Serial.println(yPos);

		if (z->type == aJson_Float)
			zPos = ((float)z->valuefloat) * STEPS_PER_MM;
		else if (z->type == aJson_Int)
			zPos = ((float)z->valueint) * STEPS_PER_MM;


		lineTo(xPos, yPos, zPos);

	}


	// delay(10);
	Serial.println(idStr);

}

void setup() {
	Serial.begin(9600);
	pinMode(X_STEP_PIN, OUTPUT);
	pinMode(X_DIR_PIN, OUTPUT);
	pinMode(Y_STEP_PIN, OUTPUT);
	pinMode(Y_DIR_PIN, OUTPUT);
	pinMode(Z_STEP_PIN, OUTPUT);
	pinMode(Z_DIR_PIN, OUTPUT);
}

void loop() {

	if (serial_stream.available()) {
		/* First, skip any accidental whitespace like newlines. */
		serial_stream.skip();
	}

	if (serial_stream.available()) {
		/* Something real on input, let's take a look. */
		aJsonObject *msg = aJson.parse(&serial_stream);
		processMessage(msg);
		aJson.deleteItem(msg);
	}

}