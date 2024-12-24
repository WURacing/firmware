#include "RecursiveUpdate.h"

// Constructor
RecursiveUpdate::RecursiveUpdate() {}

// Method to update recursive average
float RecursiveUpdate::updateRecursiveAverage(float newDataPoint, int k, float currentAverage) {
    float alpha = 1.0 / k;

    if (currentAverage == -1.0) { // -1.0 represents None in Arduino
        return newDataPoint;
    } else {
        return (1.0 - alpha) * currentAverage + (newDataPoint * alpha);
    }
}

// Method to update recursive filter
float RecursiveUpdate::updateRecursiveFilter(float newDataPoint, float currentAverage, float alpha) {
    if (currentAverage == -1.0) { // -1.0 represents None in Arduino
        return newDataPoint;
    } else {
        return (1.0 - alpha) * currentAverage + (newDataPoint * alpha);
    }
}
