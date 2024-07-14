#include "ChessBoardAnalysis.hpp"
#include "ChessBoardIterator.hpp"
#include "ChessPlayerColour.hpp"
#include <cassert>
#include <algorithm>

#include <string>

#include "Log.hpp"

typedef ChessBoardAnalysis::weight_type weight_type;

// static variables

ChessBoardFactory ChessBoardAnalysis::factory;

// helper

inline constexpr weight_type domination(const int8_t &white, const int8_t &black)
{
	return
		white==black ? 0 :
		white>black ? 1 :
		-1;
}
constexpr weight_type weightFromPiece(const ChessPiece &cp)
{
	return 
		cp==PAWN_WHITE   || cp==PAWN_BLACK   ? BOARD_PAWN_WEIGHT :
		cp==KNIGHT_WHITE || cp==KNIGHT_BLACK ? BOARD_KNIGHT_WEIGHT :
		cp==BISHOP_WHITE || cp==BISHOP_BLACK ? BOARD_BISHOP_WEIGHT :
		cp==ROOK_WHITE   || cp==ROOK_BLACK   ? BOARD_ROOK_WEIGHT :
		cp==QUEEN_WHITE  || cp==QUEEN_BLACK  ? BOARD_QUEEN_WEIGHT :
		cp==KING_WHITE   || cp==KING_BLACK   ? BOARD_KING_WEIGHT :
		0;
}

// static variables

unsigned long long ChessBoardAnalysis::constructed=0;


// class functions

ChessBoardAnalysis::ChessBoardAnalysis(ChessBoard::ptr board_)
	: board(std::move(board_)), possibleMoves(nullptr), check(false),
		underAttackByWhite(nullptr), underAttackByBlack(nullptr)
{
	assert(board!=nullptr);
	++constructed;
}

ChessBoardAnalysis::~ChessBoardAnalysis()
{
	this->reset();
}

void ChessBoardAnalysis::reset()
{
	if(possibleMoves) delete possibleMoves;
	if(underAttackByWhite) delete[] underAttackByWhite;
	if(underAttackByBlack) delete[] underAttackByBlack;
	
	possibleMoves = nullptr;
	underAttackByWhite = underAttackByBlack = nullptr;
}

void ChessBoardAnalysis::calculatePossibleMoves_common()
{
	typedef ChessMove::ChessMoveRecordingFunction ChessMoveRecordingFunction;
		// empty function
	const static ChessMoveRecordingFunction emptyFunction =
			[](ChessBoard::BoardPosition_t pos, ChessBoard::BoardPosition_t newPos) {};

	
		// take opponent's piece
		// or
		// attack empty space
	ChessMoveRecordingFunction functionTake[2][2] = 
	{
		// white turn
		{
			//white
			[this](ChessBoard::BoardPosition_t pos, ChessBoard::BoardPosition_t newPos) {
				assert(this->board->getTurn()==ChessPlayerColour::WHITE);
				
				auto nextBoard = ChessBoardAnalysis::factory.createBoard(this->board, pos, newPos);
				
				assert(nextBoard->getTurn()==ChessPlayerColour::BLACK);
				
				if(ChessMove::isMovePossible(nextBoard))
				{
					this->possibleMoves->push_back(nextBoard);
				}
				else
				{
					nextBoard.reset();
				}
			},
			//black
			[this](ChessBoard::BoardPosition_t pos, ChessBoard::BoardPosition_t newPos) {
				//assert(maybeMove->getTurn()==ChessPlayerColour::BLACK);
				
				if(this->board->getPiecePos(newPos)==KING_WHITE)
				{
					check=true;
				}
			}
		},
		// black turn
		{
			// white
			[this](ChessBoard::BoardPosition_t pos, ChessBoard::BoardPosition_t newPos) {
				assert(this->board->getTurn()==ChessPlayerColour::BLACK);
				if(this->board->getPiecePos(newPos)==KING_BLACK)
				{
					check=true;
				}
			},
			// black
			[this](ChessBoard::BoardPosition_t pos, ChessBoard::BoardPosition_t newPos) {
				assert(this->board->getTurn()==ChessPlayerColour::BLACK);

				auto nextBoard = ChessBoardAnalysis::factory.createBoard(this->board, pos, newPos);
				
				assert(nextBoard->getTurn()==ChessPlayerColour::WHITE);
				
				if(ChessMove::isMovePossible(nextBoard))
				{
					this->possibleMoves->push_back(nextBoard);
				}
				else
				{
					nextBoard.reset();
				}
			}
		}
	};
		// move to empty space
		// with no possibility of attack
		// (pawn move forward)
	ChessMoveRecordingFunction functionNoTake[2][2] = 
	{
		// white turn
		{
			// white
			functionTake[toArrayPosition(ChessPlayerColour::WHITE)][toArrayPosition(ChessPlayerColour::WHITE)],
			// black
			emptyFunction
		},
		// black turn
		{
			// white
			emptyFunction,
			// black
			functionTake[toArrayPosition(ChessPlayerColour::BLACK)][toArrayPosition(ChessPlayerColour::BLACK)]
		}
	};
	
		// we could recapture on this square	
	ChessMoveRecordingFunction functionDefend[2][2] = 
	{
		// white's turn
		{
			// white
			[this](ChessBoard::BoardPosition_t pos, ChessBoard::BoardPosition_t newPos) {
				//assert(maybeMove->getTurn()==ChessPlayerColour::WHITE);
				
				++underAttackByWhite[newPos];
			},
			// black
			[this](ChessBoard::BoardPosition_t pos, ChessBoard::BoardPosition_t newPos) {
				//assert(maybeMove->getTurn()==ChessPlayerColour::BLACK);

				++underAttackByWhite[newPos];
			}
		},
		// black's turn
		{
			// white
			[this](ChessBoard::BoardPosition_t pos, ChessBoard::BoardPosition_t newPos) {
				//assert(maybeMove->getTurn()==ChessPlayerColour::WHITE);
				
				++underAttackByBlack[newPos];
			},
			// black
			[this](ChessBoard::BoardPosition_t pos, ChessBoard::BoardPosition_t newPos) {
				//assert(maybeMove->getTurn()==ChessPlayerColour::BLACK);

				++underAttackByBlack[newPos];
			}
		}
	};

	// main common moves
	for(ChessBoard::BoardPosition_t pos=0, end=ChessBoard::param.cellCount; pos!=end; ++pos)
	{
		auto curPiece = board->getPiecePos(pos);
		if(curPiece == EMPTY_CELL) continue;
		
		auto pieceParam = moveParameters.at(curPiece);
		auto moveArrayPos = toArrayPosition(board->getTurn());
		auto pieceArrayPos = toArrayPosition(getColour(curPiece));
		
		if(pieceParam->isDifferentMoveTypes)
		{
			ChessMove::moveAttempts(functionNoTake[moveArrayPos][pieceArrayPos], emptyFunction,
				*board, pos,
				*pieceParam->noTakeMove, false);
			ChessMove::moveAttempts(functionTake[moveArrayPos][pieceArrayPos],
				functionDefend[moveArrayPos][pieceArrayPos],
				*board, pos,
				*pieceParam->takeMove, true, false);
		}
		else
		{
			ChessMove::moveAttempts(functionTake[moveArrayPos][pieceArrayPos],
				functionDefend[moveArrayPos][pieceArrayPos],
				*board, pos, *pieceParam->anyMove, true);
		}
	}
}
void ChessBoardAnalysis::calculatePossibleMoves_pawnfirst()
{
	if(board->getTurn()==ChessPlayerColour::WHITE)
	{
		// process white pawns on first ranks
		for(ChessBoard::BoardPosition_t pos = 0, end=ChessBoard::param.width*2; pos<end; ++pos)
		{
			auto curPiece = board->getPiecePos(pos);
			if(curPiece != PAWN_WHITE) continue;
			
			auto enPassan=pos+ChessBoard::param.width;
			if(board->getPiecePos(enPassan) == EMPTY_CELL)
			{
				auto newPos = pos+2*ChessBoard::param.width;
				if(board->getPiecePos(newPos)==EMPTY_CELL)
				{
					auto nextBoard = factory.createBoard(this->board, pos, newPos);
					
					if(ChessMove::isMovePossible(nextBoard))
					{
						nextBoard->enPassan=enPassan;
						this->possibleMoves->push_back(nextBoard);
					}
					else
					{
						nextBoard.reset();
					}
				}
			}
		}
	}
	else // if ChessPlayerColour::Black
	{
		// process black pawns on first ranks
		for(ChessBoard::BoardPosition_t pos = ChessBoard::param.cellCount-1, end=ChessBoard::param.cellCount - (ChessBoard::param.width*2) -1; pos>=end; --pos)
		{
			
			auto curPiece = board->getPiecePos(pos);
			if(curPiece != PAWN_BLACK) continue;
			
			auto enPassan=pos-ChessBoard::param.width;
			if(board->getPiecePos(enPassan) == EMPTY_CELL)
			{
				auto newPos = pos-2*ChessBoard::param.width;
				if(board->getPiecePos(newPos)==EMPTY_CELL)
				{
					auto nextBoard = factory.createBoard(this->board, pos, newPos);
					
					if(ChessMove::isMovePossible(nextBoard))
					{
						nextBoard->enPassan=enPassan;
						this->possibleMoves->push_back(nextBoard);
					}
					else
					{
						nextBoard.reset();
					}
				}
			}
		}
	}
}

void ChessBoardAnalysis::calculatePossibleMoves_enpassan()
{
	if(board->getTurn()==ChessPlayerColour::WHITE)
	{
		// process en passan rules
		if(board->enPassan!=ChessBoard::param.cellCount)
		{
			// test left
			if(board->enPassan % ChessBoard::param.width != 0)
			{
				auto pos = board->enPassan - ChessBoard::param.width - 1;
				if(board->getPiecePos(pos)==PAWN_WHITE)
				{
					auto nextBoard = factory.createBoard(this->board, pos, board->enPassan);
					
					if(ChessMove::isMovePossible(nextBoard))
					{
						++underAttackByWhite[pos+1];
						nextBoard->placePiecePos(pos+1, EMPTY_CELL);
						this->possibleMoves->push_back(nextBoard);
					}
					else
					{
						nextBoard.reset();
					}
				}
			}
			// test right
			if(board->enPassan % ChessBoard::param.width != ChessBoard::param.width-1)
			{
				auto pos = board->enPassan - ChessBoard::param.width + 1;
				if(board->getPiecePos(pos)==PAWN_WHITE)
				{
					auto nextBoard = factory.createBoard(this->board, pos, board->enPassan);
					
					if(ChessMove::isMovePossible(nextBoard))
					{
						++underAttackByWhite[pos-1];
						nextBoard->placePiecePos(pos-1, EMPTY_CELL);
						this->possibleMoves->push_back(nextBoard);
					}
					else
					{
						nextBoard.reset();
					}
				}
			}
		}
	}
	else // if ChessPlayerColour::Black
	{
		if(board->enPassan!=ChessBoard::param.cellCount)
		{
			// test left
			if(board->enPassan % ChessBoard::param.width != 0)
			{
				auto pos = board->enPassan + ChessBoard::param.width - 1;
				if(board->getPiecePos(pos)==PAWN_BLACK)
				{
					auto nextBoard = factory.createBoard(this->board, pos, board->enPassan);
					
					if(ChessMove::isMovePossible(nextBoard))
					{
						++underAttackByBlack[pos+1];
						nextBoard->placePiecePos(pos+1, EMPTY_CELL);
						this->possibleMoves->push_back(nextBoard);
					}
					else
					{
						nextBoard.reset();
					}
				}
			}
			// test right
			if(board->enPassan % ChessBoard::param.width != ChessBoard::param.width-1)
			{
				auto pos = board->enPassan + ChessBoard::param.width + 1;

				if(board->getPiecePos(pos)==PAWN_BLACK)
				{
					auto nextBoard = factory.createBoard(this->board, pos, board->enPassan);
					
					if(ChessMove::isMovePossible(nextBoard))
					{
						++underAttackByBlack[pos-1];
						nextBoard->placePiecePos(pos-1, EMPTY_CELL);
						this->possibleMoves->push_back(nextBoard);
					}
					else
					{
						nextBoard.reset();
					}
				}
			}
		}
	}
}

void ChessBoardAnalysis::calculatePossibleMoves_castling()
{
	if(board->getTurn()==ChessPlayerColour::WHITE)
	{
		// process castling rules
		if(board->whiteCastling[0]!=ChessBoard::param.cellCount) // if can castle left
		{
			assert(board->getPiecePos(board->whiteCastling[0])==ROOK_WHITE);
			if(
				underAttackByBlack[board->whiteKingPos[0]] == 0 &&
				underAttackByBlack[board->whiteKingPos[0]-1] == 0)
			{
				if(board->whiteKingPos[1] >=2) // farther than 2 files from the edge
				{
					bool allEmpty = true;
					for(ChessBoard::BoardPosition_t cell = board->whiteCastling[0]+1; cell<board->whiteKingPos[0]; ++cell)
					{
						if(board->getPiecePos(cell)!=EMPTY_CELL)
						{
							allEmpty=false;
							break;
						}
					}
					if(allEmpty && underAttackByBlack[board->whiteKingPos[0]-2] == 0)
					{
						auto nextBoard = factory.createBoard(this->board, 
							board->whiteKingPos[0], board->whiteKingPos[0]-2, // move king
							board->whiteCastling[0], board->whiteKingPos[0]-1 // move rook
							);
						if(ChessMove::isMovePossible(nextBoard))
						{
							this->possibleMoves->push_back(nextBoard);
						}
						else
						{
							nextBoard.reset();
						}
					}
				}
				else
				{
					auto nextBoard = factory.createBoard(this->board, 
						board->whiteKingPos[0], board->whiteKingPos[0]-1, // move king
						board->whiteCastling[0], board->whiteKingPos[0] // move rook
						);
					// not checking if move is possible, because we know that the cell on the left isn't
					// being attacked. note, that it cannot be assumed in the normal castling
					this->possibleMoves->push_back(nextBoard);
				}
			}
		}
		if(board->whiteCastling[1]!=ChessBoard::param.cellCount) // if can castle right
		{
			assert(board->getPiecePos(board->whiteCastling[1])==ROOK_WHITE);
			if(
				underAttackByBlack[board->whiteKingPos[0]] == 0 &&
				underAttackByBlack[board->whiteKingPos[0]+1] == 0)
			{
				if(board->whiteKingPos[1] < ChessBoard::param.width-2) // farther than 2 files from the edge
				{
					bool allEmpty = true;
					for(ChessBoard::BoardPosition_t cell = board->whiteCastling[0]+1; cell<board->whiteKingPos[0]; ++cell)
					{
						if(board->getPiecePos(cell)!=EMPTY_CELL)
						{
							allEmpty=false;
							break;
						}
					}
					if(allEmpty &&underAttackByBlack[board->whiteKingPos[0]+2] == 0)
					{
						auto nextBoard = factory.createBoard(this->board, 
							board->whiteKingPos[0], board->whiteKingPos[0]+2, // move king
							board->whiteCastling[1], board->whiteKingPos[0]+1 // move rook
							);
						if(ChessMove::isMovePossible(nextBoard))
						{
							this->possibleMoves->push_back(nextBoard);
						}
						else
						{
							nextBoard.reset();
						}
					}
				}
				else
				{
					auto nextBoard = factory.createBoard(this->board, 
						board->whiteKingPos[0], board->whiteKingPos[0]+1, // move king
						board->whiteCastling[1], board->whiteKingPos[0] // move rook
						);
					// not checking if move is possible, because we know that the cell on the left isn't
					// being attacked. note, that it cannot be assumed in the normal castling
					this->possibleMoves->push_back(nextBoard);
				}
			}
		}
	}
	else // if ChessPlayerColour::Black
	{
		// process castling rules
		if(board->blackCastling[0]!=ChessBoard::param.cellCount) // if can castle left
		{
			assert(board->getPiecePos(board->blackCastling[0])==ROOK_BLACK);
			if(
				underAttackByWhite[board->blackKingPos[0]] == 0 &&
				underAttackByWhite[board->blackKingPos[0]-1] == 0)
			{
				if(board->blackKingPos[1] >=2) // farther than 2 files from the edge
				{
					bool allEmpty = true;
					for(ChessBoard::BoardPosition_t cell = board->blackCastling[0]+1; cell<board->blackKingPos[0]; ++cell)
					{
						if(board->getPiecePos(cell)!=EMPTY_CELL)
						{
							allEmpty=false;
							break;
						}
					}
					if(allEmpty && underAttackByWhite[board->blackKingPos[0]-2] == 0)
					{
						auto nextBoard = factory.createBoard(this->board, 
							board->blackKingPos[0], board->blackKingPos[0]-2, // move king
							board->blackCastling[0], board->blackKingPos[0]-1 // move rook
							);
						if(ChessMove::isMovePossible(nextBoard))
						{
							this->possibleMoves->push_back(nextBoard);
						}
						else
						{
							nextBoard.reset();
						}
					}
				}
				else
				{
					auto nextBoard = factory.createBoard(this->board, 
						board->blackKingPos[0], board->blackKingPos[0]-1, // move king
						board->blackCastling[0], board->blackKingPos[0] // move rook
						);
					// not checking if move is possible, because we know that the cell on the left isn't
					// being attacked. note, that it cannot be assumed in the normal castling
					this->possibleMoves->push_back(nextBoard);
				}
			}
		}
		if(board->blackCastling[1]!=ChessBoard::param.cellCount) // if can castle right
		{
			assert(board->getPiecePos(board->blackCastling[1])==ROOK_BLACK);
			if(
				underAttackByWhite[board->blackKingPos[0]] == 0 &&
				underAttackByWhite[board->blackKingPos[0]+1] == 0)
			{
				if(board->blackKingPos[1] < ChessBoard::param.width-2) // farther than 2 files from the edge
				{
					bool allEmpty = true;
					for(ChessBoard::BoardPosition_t cell = board->blackCastling[0]+1; cell<board->blackKingPos[0]; ++cell)
					{
						if(board->getPiecePos(cell)!=EMPTY_CELL)
						{
							allEmpty=false;
							break;
						}
					}
					if(allEmpty && underAttackByWhite[board->blackKingPos[0]+2] == 0)
					{
						auto nextBoard = factory.createBoard(this->board, 
							board->blackKingPos[0], board->blackKingPos[0]+2, // move king
							board->blackCastling[1], board->blackKingPos[0]+1 // move rook
							);
						if(ChessMove::isMovePossible(nextBoard))
						{
							this->possibleMoves->push_back(nextBoard);
						}
						else
						{
							nextBoard.reset();
						}
					}
				}
				else
				{
					auto nextBoard = factory.createBoard(this->board, 
						board->blackKingPos[0], board->blackKingPos[0]+1, // move king
						board->blackCastling[1], board->blackKingPos[0] // move rook
						);
					// not checking if move is possible, because we know that the cell on the left isn't
					// being attacked. note, that it cannot be assumed in the normal castling
					this->possibleMoves->push_back(nextBoard);
				}
			}
		}
	}
}

void ChessBoardAnalysis::calculatePossibleMoves()
{
	if(possibleMoves != nullptr)
	{
		return;
	}

	underAttackByBlack = new int8_t[ChessBoard::param.cellCount]{};
	underAttackByWhite = new int8_t[ChessBoard::param.cellCount]{};

	possibleMoves=new std::vector<ChessBoard::ptr>();

	calculatePossibleMoves_common();
	calculatePossibleMoves_pawnfirst();
	calculatePossibleMoves_enpassan();
	calculatePossibleMoves_castling();
	
	std::sort(possibleMoves->begin(), possibleMoves->end(),
			[](ChessBoard::ptr &l, ChessBoard::ptr &r) -> bool {
				return ChessBoardAnalysis(l).chessPiecesWeight() < ChessBoardAnalysis(r).chessPiecesWeight();
			}
		);
	
	possibleMoves->shrink_to_fit();
}

weight_type ChessBoardAnalysis::chessPositionWeight(bool log) const
{
	if(isCheckMate())
	{
		weight_type wIsCheckMate = getWeightMultiplier(board->getTurn()) * CHECKMATE_WEIGHT;
		weight_type wMoveNum = getWeightMultiplier(board->getTurn())*board->getMoveNum();
		
		return wIsCheckMate + wMoveNum;
	}
	else
	{
		auto count = this->chessPiecesCount();
		weight_type wChessPieces = this->chessPiecesWeight(count);	// count pieces weights
		weight_type wChessPieceAttacked = this->chessPieceAttackedWeight(); // count attacked pieces
		weight_type wChessCentreControl = this->chessCentreControlWeight(); // control of the centre of the board

		if(log)
		{
			Log::info(std::string("wChessPieces: ")+std::to_string(wChessPieces));
			Log::info(std::string("wChessPieceAttacked: ")+std::to_string(wChessPieceAttacked));
			Log::info(std::string("wChessPieceAttacked: ")+std::to_string(wChessPieceAttacked));
			Log::info(std::string("wChessCentreControl: ")+std::to_string(wChessCentreControl));
		}
		return wChessPieces + wChessPieceAttacked + wChessCentreControl;
	}
}

std::array<int16_t, KNOWN_CHESS_PIECE_COUNT> ChessBoardAnalysis::chessPiecesCount() const
{
	std::array<int16_t, KNOWN_CHESS_PIECE_COUNT> count{0};
	
	for(ChessBoard::BoardPosition_t pos=0, end=ChessBoard::param.cellCount; pos!=end; ++pos)
	{
		++count[board->getPiecePos(pos)]; // ChessPiece is a numerical constant
	}
	
	return count;
}

ChessGamePart ChessBoardAnalysis::chessGamePart(const std::array<int16_t, KNOWN_CHESS_PIECE_COUNT> &count) const
{
	int countFigures = 0;
	for(size_t i=0; i<KNOWN_CHESS_PIECE_COUNT; ++i)
	{
		if(i==EMPTY_CELL ||
			i==PAWN_WHITE || i==PAWN_BLACK ||
			i==KING_WHITE || i==KING_BLACK
		)
		{
			continue;
		}
		countFigures += count[i];
	}
	
	if(countFigures <= 4)
	{
		return ChessGamePart::END_GAME;
	}
	else if(countFigures <= 10)
	{
		return ChessGamePart::MID_GAME;
	}
	else
	{
		return ChessGamePart::OPENING;
	}
}

weight_type ChessBoardAnalysis::chessPiecesWeight() const
{
	return this->chessPiecesWeight(this->chessPiecesCount());
}

weight_type ChessBoardAnalysis::chessPiecesWeight(const std::array<int16_t, KNOWN_CHESS_PIECE_COUNT> &count) const
{
	return
		PIECE_PRESENT_MILTIPLIER * (
		BOARD_PAWN_WEIGHT   * (count[PAWN_WHITE]   - count[PAWN_BLACK]) +
		BOARD_KNIGHT_WEIGHT * (count[KNIGHT_WHITE] - count[KNIGHT_BLACK]) +
		BOARD_BISHOP_WEIGHT * (count[BISHOP_WHITE] - count[BISHOP_BLACK]) +
		BOARD_ROOK_WEIGHT   * (count[ROOK_WHITE]   - count[ROOK_BLACK]) +
		BOARD_QUEEN_WEIGHT  * (count[QUEEN_WHITE]  - count[QUEEN_BLACK])
		);

}
weight_type ChessBoardAnalysis::chessPieceAttackedWeight() const
{
	assert(board);
	weight_type result = 0;
	
	ChessPiece curPiece;
	for(ChessBoard::BoardPosition_t pos=0, end=ChessBoard::param.cellCount; pos!=end; ++pos)
	{
		// get non-empty cells
		curPiece = board->getPiecePos(pos);
		if(curPiece == EMPTY_CELL) continue;
		
		auto multiplierColour = getWeightMultiplier(getColour(curPiece));
		auto dominator = domination( // who has more attacks -1 (black); 0 (neutral); 1 (white)
			underAttackByWhite[pos],
			underAttackByBlack[pos]
			);
		auto attackOrDefence = PIECE_ATTACK_MULTIPLIER;
		if(multiplierColour==dominator)
		{
			attackOrDefence = PIECE_DEFENCE_MUTIPLIER; // defending own piece
		}
		
		result += dominator * weightFromPiece(curPiece) * attackOrDefence;
	}
	
	return result;
}

weight_type ChessBoardAnalysis::chessCentreControlWeight() const
{
	const static weight_type CELL_WEIGHT_MULTIPLIER = 300;
	const static weight_type CELL_WEIGHT[64]
	{
		3, 3, 3, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 3, 3, 3, 3,
		2, 2, 7, 7, 7, 7, 2, 2,
		1, 4, 6, 8, 8, 6, 4, 1,
		1, 4, 6, 8, 8, 6, 4, 1,
		2, 2, 7, 7, 7, 7, 2, 2,
		3, 3, 3, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 3, 3, 3, 3
	};
	
	weight_type result = 0;
	for(ChessBoard::BoardPosition_t pos=0, end=ChessBoard::param.cellCount; pos!=end; ++pos)
	{
		result +=
			domination(
				underAttackByWhite[pos],
				underAttackByBlack[pos]
				)
			* CELL_WEIGHT[pos]
			* CELL_WEIGHT_MULTIPLIER;
	}
	return result;
}

weight_type ChessBoardAnalysis::chessKingPositionWeight(ChessGamePart gamePart) const
{
	if(gamePart==ChessGamePart::END_GAME)
	{
		int wh = ChessBoard::param.width - board->whiteKingPos[1];
		int wv = ChessBoard::param.height - board->whiteKingPos[2];
		int bh = ChessBoard::param.width - board->blackKingPos[1];
		int bv = ChessBoard::param.height - board->blackKingPos[2];
		wh = wh<0 ? -wh : wh;
		wv = wv<0 ? -wv : wv;
		bh = bh<0 ? -bh : bh;
		bv = bv<0 ? -bv : bv;
		
		int wd = wh + wv;
		int bd = bh + bv;
		
		return (wd - bd);
	}
	else//(gamePart==ChessGamePart::MID_GAME || gamePart==ChessGamePart::OPENING)
	{
		int res = 0;
		const std::pair<int, int> neighbours[8] = {
			std::pair<int, int>(-1, -1),
			std::pair<int, int>(-1, 0),
			std::pair<int, int>(-1, 1),
			std::pair<int, int>(0, -1),
			std::pair<int, int>(0, 1),
			std::pair<int, int>(1, -1),
			std::pair<int, int>(1, 0),
			std::pair<int, int>(1, 1)
		};
		
		size_t x, y;
		for(auto p : neighbours)
		{
			x = board->whiteKingPos[1] + p.first;
			y = board->whiteKingPos[2] + p.second;
			
			if(x >= ChessBoard::param.width || y >= ChessBoard::param.height)
			{
				++res;
				continue;
			}
			
			res+=domination(underAttackByWhite[y*ChessBoard::param.width+x], underAttackByBlack[y*ChessBoard::param.width+x]);
		}
		for(auto p : neighbours)
		{
			x = board->blackKingPos[1] + p.first;
			y = board->blackKingPos[2] + p.second;
			
			if(x >= ChessBoard::param.width || y >= ChessBoard::param.height)
			{
				--res;
				continue;
			}
			
			res+=domination(underAttackByWhite[y*ChessBoard::param.width+x], underAttackByBlack[y*ChessBoard::param.width+x]);
		}
		
		return res;
	}
}

bool ChessBoardAnalysis::isCheck() const
{
	return check;
}

bool ChessBoardAnalysis::isCheckMate() const
{
	if(isCheck())
	{
		return possibleMoves->empty();
	}
	
	return false;
}

void ChessBoardAnalysis::clearPossibleMoves(ChessBoard::ptr toKeep)
{
	if(!possibleMoves) return;
	for(auto it=possibleMoves->begin(), end=possibleMoves->end(); it!=end; ++it)
	{
		if(*it != toKeep)
		{
			(*it)->from.reset();
			(*it)->clearPossibleMoves();
		}
	}
	this->reset();
	if(toKeep)
	{
		possibleMoves = new std::vector<ChessBoard::ptr>(1);
		(*possibleMoves)[0]=std::move(toKeep);
		possibleMoves->shrink_to_fit();
	}
}

std::vector<ChessBoard::ptr> * const ChessBoardAnalysis::getPossibleMoves() const
{
	return possibleMoves;
}

ChessBoard::ptr ChessBoardAnalysis::getBoard() const
{
	return board;
}
