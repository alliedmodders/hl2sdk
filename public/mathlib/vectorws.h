#ifndef VECTORWS_H
#define VECTORWS_H

#ifdef _WIN32
#pragma once
#endif

#include "vector.h"

// AMNOTE: Mostly a stub over a real VectorWS,
// most likely meaning of it is world space vector
class VectorWS : public Vector
{
public:
	using Vector::Vector;
};

class QuaternionWS : public Quaternion
{
public:
	using Quaternion::Quaternion;
};

#endif // VECTORWS_H