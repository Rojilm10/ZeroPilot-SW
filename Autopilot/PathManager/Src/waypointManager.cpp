/**
 * Waypoint Manager Methods and Helpers Implementation
 * Author: Dhruv Rawat
 * Created: November 2020
 * Last Updated: December 2020 (Dhruv)
 */

#include "waypointManager.hpp"

#define LINE_FOLLOWING 0
#define ORBIT_FOLLOWING 1

//Constants
#define EARTH_RADIUS 6378.137
#define MAX_PATH_APPROACH_ANGLE M_PI/2

//Basic Mathematical Conversions
#define deg2rad(angle_in_degrees) ((angle_in_degrees) * M_PI/180.0)
#define rad2deg(angle_in_radians) ((angle_in_radians) * 180.0/M_PI)

static float k_gain[2] = {0.01, 1.0f};

/*** INITIALIZATION ***/


WaypointManager::WaypointManager(float relLat, float relLong) {
    nextAssignedId = 0;

    #if UNIT_TESTING
        // // std::cout << "Hewwo" << std::endl;
        currentIndex = 2;
    #else 
        currentIndex = 0;
    #endif

    // Sets relative long and lat
    relativeLongitude = relLong;
    relativeLatitude = relLat;

    // Sets boolean variables
    inHold = false;
    goingHome = false;
    dataIsNew = false;
    orbitPathStatus = PATH_FOLLOW;

    for(int i = 0; i < PATH_BUFFER_SIZE; i++) {
        waypointBufferStatus[i] = FREE;
    }
}

_WaypointStatus WaypointManager::initialize_flight_path(_PathData ** initialWaypoints, int numberOfWaypoints, _PathData *currentLocation) {
    
    // The waypointBuffer array must be empty before we initialize the flight path
    if (numWaypoints != 0) {
        errorStatus = UNDEFINED_FAILURE;
        return errorStatus;
    }
    
    homeBase = currentLocation;
    
    numWaypoints = numberOfWaypoints;

    nextFilledIndex = 0;

    // Initializes the waypointBuffer array
    for (int i = 0; i < numWaypoints; i++) {
        waypointBuffer[i] = initialWaypoints[i]; // Sets the element in the waypointBuffer
        waypointBufferStatus[i] = FULL;
        nextFilledIndex = i + 1;
    }

    // Links waypoints together
    for (int i = 0; i < numWaypoints; i++) {
        if (i == 0) { // If first waypoint, link to next one only
            waypointBuffer[i]->next = waypointBuffer[i+1];
            waypointBuffer[i]->previous = nullptr;
        } else if (i == numWaypoints - 1) { // If last waypoint, link to previous one only
            waypointBuffer[i]->next = nullptr;
            waypointBuffer[i]->previous = waypointBuffer[i-1];
        } else {
            waypointBuffer[i]->next = waypointBuffer[i+1];
            waypointBuffer[i]->previous = waypointBuffer[i-1];
        }
    }

    for(int i = numWaypoints; i < PATH_BUFFER_SIZE; i++) {
        waypointBuffer[i] = nullptr;
    }

    errorStatus = WAYPOINT_SUCCESS;
    return errorStatus;
}

_WaypointStatus WaypointManager::initialize_flight_path(_PathData ** initialWaypoints, int numberOfWaypoints) {

    // The waypointBuffer array must be empty before we initialize the flight path
    if (numWaypoints != 0) {
        errorStatus = UNDEFINED_FAILURE;
        return errorStatus;
    }

    #if UNIT_TESTING
        currentIndex = 2;
    #endif

    numWaypoints = numberOfWaypoints;
    nextFilledIndex = 0;

    // Initializes the waypointBuffer array
    for (int i = 0; i < numWaypoints; i++) {
        waypointBuffer[i] = initialWaypoints[i]; // Sets the element in the waypointBuffer
        waypointBufferStatus[i] = FULL;
        nextFilledIndex = i + 1;
    }

    // Links waypoints together
    for (int i = 0; i < numWaypoints; i++) {
        if (i == 0) { // If first waypoint, link to next one only
            waypointBuffer[i]->next = waypointBuffer[i+1];
            waypointBuffer[i]->previous = nullptr;
        } else if (i == numWaypoints - 1) { // If last waypoint, link to previous one only
            waypointBuffer[i]->next = nullptr;
            waypointBuffer[i]->previous = waypointBuffer[i-1];
        } else {
            waypointBuffer[i]->next = waypointBuffer[i+1];
            waypointBuffer[i]->previous = waypointBuffer[i-1];
        }
    }

    for(int i = numWaypoints; i < PATH_BUFFER_SIZE; i++) {
        waypointBuffer[i] = nullptr;
    }
    

    errorStatus = WAYPOINT_SUCCESS;
    return errorStatus;
}

_PathData* WaypointManager::initialize_waypoint() {
    _PathData* waypoint = new _PathData; // Create new waypoint in the heap
    waypoint->waypointId = nextAssignedId++; // Set ID and increment
    waypoint->latitude = -1;
    waypoint->longitude = -1;
    waypoint->altitude = -1;
    waypoint->waypointType = PATH_FOLLOW;
    waypoint->turnRadius = -1;
    // Set next and previous waypoints to empty for now
    waypoint->next = nullptr;
    waypoint->previous = nullptr;

    return waypoint;
}

_PathData* WaypointManager::initialize_waypoint(long double longitude, long double latitude, int altitude, _WaypointOutputType waypointType) {
    _PathData* waypoint = new _PathData; // Create new waypoint in the heap
    waypoint->waypointId = nextAssignedId; // Set ID and increment
    nextAssignedId++;
    waypoint->latitude = latitude;
    waypoint->longitude = longitude;
    waypoint->altitude = altitude;
    waypoint->waypointType = waypointType;
    waypoint->turnRadius = -1;
    // Set next and previous waypoints to empty for now
    waypoint->next = nullptr;
    waypoint->previous = nullptr;

    return waypoint;
}

_PathData* WaypointManager::initialize_waypoint(long double longitude, long double latitude, int altitude, _WaypointOutputType waypointType, float turnRadius) {
    _PathData* waypoint = new _PathData; // Create new waypoint in the heap
    waypoint->waypointId = nextAssignedId; // Set ID and increment
    nextAssignedId++;
    waypoint->latitude = latitude;
    waypoint->longitude = longitude;
    waypoint->altitude = altitude;
    waypoint->waypointType = waypointType;
    waypoint->turnRadius = turnRadius;
    // Set next and previous waypoints to empty for now
    waypoint->next = nullptr;
    waypoint->previous = nullptr;

    return waypoint;
}


/*** UNIVERSAL HELPERS (universal to this file, ofc) ***/


int WaypointManager::get_waypoint_index_from_id(int waypointId) {
    for (int i = 0; i < PATH_BUFFER_SIZE; i++) {
        if(waypointBufferStatus[i] == FREE) { // If array is empty at the specified index, waypoint is not in buffer
            return -1;
        }
        if(waypointBuffer[i]->waypointId == waypointId) { // If waypoint is found, return index
            return i;
        }
    }

    return -1; // If waypoint is not found
}

void WaypointManager::get_coordinates(long double longitude, long double latitude, float* xyCoordinates) { // Parameters expected to be in degrees
    xyCoordinates[0] = get_distance(relativeLatitude, relativeLongitude, relativeLatitude, longitude); //Calculates longitude (x coordinate) relative to defined origin (RELATIVE_LONGITUDE, RELATIVE_LATITUDE)
    xyCoordinates[1] = get_distance(relativeLatitude, relativeLongitude, latitude, relativeLongitude); //Calculates latitude (y coordinate) relative to defined origin (RELATIVE_LONGITUDE, RELATIVE_LATITUDE)
}

float WaypointManager::get_distance(long double lat1, long double lon1, long double lat2, long double lon2) { // Parameters expected to be in degrees
    // Longitude and latitude stored in degrees
    // This calculation uses the Haversine formula
    long double change_in_Lat = deg2rad(lat2 - lat1); //Converts change in latitude to radians
    long double change_in_lon = deg2rad(lon2 - lon1); //Converts change in longitude to radians

    double haversine_ans = sin(change_in_Lat / 2) * sin(change_in_Lat / 2) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * sin(change_in_lon / 2) * sin(change_in_lon / 2); // In kilometers

    if ((change_in_Lat >= 0 && change_in_lon >=0)||(change_in_Lat < 0 && change_in_lon < 0)){
        return EARTH_RADIUS * (2 * atan2(sqrt(haversine_ans),sqrt(1 - haversine_ans))) * 1000; //Multiply by 1000 to convert to metres
    } else { // If result is negative.
        return EARTH_RADIUS * (2 * atan2(sqrt(haversine_ans),sqrt(1 - haversine_ans))) * -1000;
    }
}

_WaypointStatus WaypointManager::change_current_index(int id) {
    int waypointIndex = get_waypoint_index_from_id(id); // Gets index of waypoint in waypointBuffer array

    if (waypointIndex == -1 || waypointBuffer[waypointIndex]->next == nullptr || waypointBuffer[waypointIndex]->next->next == nullptr) { // If waypoint with set id does not exist. Or if the next waypoint or next to next waypoints are not defined. 
        return INVALID_PARAMETERS;
    }

    currentIndex = waypointIndex; // If checks pass, then the current index value is updated
    
    return WAYPOINT_SUCCESS;
}


/*** NAVIGATION ***/


_WaypointStatus WaypointManager::get_next_directions(_WaypointManager_Data_In currentStatus, _WaypointManager_Data_Out *Data) {

    errorCode = WAYPOINT_SUCCESS;

    // Gets current position
    float position[3]; 

    // Gets current heading
    float currentHeading = (float) currentStatus.heading;

    // Holding is given higher priority to heading home
    if (inHold) {   // If plane is currently circling and waiting for commands
        if(turnRadius <= 0 || turnDirection < -1 || turnDirection > 1) {
            return INVALID_PARAMETERS;
        }

        // Sets position array
        position[0] = currentStatus.longitude;
        position[1] = currentStatus.latitude;
        position[2] = (float) currentStatus.altitude;

        // Calculates desired heading 
        follow_hold_pattern(position, currentHeading);

        // Updates the return structure
        outputType = ORBIT_FOLLOW;
        dataIsNew = true;
        update_return_data(Data); 

        return errorCode;
    }

    // Sets position array
    get_coordinates(currentStatus.longitude, currentStatus.latitude, position);
    position[2] = (float) currentStatus.altitude;

    if (goingHome) { // If plane was instructed to go back to base (and is awaiting for waypointBuffer to be updated)
        if (!homeBase) {
            return UNDEFINED_PARAMETER;
        }

        // Creates a path data object to represent current position
        _PathData * currentPosition = new _PathData;
        currentPosition->latitude = currentStatus.latitude;
        currentPosition->longitude = currentStatus.longitude;
        currentPosition->altitude = currentStatus.altitude;
        currentPosition->turnRadius = -1;
        currentPosition->waypointType = PATH_FOLLOW;
        currentPosition->previous = nullptr;
        currentPosition->next = homeBase;

        // Updates home base object accordingly
        homeBase->previous = currentPosition;
        homeBase->next = nullptr;
        homeBase->waypointType = HOLD_WAYPOINT;

        follow_waypoints(currentPosition, position, currentHeading);
        
        // Updates the return structure
        dataIsNew = true;
        update_return_data(Data); 

        // Removes currentPosition path data object from memory
        homeBase->previous = nullptr;
        delete currentPosition; 

        return errorCode;
    }

    if (numWaypoints - currentIndex < 1 && numWaypoints >= 0) { // Ensures that the currentIndex parameter will not cause a segmentation fault
        return CURRENT_INDEX_INVALID;
    }

    // Calls method to follow waypoints
    follow_waypoints(waypointBuffer[currentIndex], position, currentHeading);

    // std::cout << "Normal nav" << std::endl;

    // Updates the return structure
    dataIsNew = true;
    update_return_data(Data); 

    return errorCode;
}

void WaypointManager::update_return_data(_WaypointManager_Data_Out *Data) {
    Data->desiredHeading = desiredHeading;
    Data->desiredAltitude =  desiredAltitude;
    Data->distanceToNextWaypoint = distanceToNextWaypoint;
    Data->radius = turnRadius;
    Data->turnDirection = turnDirection;
    Data->errorCode = errorCode;
    Data->isDataNew = dataIsNew;
    dataIsNew = false;
    Data->timeOfData = 0;
    Data->out_type = outputType;

    // Not setting time of data yet bc I think we need to come up with a way to get it???
}

void WaypointManager::start_circling(_WaypointManager_Data_In currentStatus, float radius, int direction, int altitude, bool cancelTurning) {
    if (!cancelTurning) {
        inHold = true;

        turnDesiredAltitude = altitude;
        turnRadius = radius;
        turnDirection = direction;

        // Gets current heading
        float currentHeading = (float) currentStatus.heading;

        // Gets current position
        float position[3]; 
        position[0] = deg2rad(currentStatus.longitude);
        position[1] = deg2rad(currentStatus.latitude);
        position[2] = (float) currentStatus.altitude;

        turnCenter[2] = turnDesiredAltitude;
        float turnCenterBearing = 0.0f; // Bearing of line pointing to the center point of the turn

        if (turnDirection == -1) { // CW
            turnCenterBearing = currentHeading + 90;
        } else if (turnDirection == 1) { // CCW
            turnCenterBearing = currentHeading - 90;
        }

        // Normalizes heading (keeps it between 0.0 and 259.9999)
        while (turnCenterBearing >= 360.0) {
            turnCenterBearing -= 360.0; // IF THERE IS A WAY TO DO THIS WITHOUT A WHILE LOOP PLS LMK
        }

        while (turnCenterBearing < 0.0) {
            turnCenterBearing += 360.0; // IF THERE IS A WAY TO DO THIS WITHOUT A WHILE LOOP PLS LMK
        }

        float angularDisplacement = turnRadius / (EARTH_RADIUS * 1000);

        double turnCenterBearing_Radians = deg2rad(turnCenterBearing);

        // Calculates latitude and longitude of end coordinates (Calculations taken from here: http://www.movable-type.co.uk/scripts/latlong.html#destPoint)
        turnCenter[1] = asin(sin(position[1]) * cos(angularDisplacement) + cos(position[1]) * sin(angularDisplacement) * cos(turnCenterBearing_Radians)); // latitude
        turnCenter[0] = position[0] + atan2(sin(turnCenterBearing_Radians) * sin(angularDisplacement) * cos(position[1]), cos(angularDisplacement) - sin(position[1]) * sin(turnCenter[1])); // Longitude

        #ifdef UNIT_TESTING
            orbitCentreLong = rad2deg(turnCenter[0]);
            orbitCentreLat = rad2deg(turnCenter[1]);
            orbitCentreAlt = turnCenter[2];

            // std::cout << "Check 1: Lat - " << orbitCentreLat << " " << orbitCentreLong << std::endl;
        #endif

        get_coordinates(rad2deg(turnCenter[0]), rad2deg(turnCenter[1]), turnCenter);
    } else {
        inHold = false;
    }
}

bool WaypointManager::head_home() {
    if (homeBase == nullptr) { // Checks if home waypoint is actually initialized.
        return false;
    }

    if (!goingHome) {
        clear_path_nodes(); // Clears path nodes so state machine can input new flight path
        goingHome = true;
        return true;
    } else {
        goingHome = false;
        return false;
    }
}

void WaypointManager::follow_hold_pattern(float* position, float heading) {
    // Converts the position array and turnCenter array from radians to an xy coordinate system.
    get_coordinates(position[0], position[1], position);

    // // std::cout << "Check 2: Lat - " << turnCenter[1] << " " << turnCenter[0] << std::endl;
    // // std::cout << "Lat - " << position[1] << " " << position[0] << " " << heading << std::endl;

    // Calls follow_orbit method 
    follow_orbit(position, heading);
}

void WaypointManager::follow_waypoints(_PathData * currentWaypoint, float* position, float heading) {
    float waypointPosition[3]; 
    get_coordinates(currentWaypoint->longitude, currentWaypoint->latitude, waypointPosition);
    waypointPosition[2] = currentWaypoint->altitude;

    if (currentWaypoint->next == nullptr) { // If target waypoint is not defined
        // std::cout << "Next not defined" << std::endl;
        follow_last_line_segment(currentWaypoint, position, heading);
        return;
    }
    if (currentWaypoint->next->next == nullptr) { // If waypoint after target waypoint is not defined
        // std::cout << "Next to next not defined" << std::endl;
        follow_line_segment(currentWaypoint, position, heading);
        return;
    }

    // Defines target waypoint
    _PathData * targetWaypoint = currentWaypoint->next;
    float targetCoordinates[3];
    get_coordinates(targetWaypoint->longitude, targetWaypoint->latitude, targetCoordinates);
    targetCoordinates[2] = targetWaypoint->altitude;

    // Defines waypoint after target waypoint
    _PathData* waypointAfterTarget = targetWaypoint->next;
    float waypointAfterTargetCoordinates[3];
    get_coordinates(waypointAfterTarget->longitude, waypointAfterTarget->latitude, waypointAfterTargetCoordinates);
    waypointAfterTargetCoordinates[2] = waypointAfterTarget->altitude;

    // Gets the unit vectors representing the direction towards the target waypoint
    float waypointDirection[3];
    float norm = sqrt(pow(targetCoordinates[0] - waypointPosition[0],2) + pow(targetCoordinates[1] - waypointPosition[1],2) + pow(targetCoordinates[2] - waypointPosition[2],2));
    waypointDirection[0] = (targetCoordinates[0] - waypointPosition[0])/norm;
    waypointDirection[1] = (targetCoordinates[1] - waypointPosition[1])/norm;
    waypointDirection[2] = (targetCoordinates[2] - waypointPosition[2])/norm;

    // std::cout << "Here1.1 --> " << position[0] << " " << position[1] << " " << position[2] << std::endl;
    // std::cout << "Here1.2 --> " << targetCoordinates[0] << " " << targetCoordinates[1] << " " << targetCoordinates[2] << std::endl;
    // std::cout << "Here1.3 --> " << waypointPosition[0] << " " << waypointPosition[1] << " " << waypointPosition[2] << std::endl;

    // Gets the unit vectors representing the direction vector from the target waypoint to the waypoint after the target waypoint 
    float nextWaypointDirection[3];
    float norm2 = sqrt(pow(waypointAfterTargetCoordinates[0] - targetCoordinates[0],2) + pow(waypointAfterTargetCoordinates[1] - targetCoordinates[1],2) + pow(waypointAfterTargetCoordinates[2] - targetCoordinates[2],2));
    nextWaypointDirection[0] = (waypointAfterTargetCoordinates[0] - targetCoordinates[0])/norm2;
    nextWaypointDirection[1] = (waypointAfterTargetCoordinates[1] - targetCoordinates[1])/norm2;
    nextWaypointDirection[2] = (waypointAfterTargetCoordinates[2] - targetCoordinates[2])/norm2;

    float turningAngle = acos(-deg2rad(waypointDirection[0] * nextWaypointDirection[0] + waypointDirection[1] * nextWaypointDirection[1] + waypointDirection[2] * nextWaypointDirection[2]));
    float tangentFactor = targetWaypoint->turnRadius/tan(turningAngle/2);

    float halfPlane[3];
    halfPlane[0] = targetCoordinates[0] - tangentFactor * waypointDirection[0];
    halfPlane[1] = targetCoordinates[1] - tangentFactor * waypointDirection[1];
    halfPlane[2] = targetCoordinates[2] - tangentFactor * waypointDirection[2];

    // Calculates distance to next waypoint
    float distanceToWaypoint = sqrt(pow(targetCoordinates[0] - position[0],2) + pow(targetCoordinates[1] - position[1],2) + pow(targetCoordinates[2] - position[2],2));
    distanceToNextWaypoint = distanceToWaypoint; // Stores distance to next waypoint :))
    // std::cout << "Distance = " << distanceToWaypoint << std::endl;

    if (orbitPathStatus == PATH_FOLLOW) {
        float dotProduct = waypointDirection[0] * (position[0] - halfPlane[0]) + waypointDirection[1] * (position[1] - halfPlane[1]) + waypointDirection[2] * (position[2] - halfPlane[2]);
        
        if (dotProduct > 0){
            orbitPathStatus = ORBIT_FOLLOW;
            if (targetWaypoint->waypointType == HOLD_WAYPOINT) {
                inHold = true;
                turnDirection = 1; // Automatically turn CCW
                turnRadius = targetWaypoint->turnRadius;
                turnDesiredAltitude = targetWaypoint->altitude;
                turnCenter[0] = targetWaypoint->longitude;
                turnCenter[1] = targetWaypoint->latitude;
                turnCenter[2] = turnDesiredAltitude;
            }
        }

        // std::cout << "Here2 --> " << position[0] << " " << position[1] << " " << position[2] << std::endl;
        // std::cout << "Here3 --> " << waypointDirection[0] << " " << waypointDirection[1] << " " << waypointDirection[2] << std::endl;
        // std::cout << "Here4 --> " << targetCoordinates[0] << " " << targetCoordinates[1] << " " << targetCoordinates[2] << std::endl;

        follow_straight_path(waypointDirection, targetCoordinates, position, heading);
    } else {
        // Determines turn direction (CCW returns 2; CW returns 1)
        turnDirection = waypointDirection[0] * nextWaypointDirection[1] - waypointDirection[1] * nextWaypointDirection[0]>0?1:-1;
        
        // Since the Earth is not flat *waits for the uproar to die down* we need to do some fancy geometry. Introducing!!!!!!!!!! EUCLIDIAN GEOMETRY! (translation: I have no idea what this line does but it should work)
        float euclideanWaypointDirection = sqrt(pow(nextWaypointDirection[0] - waypointDirection[0],2) + pow(nextWaypointDirection[1] - waypointDirection[1],2) + pow(nextWaypointDirection[2] - waypointDirection[2],2)) * ((nextWaypointDirection[0] - waypointDirection[0]) < 0?-1:1) * ((nextWaypointDirection[1] - waypointDirection[1]) < 0?-1:1) * ((nextWaypointDirection[2] - waypointDirection[2]) < 0?-1:1);

        // Determines coordinates of the turn center
        turnCenter[0] = targetCoordinates[0] + (tangentFactor * (nextWaypointDirection[0] - waypointDirection[0])/euclideanWaypointDirection);
        turnCenter[1] = targetCoordinates[1] + (tangentFactor * (nextWaypointDirection[1] - waypointDirection[1])/euclideanWaypointDirection);
        turnCenter[2] = targetCoordinates[2] + (tangentFactor * (nextWaypointDirection[2] - waypointDirection[2])/euclideanWaypointDirection);

        // if target waypoint is a hold waypoint the plane will follow the orbit until start_circling is called again
        if (inHold == true) {
            follow_orbit(position, heading);
            return;
        }

        float dotProduct = nextWaypointDirection[0] * (position[0] - halfPlane[0]) + nextWaypointDirection[1] * (position[1] - halfPlane[1]) + nextWaypointDirection[2] * (position[2] - halfPlane[2]);
        if (dotProduct > 0){
            orbitPathStatus = PATH_FOLLOW;
        }

        //If two waypoints are parallel to each other (no turns)
        if (euclideanWaypointDirection == 0){
            orbitPathStatus = PATH_FOLLOW;
        }

        outputType = ORBIT_FOLLOW;

        follow_orbit(position, heading);
    }
}

void WaypointManager::follow_line_segment(_PathData * currentWaypoint, float* position, float heading) {
    float waypointPosition[3];
    get_coordinates(currentWaypoint->longitude, currentWaypoint->latitude, waypointPosition);
    waypointPosition[2] = currentWaypoint->altitude;

    _PathData * targetWaypoint = currentWaypoint->next;
    float targetCoordinates[3];
    get_coordinates(targetWaypoint->longitude, targetWaypoint->latitude, targetCoordinates);
    targetCoordinates[2] = targetWaypoint->altitude;

    float waypointDirection[3];
    float norm = sqrt(pow(targetCoordinates[0] - waypointPosition[0],2) + pow(targetCoordinates[1] - waypointPosition[1],2) + pow(targetCoordinates[2] - waypointPosition[2],2));
    waypointDirection[0] = (targetCoordinates[0] - waypointPosition[0])/norm;
    waypointDirection[1] = (targetCoordinates[1] - waypointPosition[1])/norm;
    waypointDirection[2] = (targetCoordinates[2] - waypointPosition[2])/norm;

    // Calculates distance to next waypoint
    float distanceToWaypoint = sqrt(pow(targetCoordinates[0] - position[0],2) + pow(targetCoordinates[1] - position[1],2) + pow(targetCoordinates[2] - position[2],2));
    distanceToNextWaypoint = distanceToWaypoint; // Stores distance to next waypoint :))

    // std::cout << "Here1.1 --> " << waypointDirection[0] << " " << waypointDirection[1] << " " << waypointDirection[2] << std::endl;
    // std::cout << "Here1.2 --> " << targetCoordinates[0] << " " << targetCoordinates[1] << " " << targetCoordinates[2] << std::endl;
    // std::cout << "Here1.3 --> " << waypointPosition[0] << " " << waypointPosition[1] << " " << waypointPosition[2] << std::endl;
    // std::cout << heading << std::endl;

    follow_straight_path(waypointDirection, targetCoordinates, position, heading);
}

void WaypointManager::follow_last_line_segment(_PathData * currentWaypoint, float* position, float heading) {
    float waypointPosition[3];
    waypointPosition[0] = position[0];
    waypointPosition[1] = position[1];
    waypointPosition[2] = position[2];

    _PathData * targetWaypoint = currentWaypoint;
    float targetCoordinates[3];
    get_coordinates(targetWaypoint->longitude, targetWaypoint->latitude, targetCoordinates);
    targetCoordinates[2] = targetWaypoint->altitude;

    float waypointDirection[3];
    float norm = sqrt(pow(targetCoordinates[0] - waypointPosition[0],2) + pow(targetCoordinates[1] - waypointPosition[1],2) + pow(targetCoordinates[2] - waypointPosition[2],2));
    waypointDirection[0] = (targetCoordinates[0] - waypointPosition[0])/norm;
    waypointDirection[1] = (targetCoordinates[1] - waypointPosition[1])/norm;
    waypointDirection[2] = (targetCoordinates[2] - waypointPosition[2])/norm;

    // Calculates distance to next waypoint
    float distanceToWaypoint = sqrt(pow(targetCoordinates[0] - position[0],2) + pow(targetCoordinates[1] - position[1],2) + pow(targetCoordinates[2] - position[2],2));
    distanceToNextWaypoint = distanceToWaypoint; // Stores distance to next waypoint :))

    float dotProduct = waypointDirection[0] * (position[0] - targetCoordinates[0]) + waypointDirection[1] * (position[1] - targetCoordinates[1]) + waypointDirection[2] * (position[2] - targetCoordinates[2]);
    
    if (dotProduct > 0){
        inHold = true;
        turnDirection = 1; // Automatically turn CCW
        turnRadius = 50;
        turnDesiredAltitude = targetWaypoint->altitude;
        turnCenter[0] = targetWaypoint->longitude;
        turnCenter[1] = targetWaypoint->latitude;
        turnCenter[2] = turnDesiredAltitude; 
    }

    follow_straight_path(waypointDirection, targetCoordinates, position, heading);
}

void WaypointManager::follow_orbit(float* position, float heading) {
    float currentHeading = deg2rad(90 - heading);

    float orbitDistance = sqrt(pow(position[0] - turnCenter[0],2) + pow(position[1] - turnCenter[1],2));
    float courseAngle = atan2(position[1] - turnCenter[1], position[0] - turnCenter[0]); // (y,x) format

    // Gets angle between plane and line connecting it and center of orbit
    while (courseAngle - currentHeading < - M_PI){
        courseAngle += 2 * M_PI;
    }
    while (courseAngle - currentHeading > M_PI){
        courseAngle -= 2 * M_PI;
    }

    // This line is causing some problems
    int calcHeading = round(90 - rad2deg(courseAngle + turnDirection * (M_PI/2 + atan(k_gain[ORBIT_FOLLOW] * (orbitDistance - turnRadius)/turnRadius)))); //Heading in degrees (magnetic)
    
    // Normalizes heading (keeps it between 0.0 and 259.9999)
    while (calcHeading >= 360.0) {
        calcHeading -= 360.0; // IF THERE IS A WAY TO DO THIS WITHOUT A WHILE LOOP PLS LMK
    }

    while (calcHeading < 0.0) {
        calcHeading += 360.0; // IF THERE IS A WAY TO DO THIS WITHOUT A WHILE LOOP PLS LMK
    }
    
    // Sets the return values
    desiredHeading = calcHeading;
    distanceToNextWaypoint = 0.0;
    outputType = ORBIT_FOLLOW;
    desiredAltitude = turnDesiredAltitude;
}

void WaypointManager::follow_straight_path(float* waypointDirection, float* targetWaypoint, float* position, float heading) {
    heading = deg2rad(90 - heading);//90 - heading = magnetic heading to cartesian heading
    float courseAngle = atan2(waypointDirection[1], waypointDirection[0]); // (y,x) format
    
    // Adjusts angle values so they are within -pi and pi inclusive
    while (courseAngle - heading < -M_PI){
        courseAngle += 2 * M_PI;
    }
    while (courseAngle - heading > M_PI){
        courseAngle -= 2 * M_PI;
    }

    float pathError = -sin(courseAngle) * (position[0] - targetWaypoint[0]) + cos(courseAngle) * (position[1] - targetWaypoint[1]);
    int calcHeading = 90 - rad2deg(courseAngle - MAX_PATH_APPROACH_ANGLE * 2/M_PI * atan(k_gain[PATH_FOLLOW] * pathError)); //Heading in degrees (magnetic) 
    
    // Normalizes heading (keeps it between 0.0 and 259.9999)
    while (calcHeading >= 360.0) {
        calcHeading -= 360.0; // IF THERE IS A WAY TO DO THIS WITHOUT A WHILE LOOP PLS LMK
    }

    while (calcHeading < 0.0) {
        calcHeading += 360.0; // IF THERE IS A WAY TO DO THIS WITHOUT A WHILE LOOP PLS LMK
    }
    
    // Sets the return values 
    desiredHeading = calcHeading;
    outputType = PATH_FOLLOW;
    desiredAltitude = targetWaypoint[2];

    if (!inHold) {
        turnRadius = 0;
        turnDirection = 0;
    }
    // std::cout << "Done!" << std::endl;
}


/*** FLIGHT PATH MANAGEMENT ***/


_WaypointStatus WaypointManager::update_path_nodes(_PathData * waypoint, _WaypointBufferUpdateType updateType, int waypointId, int previousId, int nextId) {
    if (numWaypoints == PATH_BUFFER_SIZE && (updateType == APPEND_WAYPOINT || updateType == INSERT_WAYPOINT)) { // If array is already full, if we insert it will overflow
        return INVALID_PARAMETERS;
    }

    // Conducts a different operation based on the update type
    if (updateType == APPEND_WAYPOINT) {
        errorCode = append_waypoint(waypoint);
    } else if (updateType == INSERT_WAYPOINT) {
        errorCode = insert_new_waypoint(waypoint, previousId, nextId);
    } else if (updateType == UPDATE_WAYPOINT) {
        errorCode = update_waypoint(waypoint, waypointId);
    } else if (updateType == DELETE_WAYPOINT) {
        errorCode = delete_waypoint(waypointId);
    }
    
    return errorCode;
}

void WaypointManager::clear_path_nodes() {
    for(int i = 0; i < PATH_BUFFER_SIZE; i++) {
        if (waypointBufferStatus[i] == FULL) { // If array element has a waypoint in it
            int destroyedId = destroy_waypoint(waypointBuffer[i]); // Remove waypoint from the heap
        }
        waypointBufferStatus[i] = FREE; // Set array element free
        waypointBuffer[i] = nullptr; //Set buffer element to empty struct
    }
    // Resets buffer status variables
    numWaypoints = 0;
    nextFilledIndex = 0;
    nextAssignedId = 0;
    currentIndex = 0;
}

int WaypointManager::destroy_waypoint(_PathData *waypoint) {
    int destroyedId = waypoint->waypointId;
    waypoint->next = nullptr;
    waypoint->previous = nullptr;
    delete waypoint; // Free heap memory
    return destroyedId;
}

_WaypointStatus WaypointManager::append_waypoint(_PathData * newWaypoint) {
    int previousIndex = 0;

    previousIndex = nextFilledIndex - 1;

    // Before initializing elements, checks if new waypoint is not a duplicate
    if (previousIndex != -1 && waypointBuffer[previousIndex]->latitude == newWaypoint->latitude && waypointBuffer[previousIndex]->longitude == newWaypoint->longitude) {
        return INVALID_PARAMETERS;
    }

    waypointBuffer[nextFilledIndex] = newWaypoint;
    waypointBufferStatus[nextFilledIndex] = FULL;

    if (previousIndex == -1) { //If we are initializing the first element
        nextFilledIndex++;
        numWaypoints++;

        return WAYPOINT_SUCCESS;
    }

    // Links previous waypoint with current one
    waypointBuffer[nextFilledIndex]->previous = waypointBuffer[previousIndex];
    waypointBuffer[previousIndex]->next = waypointBuffer[nextFilledIndex];

    nextFilledIndex++;
    numWaypoints++;

    return WAYPOINT_SUCCESS;
}

_WaypointStatus WaypointManager::insert_new_waypoint(_PathData* newWaypoint, int previousId, int nextId) {
    int nextIndex = get_waypoint_index_from_id(nextId);
    int previousIndex = get_waypoint_index_from_id(previousId);

    // If any of the waypoints could not be found. Or, if the two IDs do not correspond to adjacent elements in waypointBuffer[]
    if (nextIndex == -1 || previousIndex == -1 || nextIndex - 1 != previousIndex || nextIndex == 0){
        return INVALID_PARAMETERS;
    }

    // Adjusts array. Starts at second last element
    for (int i = PATH_BUFFER_SIZE - 2; i >= nextIndex; i--) {
        if (waypointBufferStatus[i] == FULL) { // If current element is initialized
            waypointBuffer[i+1] = waypointBuffer[i]; // Sets next element to current element
            waypointBufferStatus[i+1] = FULL; // Updates state array
        }
    }

    // Put new waypoint in buffer
    waypointBuffer[nextIndex] = newWaypoint;
    waypointBufferStatus[nextIndex] = FULL;

    // Links waypoints together
    waypointBuffer[nextIndex]->next =  waypointBuffer[nextIndex+1];
    waypointBuffer[nextIndex]->previous =  waypointBuffer[previousIndex];
    waypointBuffer[previousIndex]->next = newWaypoint;
    waypointBuffer[nextIndex+1]->previous = newWaypoint;

    return WAYPOINT_SUCCESS;
}

_WaypointStatus WaypointManager::delete_waypoint(int waypointId) {
    int waypointIndex = get_waypoint_index_from_id(waypointId);

    if (waypointIndex == -1) {
        return INVALID_PARAMETERS;
    }

    _PathData* waypointToDelete = waypointBuffer[waypointIndex];

    // Links previous and next buffers together
    if (waypointIndex == 0) { //First element
        waypointBuffer[waypointIndex + 1]->previous = nullptr;
    } else if (waypointIndex == PATH_BUFFER_SIZE - 1 || waypointBufferStatus[waypointIndex+1] == FREE) { // Last element
        waypointBuffer[waypointIndex - 1]->next = nullptr;
    } else if (waypointBufferStatus[waypointIndex + 1] == FULL){ // Ensures that the next index is
        waypointBuffer[waypointIndex-1]->next = waypointBuffer[waypointIndex+1];
        waypointBuffer[waypointIndex+1]->previous = waypointBuffer[waypointIndex-1];
    }

    destroy_waypoint(waypointToDelete); // Frees heap memory

    // Adjusts indeces so there are no empty elements
    if(waypointIndex == numWaypoints - 1) { // Case where element is the last one in the current list
        waypointBuffer[waypointIndex] = nullptr;
        waypointBufferStatus[waypointIndex] = FREE;
    } else {
        for(int i = waypointIndex; i < numWaypoints-1; i++) {
            if (waypointBufferStatus[i+1] == FREE) {
                waypointBufferStatus[i] = FREE;
                waypointBuffer[i] = nullptr;
            } else if (waypointBufferStatus[i+1] == FULL) {
                waypointBufferStatus[i] = FULL;
                waypointBuffer[i] = waypointBuffer[i+1];
                waypointBufferStatus[i+1] = FREE;
            }
        }
    }

    // Updates array trackers
    numWaypoints--;
    nextFilledIndex--;

    return WAYPOINT_SUCCESS;
}

_WaypointStatus WaypointManager::update_waypoint(_PathData* updatedWaypoint, int waypointId) {
    int waypointIndex = get_waypoint_index_from_id(waypointId);

    if (waypointIndex == -1) {
        return INVALID_PARAMETERS;
    }

    _PathData * oldWaypoint = waypointBuffer[waypointIndex];
    waypointBuffer[waypointIndex] = updatedWaypoint; // Updates waypoint

    //Links waypoints together
    if (waypointIndex == 0) { // First element
        waypointBuffer[waypointIndex]->next = waypointBuffer[waypointIndex+1];
        waypointBuffer[waypointIndex + 1]->previous = waypointBuffer[waypointIndex];
    } else if (waypointIndex == PATH_BUFFER_SIZE - 1 || waypointBufferStatus[waypointIndex+1] == FREE) { // Last element
        waypointBuffer[waypointIndex]->previous = waypointBuffer[waypointIndex-1];
        waypointBuffer[waypointIndex - 1]->next = waypointBuffer[waypointIndex];
    } else {
        waypointBuffer[waypointIndex]->next = waypointBuffer[waypointIndex+1];
        waypointBuffer[waypointIndex]->previous = waypointBuffer[waypointIndex-1];
        waypointBuffer[waypointIndex - 1]->next = waypointBuffer[waypointIndex];
        waypointBuffer[waypointIndex + 1]->previous = waypointBuffer[waypointIndex];
    }

    destroy_waypoint(oldWaypoint); // Frees old waypoint from heap memory

    return WAYPOINT_SUCCESS;
}


/*** MISCELLANEOUS ***/


_PathData ** WaypointManager::get_waypoint_buffer() {
    return waypointBuffer;
}

_WaypointBufferStatus WaypointManager::get_status_of_index(int index) {
    return waypointBufferStatus[index];
}

_PathData * WaypointManager::get_home_base() {
    return homeBase;
}

