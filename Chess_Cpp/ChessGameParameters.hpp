#ifndef CHESSGAMEPARAMETERS__
#define CHESSGAMEPARAMETERS__

#include <vector>
#include <cstdint>
#include <memory>

#include "ChessPiece.hpp"

struct ChessGameParameters
{
public:
	typedef uint16_t BoardPosition_t;
	
	BoardPosition_t height;
	BoardPosition_t width;
	BoardPosition_t cellCount;
	std::vector<ChessPiece> possiblePieces;

	void setDimentions(BoardPosition_t w, BoardPosition_t h);
};

#endif