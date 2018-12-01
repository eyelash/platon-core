#pragma once

#include "os.hpp"
#include "tree.hpp"
#include <vector>

class Piece {
public:
	std::size_t reference_count = 0;
	Piece() = default;
	Piece(const Piece&) = delete;
	virtual ~Piece() = default;
	Piece& operator =(const Piece&) = delete;
	virtual bool insert(std::size_t index, char c) = 0;
	virtual bool remove(std::size_t index) = 0;
};

class MmapPiece: public Piece {
	Mmap mmap;
public:
	MmapPiece(const char* path): mmap(path) {}
	bool insert(std::size_t index, char c) override {
		return false;
	}
	bool remove(std::size_t index) override {
		return false;
	}
	const char* begin() const {
		return mmap.begin();
	}
	const char* end() const {
		return mmap.end();
	}
};

class MutablePiece: public Piece {
	static constexpr std::size_t SIZE = 8;
	StaticVector<char, SIZE> data;
public:
	MutablePiece() {}
	MutablePiece(char c) {
		data.insert(c);
	}
	bool insert(std::size_t index, char c) override {
		if (data.get_size() == SIZE) {
			return false;
		}
		data.insert(index, c);
		return true;
	}
	bool remove(std::size_t index) override {
		if (reference_count > 1) {
			return false;
		}
		data.remove(index);
		return true;
	}
	const char* begin() const {
		return data.begin();
	}
	const char* end() const {
		return data.end();
	}
};

class PiecePtr {
	Piece* piece;
	const char* first;
	const char* last;
public:
	PiecePtr(Piece* piece, const char* first, const char* last): piece(piece), first(first), last(last) {
		++piece->reference_count;
	}
	PiecePtr(const PiecePtr& ptr): piece(ptr.piece), first(ptr.first), last(ptr.last) {
		++piece->reference_count;
	}
	~PiecePtr() {
		if (--piece->reference_count == 0) {
			delete piece;
		}
	}
	PiecePtr& operator =(const PiecePtr& ptr) {
		if (--piece->reference_count == 0) {
			delete piece;
		}
		piece = ptr.piece;
		first = ptr.first;
		last = ptr.last;
		++piece->reference_count;
		return *this;
	}
	std::size_t get_size() const {
		return last - first;
	}
	char operator [](std::size_t index) const {
		return *(first + index);
	}
	bool insert(std::size_t index, char c) {
		if (piece->insert(index, c)) {
			++last;
			return true;
		}
		else {
			return false;
		}
	}
	bool remove(std::size_t index) {
		if (piece->remove(index)) {
			--last;
			return true;
		}
		else {
			return false;
		}
	}
	PiecePtr split_left(std::size_t index) const {
		return PiecePtr(piece, first, first + index);
	}
	PiecePtr split_right(std::size_t index) const {
		return PiecePtr(piece, first + index, last);
	}
};

class PieceTable {
	std::vector<PiecePtr> pieces;
public:
	PieceTable() {
		MutablePiece* piece = new MutablePiece();
		pieces.emplace_back(piece, piece->begin(), piece->end());
	}
	PieceTable(const char* path) {
		MmapPiece* piece = new MmapPiece(path);
		pieces.emplace_back(piece, piece->begin(), piece->end());
	}
	std::size_t get_size() const {
		std::size_t size = 0;
		for (const PiecePtr& piece: pieces) {
			size += piece.get_size();
		}
		return size;
	}
	char operator [](std::size_t index) const {
		for (const PiecePtr& piece: pieces) {
			if (index < piece.get_size()) {
				return piece[index];
			}
			index -= piece.get_size();
		}
	}
	void insert(std::size_t index, char c) {
		for (std::size_t i = 0; i < pieces.size(); ++i) {
			if (index <= pieces[i].get_size()) {
				if (!pieces[i].insert(index, c)) {
					PiecePtr left = pieces[i].split_left(index);
					MutablePiece* piece = new MutablePiece(c);
					PiecePtr right = pieces[i].split_right(index);
					pieces.erase(pieces.begin() + i);
					if (left.get_size() > 0) {
						pieces.insert(pieces.begin() + i, left);
						++i;
					}
					pieces.insert(pieces.begin() + i, PiecePtr(piece, piece->begin(), piece->end()));
					++i;
					if (right.get_size() > 0) {
						pieces.insert(pieces.begin() + i, right);
					}
				}
				return;
			}
			index -= pieces[i].get_size();
		}
	}
	void remove(std::size_t index) {
		for (std::size_t i = 0; i < pieces.size(); ++i) {
			if (index < pieces[i].get_size()) {
				if (!pieces[i].remove(index)) {
					PiecePtr left = pieces[i].split_left(index);
					PiecePtr right = pieces[i].split_right(index + 1);
					pieces.erase(pieces.begin() + i);
					if (left.get_size() > 0) {
						pieces.insert(pieces.begin() + i, left);
						++i;
					}
					if (right.get_size() > 0) {
						pieces.insert(pieces.begin() + i, right);
					}
				}
				return;
			}
			index -= pieces[i].get_size();
		}
	}
	class Iterator {
		const PieceTable& piece_table;
		std::size_t index;
	public:
		Iterator(const PieceTable& piece_table, std::size_t index): piece_table(piece_table), index(index) {}
		char operator *() const {
			return piece_table[index];
		}
		bool operator !=(const Iterator& iterator) const {
			return index != iterator.index;
		}
		Iterator& operator ++() {
			++index;
			return *this;
		}
	};
	Iterator get_iterator(std::size_t index) const {
		return Iterator(*this, index);
	}
	Iterator begin() const {
		return get_iterator(0);
	}
	Iterator end() const {
		return get_iterator(get_size());
	}
};
