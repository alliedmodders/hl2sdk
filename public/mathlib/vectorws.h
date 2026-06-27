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
	using Vector::Vector;
};

#endif // VECTORWS_H