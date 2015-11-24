#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <thread>
#include <string>
#include <signal.h>

#include <grove.h>
#include <ublox6.h>
#include <rfr359f.h>

#include "../lib/restclient-cpp/include/restclient-cpp/restclient.h"

using namespace std;

const size_t bufferLength = 256;

// The hardware devices that the example is going to connect to
struct Devices
{
  upm::RFR359F* detector;
  upm::Ublox6* nmea;

  Devices(){
  };

  // Initialization function
  void init() {
    detector = new upm::RFR359F(2);
    nmea = new upm::Ublox6(0);
    initPort();
  };

  // Cleanup on exit
  void cleanup() {
    delete detector;
    delete nmea;
  }

  int initPort(){
    if (!nmea->setupTty(B9600)) {
      cerr << "Failed to setup tty port parameters" << endl;
      return 1;
    }
    return 0;
  }

	// Get GPS data
  string gps_info(){
    string result;
    char nmeaBuffer[bufferLength];

    if (nmea->dataAvailable()) {
      int rv = nmea->readData(nmeaBuffer, bufferLength);
      if (rv > 0) {
        result = nmeaBuffer;
        return result;
      }

      if (rv < 0) {
        cerr << "Port read error." << endl;
        return "GPS Error";
      }
    }

    return "No GPS Data";
  }

	// Notify remote datastore where there is a close call
  void notify(string message, string location) {
    cout << message << endl;
    cout << location << endl;

    if (!getenv("SERVER") || !getenv("AUTH_TOKEN")) {
      return;
    }

    time_t now = time(NULL);
    char mbstr[sizeof "2011-10-08T07:07:09Z"];
    strftime(mbstr, sizeof(mbstr), "%FT%TZ", localtime(&now));

    stringstream payload;
    payload << "{";
    payload << "\"message\":";
    payload << "\"" << message << " " << mbstr << "\"";
    payload << "\"location\":";
    payload << "\"" << location << "\"";
    payload << "}";

    RestClient::headermap headers;
    headers["X-Auth-Token"] = getenv("AUTH_TOKEN");

    RestClient::response r = RestClient::put(getenv("SERVER"), "text/json", payload.str(), headers);
    cout << "Datastore called. Result:" << r.code << endl;
    cout << r.body << endl;
  }

	// Check to see if any object is within range
  void check_object_detected(){
    if (detector->objectDetected()) {
      notify("object-detected", gps_info());
    } else {
      cout << "Area is clear" << endl;
    }
  }
};

Devices devices;

// Exit handler for program
void exit_handler(int param)
{
  devices.cleanup();
  exit(1);
}

// The main function for the example program
int main() {
  // Handles ctrl-c or other orderly exits
  signal(SIGINT, exit_handler);

  // check that we are running on Galileo or Edison
  mraa_platform_t platform = mraa_get_platform_type();
  if ((platform != MRAA_INTEL_GALILEO_GEN1) &&
    (platform != MRAA_INTEL_GALILEO_GEN2) &&
    (platform != MRAA_INTEL_EDISON_FAB_C)) {
    cerr << "ERROR: Unsupported platform" << endl;
    return MRAA_ERROR_INVALID_PLATFORM;
  }

  // create and initialize UPM devices
  devices.init();

	for (;;) {
    devices.check_object_detected();
    sleep(5);
  }

  return MRAA_SUCCESS;
}
