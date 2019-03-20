#include "Position.h"

// operator overloads that aren't members
Position operator + (const Position & a, const Position & b){
	return Position(a.X() + b.X(), a.Y() + b.Y(), a.Z() + b.Z());
}

Position operator - (const Position & a, const Position & b){
	return Position(a.X() - b.X(), a.Y() - b.Y(), a.Z() - b.Z());
}

Position operator * (const Position & p, double a){
	return Position(a*p.X(), a*p.Y(), a*p.Z());
}

Position operator * (double a, const Position & p){
	return Position(a*p.X(), a*p.Y(), a*p.Z());
}

double operator * (const Position & a, const Position & b){
	return a.Dot(b);
}
