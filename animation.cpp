#include "animation.h"

Animation::Animation()
{

}

Animation::~Animation()
{
    // TODO: how to properly clear these?
}

void Animation::main()
{
    animation_t animation;

    for (vector<animation_t>::iterator animation_it = animations.begin(); animation_it != animations.end();)
    {
        animation = *animation_it;

        if (*(animation.object) < animation.endValue - animation.step)
        {
            *(animation.object) += animation.step;
            animation_it++;
        }
        else if (*(animation.object) > animation.endValue + animation.step)
        {
            *(animation.object) -= animation.step;
            animation_it++;
        }
        else
        {
            *(animation.object) = animation.endValue;

            if (animation.callback != NULL)
                (*animation.callback)();

            animations.erase(animation_it);
            // TODO: delete struct?
        }

    }
}

void Animation::addAnimation(int *object, int endValue, int step, void (*callback)())
{
    animation_t animation;
    animation.object = object;
    animation.endValue = endValue;
    animation.step = step;
    animation.callback = callback;

    animations.push_back(animation);
}

void Animation::addAnimation(int *object, int startValue, int endValue, int step, void (*callback)())
{
    *object = startValue;

    addAnimation(object, endValue, step, callback);
}

bool Animation::isAnimating(int *object)
{
    animation_t animation;

    for (vector<animation_t>::iterator animation_it = animations.begin(); animation_it != animations.end(); animation_it++)
    {
        animation = *animation_it;
        if (animation.object == object)
            return true;
    }

    return false;
}
