#ifndef ANIMATION_H
#define ANIMATION_H

#include <iostream>
#include <vector>

using namespace std;

struct animation_t
{
    int *object;
    int endValue;
    int step;
    void (*callback)();
};

class Animation
{

public:
    Animation();
    ~Animation();

    void addAnimation(int *, int startValue, int endValue, int, void (*callback)());
    void addAnimation(int *, int endValue, int, void (*callback)());
    bool isAnimating(int *);
    void main();


private:
    vector<animation_t> animations;
};

#endif
