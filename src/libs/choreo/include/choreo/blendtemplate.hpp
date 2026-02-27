/*
 * choreo/blendtemplate.hpp
 */

#ifndef CHOREO_BLENDTEMPLATE_HPP
#define CHOREO_BLENDTEMPLATE_HPP


#include <radmath/radmath.hpp>
#include <choreo/utility.hpp>


namespace choreo
{


//----------------------------------------------------------------------
// Priority slot structure
//----------------------------------------------------------------------

template <class T>
struct BlendPriority
{
    int priority;
    float weight;
    float maxWeight;
    T value;
};


//----------------------------------------------------------------------
// Generalized blend functor
//----------------------------------------------------------------------

template <class T>
class BlendFunctor
{
public:

    void Blend(const T& in0, const T& in1, float alpha, T& out)
    {
        out = in0;
        out.Blend(alpha, in1);
    }
};


//----------------------------------------------------------------------
// Float blend functor
//----------------------------------------------------------------------

template <>
class BlendFunctor<float>
{
public:

    void Blend(float in0, float in1, float alpha, float& out)
    {
        out = BlendFloat(in0, in1, alpha);
    }
};


//----------------------------------------------------------------------
// Vector blend functor
//----------------------------------------------------------------------

template <>
class BlendFunctor<rmt::Vector>
{
public:

    void Blend(const rmt::Vector& in0,
               const rmt::Vector& in1,
               float alpha,
               rmt::Vector& out)
    {
        BlendVector(in0, in1, alpha, out);
    }
};


//----------------------------------------------------------------------
// Quaternion blend functor
//----------------------------------------------------------------------

template <>
class BlendFunctor<rmt::Quaternion>
{
public:

    void Blend(const rmt::Quaternion& in0,
               const rmt::Quaternion& in1,
               float alpha,
               rmt::Quaternion& out)
    {
        BlendQuaternion(in0, in1, alpha, out);
    }
};


//----------------------------------------------------------------------
// Cross-driver blending function
//----------------------------------------------------------------------

template <class T, class Driver>
void BlendDriverNoContext(Driver* driver,
                          void (Driver::* driverFn)(T&) const,
                          float driverWeight,
                          int driverPriority,
                          BlendPriority<T>* priorities,
                          int maxPriorities,
                          int &priorityCount)
{
    // search for this driver's slot in the current joint's priority stack
    bool performInsert = false;
    int priorityIndex;
    for (priorityIndex = 0; priorityIndex < priorityCount; ++priorityIndex)
    {
        if (driverPriority < priorities[priorityIndex].priority)
        {
            // driver has higher priority than some other in the priority stack,
            // so perform an insert into the priority stack
            performInsert = true;
            break;
        }
        else if (driverPriority == priorities[priorityIndex].priority)
        {
            // other drivers of the same priority already have been blended
            break;
        }
        else if (priorities[priorityIndex].maxWeight >= 1.0f)
        {
            // a higher priority's weight is maxed, so we wouldn't affect
            // the final blend anyway...
            return;
        }
    }
    
    if (priorityIndex >= maxPriorities)
    {
        // priority stack is full, and we're lower priority than anyone on it...
        return;
    }
    
    BlendPriority<T>* priority = (priorities + priorityIndex);

    if (performInsert || (priorityIndex == priorityCount))
    {
        // we're either inserting or appending to the priority stack
        
        // shift array contents
        for (int i = priorityCount; i > priorityIndex; --i)
        {
            priorities[i] = priorities[i - 1];
        }

        // initialize priority slot
        priority->priority = driverPriority;
        priority->weight = driverWeight;
        priority->maxWeight = driverWeight;

        (driver->*driverFn)(priority->value);
        
        ++priorityCount;
        return;
    }
    
    // find highest overall driver weight for this priority slot
    if (driverWeight > priority->maxWeight)
    {
        priority->maxWeight = driverWeight;
    }
    
    // calculate blend alpha for this driver within this priority slot
    priority->weight += driverWeight;
    float alpha = driverWeight / priority->weight;

    // retrieve value from driver
    T driverValue;
    (driver->*driverFn)(driverValue);

    // blend value
    T blendResult;
    BlendFunctor<T> blender;
    blender.Blend(priority->value, driverValue, alpha, blendResult);
    priority->value = blendResult;
}

template <class T, class Driver>
void BlendDriverNoContext(Driver* driver,
                          T (Driver::* driverFn)() const,
                          float driverWeight,
                          int driverPriority,
                          BlendPriority<T>* priorities,
                          int maxPriorities,
                          int &priorityCount)
{
    // search for this driver's slot in the current joint's priority stack
    bool performInsert = false;
    int priorityIndex;
    for (priorityIndex = 0; priorityIndex < priorityCount; ++priorityIndex)
    {
        if (driverPriority < priorities[priorityIndex].priority)
        {
            // driver has higher priority than some other in the priority stack,
            // so perform an insert into the priority stack
            performInsert = true;
            break;
        }
        else if (driverPriority == priorities[priorityIndex].priority)
        {
            // other drivers of the same priority already have been blended
            break;
        }
        else if (priorities[priorityIndex].maxWeight >= 1.0f)
        {
            // a higher priority's weight is maxed, so we wouldn't affect
            // the final blend anyway...
            return;
        }
    }
    
    if (priorityIndex >= maxPriorities)
    {
        // priority stack is full, and we're lower priority than anyone on it...
        return;
    }
    
    BlendPriority<T>* priority = (priorities + priorityIndex);

    if (performInsert || (priorityIndex == priorityCount))
    {
        // we're either inserting or appending to the priority stack
        
        // shift array contents
        for (int i = priorityCount; i > priorityIndex; --i)
        {
            priorities[i] = priorities[i - 1];
        }

        // initialize priority slot
        priority->priority = driverPriority;
        priority->weight = driverWeight;
        priority->maxWeight = driverWeight;
        priority->value = (driver->*driverFn)();
        
        ++priorityCount;
        return;
    }
    
    // find highest overall driver weight for this priority slot
    if (driverWeight > priority->maxWeight)
    {
        priority->maxWeight = driverWeight;
    }
    
    // calculate blend alpha for this driver within this priority slot
    priority->weight += driverWeight;
    float alpha = driverWeight / priority->weight;

    // blend value
    T blendResult;
    BlendFunctor<T> blender;
    blender.Blend(priority->value, (driver->*driverFn)(), alpha, blendResult);
    priority->value = blendResult;
}

template <class T, class Context, class Driver>
void BlendDriverWithContext(Context context,
                            Driver* driver,
                            void (Driver::* driverFn)(Context, T&) const,
                            float driverWeight,
                            int driverPriority,
                            BlendPriority<T>* priorities,
                            int maxPriorities,
                            int &priorityCount)
{
    // search for this driver's slot in the current joint's priority stack
    bool performInsert = false;
    int priorityIndex;
    for (priorityIndex = 0; priorityIndex < priorityCount; ++priorityIndex)
    {
        if (driverPriority < priorities[priorityIndex].priority)
        {
            // driver has higher priority than some other in the priority stack,
            // so perform an insert into the priority stack
            performInsert = true;
            break;
        }
        else if (driverPriority == priorities[priorityIndex].priority)
        {
            // other drivers of the same priority already have been blended
            break;
        }
        else if (priorities[priorityIndex].maxWeight >= 1.0f)
        {
            // a higher priority's weight is maxed, so we wouldn't affect
            // the final blend anyway...
            return;
        }
    }
    
    if (priorityIndex >= maxPriorities)
    {
        // priority stack is full, and we're lower priority than anyone on it...
        return;
    }
    
    BlendPriority<T>* priority = (priorities + priorityIndex);

    if (performInsert || (priorityIndex == priorityCount))
    {
        // we're either inserting or appending to the priority stack
        
        // shift array contents
        for (int i = priorityCount; i > priorityIndex; --i)
        {
            priorities[i] = priorities[i - 1];
        }

        // initialize priority slot
        priority->priority = driverPriority;
        priority->weight = driverWeight;
        priority->maxWeight = driverWeight;

        (driver->*driverFn)(context, priority->value);
        
        ++priorityCount;
        return;
    }
    
    // find highest overall driver weight for this priority slot
    if (driverWeight > priority->maxWeight)
    {
        priority->maxWeight = driverWeight;
    }
    
    // calculate blend alpha for this driver within this priority slot
    priority->weight += driverWeight;
    float alpha = driverWeight / priority->weight;

    // retrieve value from driver
    T driverValue;
    (driver->*driverFn)(context, driverValue);

    // blend value
    T blendResult;
    BlendFunctor<T> blender;
    blender.Blend(priority->value, driverValue, alpha, blendResult);
    priority->value = blendResult;
}


//----------------------------------------------------------------------
// Cross-priority-slot blending function
//----------------------------------------------------------------------

template <class T>
bool BlendPriorities(const BlendPriority<T>* priorities,
                     int priorityCount,
                     T& result)
{
    if (priorityCount <= 0)
        return false;
    
    // initialize blend across priority slots with the slot
    // with the highest priority
    result = priorities[0].value;
    float weightTotal = priorities[0].maxWeight;
    float weightLeft = 1.0f - weightTotal;

    BlendFunctor<T> blender;

    for (int i = 1; i < priorityCount; ++i)
    {
        // if we've used up all the available weight, no
        // more priorities get blended in
        if (weightLeft <= 0.0f)
            break;
        
        const BlendPriority<T>* priority = (priorities + i);
        
        float blendWeight;
        if (priority->maxWeight >= weightLeft)
        {
            // use up remaining weight
            blendWeight = weightLeft;
            weightLeft = 0.0f;
        }
        else
        {
            // decrement weight
            blendWeight = priority->maxWeight;
            weightLeft -= blendWeight;
        }
        
        weightTotal += blendWeight;
        float alpha = blendWeight / weightTotal;
        
        // blend into this slot's summed transform
        T blendResult;
        blender.Blend(result, priority->value, alpha, blendResult);
        result = blendResult;
    }

    return true;
}


} // namespace choreo


#endif // CHOREO_BLENDTEMPLATE_HPP
