#ifndef SPRING_H
#define SPRING_H

#include "BALL.h"

class SPRING
{
public:
	int ball1;
	int ball2;

	float tension;

	float springConstant;
	float naturalLength;

	SPRING() : ball1(-1), ball2(-1)
	{}
	~SPRING()
	{}
};

#endif	//SPRING_H