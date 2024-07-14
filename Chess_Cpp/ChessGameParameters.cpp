#include "ChessGameParameters.hpp"

void ChessGameParameters::setDimentions(
	ChessGameParameters::BoardPosition_t w, ChessGameParameters::BoardPosition_t h)
{
	this->width=w;
	this->height=h;
	this->cellCount = h * w;
}