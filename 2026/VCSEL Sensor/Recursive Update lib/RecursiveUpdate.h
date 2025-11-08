#ifndef RECURSIVE_UPDATE_H
#define RECURSIVE_UPDATE_H

class RecursiveUpdate {
public:
    // Constructor
    RecursiveUpdate();

    // Methods for recursive updates
    float updateRecursiveAverage(float newDataPoint, int k, float currentAverage);
    float updateRecursiveFilter(float newDataPoint, float currentAverage, float alpha);
};

#endif // RECURSIVE_UPDATE_H