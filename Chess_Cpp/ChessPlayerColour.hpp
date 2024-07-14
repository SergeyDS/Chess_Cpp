#ifndef CHESSPLAYERCOLOUR__
#define CHESSPLAYERCOLOUR__

#include "config.hpp"

#include <cstddef>

enum class ChessPlayerColour
{
	WHITE=0, BLACK=1
};

constexpr ChessPlayerColour operator!(ChessPlayerColour c)
{
	return (c==ChessPlayerColour::WHITE) ? ChessPlayerColour::BLACK : ChessPlayerColour::WHITE;
}

constexpr long long getWeightMultiplier(ChessPlayerColour c)
{
	return (c==ChessPlayerColour::WHITE) ? 1 : -1;
}

constexpr size_t toArrayPosition(ChessPlayerColour c)
{
	//return (c==ChessPlayerColour::WHITE) ? (size_t)0 : (size_t)1;
	return (size_t)c;
}

#endif