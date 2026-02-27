#ifndef  _IKCONSTS_H
#define  _IKCONSTS_H

#define RED    tColour(255, 0, 0)
#define GREEN  tColour(0, 255, 0)
#define BLUE   tColour(0, 0, 255)

#define AX_SIZE  0.10f
#define LOCAL_X Vector(AX_SIZE, 0.0f, 0.0f)
#define LOCAL_Y Vector(0.0f, AX_SIZE, 0.0f)
#define LOCAL_Z Vector(0.0f, 0.0f, AX_SIZE)
#define X Vector(1.0f, 0.0f, 0.0f)
#define Y Vector(0.0f, 1.0f, 0.0f)
#define Z Vector(0.0f, 0.0f, 1.0f)
//#define nX Vector(-1.0f, 0.0f, 0.0f)  danger, conflicting names
//#define nY Vector(0.0f, -1.0f, 0.0f)
//#define nZ Vector(0.0f, 0.0f, -1.0f)

#endif