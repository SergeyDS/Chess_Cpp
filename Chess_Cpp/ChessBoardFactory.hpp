#ifndef CHESSBOARDFACTORY__
#define CHESSBOARDFACTORY__

#include "config.hpp"

#include "ChessBoard.hpp"
#include "ChessMove.hpp"
#include <string>
#include <vector>

class ChessBoardFactory
{
	ChessBoard::ptr createBoard(const ChessBoard::ptr &fromBoard);
public:
	//static std::vector<std::weak_ptr<ChessBoard>> allBoards;
	ChessBoard::ptr createBoard();
	ChessBoard::ptr createBoard(std::string fen);
	ChessBoard::ptr createBoard(
		const ChessBoard::ptr &fromBoard,
		const size_t &posFrom, const size_t &posTo);
	ChessBoard::ptr createBoard(
		const ChessBoard::ptr &fromBoard,
		const size_t &posFrom1, const size_t &posTo1,
		const size_t &posFrom2, const size_t &posTo2);
};

#endif