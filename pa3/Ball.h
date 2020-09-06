#include "WaveFrontOBJ.h" 
#ifndef BALL_H
#define BALL_H

#pragma pack(push)
#pragma pack(4)

class BALL
{
public:
	Vector position;
	Vector velocity;

	float mass;

	bool fixed;

	Vector normal;
};

#pragma pack(pop)

#endif