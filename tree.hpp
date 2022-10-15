#pragma once

#include <cstddef>
#include <utility>
#include <type_traits>
#include <new>
#include <iterator>
#include <cassert>

template <class T, std::size_t N> class StaticVector {
	typename std::aligned_storage<sizeof(T), alignof(T)>::type data[N];
	std::size_t size;
public:
	T* get_data() noexcept {
		return reinterpret_cast<T*>(data);
	}
	const T* get_data() const noexcept {
		return reinterpret_cast<const T*>(data);
	}
	std::size_t get_size() const noexcept {
		return size;
	}
	T& get(std::size_t index) noexcept {
		return *(get_data() + index);
	}
	const T& get(std::size_t index) const noexcept {
		return *(get_data() + index);
	}
	T& get() noexcept {
		return get(size - 1);
	}
	const T& get() const noexcept {
		return get(size - 1);
	}
	StaticVector(): size(0) {}
	StaticVector(const StaticVector& v): size(v.size) {
		for (std::size_t i = 0; i < size; ++i) {
			new (data + i) T(v.get(i));
		}
	}
	~StaticVector() {
		for (std::size_t i = 0; i < size; ++i) {
			get(i).~T();
		}
	}
	StaticVector& operator =(const StaticVector& v) {
		for (std::size_t i = 0; i < size; ++i) {
			get(i).~T();
		}
		size = v.size;
		for (std::size_t i = 0; i < size; ++i) {
			new (data + i) T(v.get(i));
		}
		return *this;
	}
	void insert(std::size_t index, const T& element) {
		assert(index <= size && size < N);
		if (index == size) {
			new (data + size) T(element);
		}
		else {
			new (data + size) T(std::move(get(size - 1)));
			for (std::size_t i = size - 1; i > index; --i) {
				get(i) = std::move(get(i - 1));
			}
			get(index) = element;
		}
		++size;
	}
	void insert(const T& element) {
		assert(size < N);
		new (data + size) T(element);
		++size;
	}
	void remove(std::size_t index) {
		assert(index < size);
		--size;
		for (std::size_t i = index; i < size; ++i) {
			get(i) = std::move(get(i + 1));
		}
		get(size).~T();
	}
	void remove() {
		assert(size > 0);
		--size;
		get(size).~T();
	}
	void balance_out(StaticVector& v, std::size_t n) {
		// move n elements from the end of this to the beginning of v
		assert(n <= size && v.size + n <= N);
		if (n < v.size) {
			for (std::size_t i = 0; i < n; ++i) {
				new (v.data + v.size + n - 1 - i) T(std::move(v.get(v.size - 1 - i)));
			}
			for (std::size_t i = n; i < v.size; ++i) {
				v.get(v.size + n - 1 - i) = std::move(v.get(v.size - 1 - i));
			}
			for (std::size_t i = 0; i < n; ++i) {
				v.get(i) = std::move(get(size - n + i));
				get(size - n + i).~T();
			}
		}
		else {
			for (std::size_t i = 0; i < v.size; ++i) {
				new (v.data + n + i) T(std::move(v.get(i)));
				v.get(i) = std::move(get(size - n + i));
				get(size - n + i).~T();
			}
			for (std::size_t i = v.size; i < n; ++i) {
				new (v.data + i) T(std::move(get(size - n + i)));
				get(size - n + i).~T();
			}
		}
		size -= n;
		v.size += n;
	}
	void balance_in(StaticVector& v, std::size_t n) {
		// move n elements from the beginning of v to the end of this
		assert(n <= v.size && size + n <= N);
		for (std::size_t i = 0; i < n; ++i) {
			new (data + size + i) T(std::move(v.get(i)));
		}
		for (std::size_t i = n; i < v.size; ++i) {
			v.get(i - n) = std::move(v.get(i));
		}
		for (std::size_t i = 0; i < n; ++i) {
			v.get(v.size - n + i).~T();
		}
		size += n;
		v.size -= n;
	}
	T& operator [](std::size_t index) {
		return get(index);
	}
	const T& operator [](std::size_t index) const {
		return get(index);
	}
	T* begin() {
		return get_data();
	}
	const T* begin() const {
		return get_data();
	}
	T* end() {
		return get_data() + get_size();
	}
	const T* end() const {
		return get_data() + get_size();
	}
};

struct TreeBeginComp {
	template <class I> bool operator <(const I&) const {
		return true;
	}
};
struct TreeEndComp {
	template <class I> bool operator <(const I&) const {
		return false;
	}
};
inline TreeBeginComp tree_begin() {
	return TreeBeginComp();
}
inline TreeEndComp tree_end() {
	return TreeEndComp();
}

template <class I> class Tree {
	using T = typename I::T;
	struct Node {
		I info;
	};
	struct Leaf: Node {
		static constexpr std::size_t SIZE = 128;
		StaticVector<T, SIZE> children;
		Leaf* previous_leaf = nullptr;
		Leaf* next_leaf = nullptr;
	};
	struct INode: Node {
		static constexpr std::size_t SIZE = 16;
		StaticVector<Node*, SIZE> children;
	};

public:
	class Iterator {
		Leaf* leaf;
		std::size_t i;
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = const T*;
		using reference = const T&;
		using iterator_category = std::input_iterator_tag;
		Iterator(Leaf* leaf, std::size_t i): leaf(leaf), i(i) {}
		bool operator ==(const Iterator& rhs) const {
			return leaf == rhs.leaf && i == rhs.i;
		}
		bool operator !=(const Iterator& rhs) const {
			return !operator ==(rhs);
		}
		const T& operator *() const {
			return leaf->children[i];
		}
		Iterator& operator ++() {
			++i;
			if (i == leaf->children.get_size() && leaf->next_leaf) {
				leaf = leaf->next_leaf;
				i = 0;
			}
			return *this;
		}
	};
private:

	static I get_info(const T& child) {
		return I(child);
	}
	static I get_info(Node* child) {
		return child->info;
	}
	static std::size_t get_last_index(Leaf* node) {
		return node->children.get_size();
	}
	static std::size_t get_last_index(INode* node) {
		return node->children.get_size() - 1;
	}
	template <class N, class C> static std::size_t get_index(std::size_t depth, N* node, I& sum, C comp) {
		std::size_t i;
		for (i = 0; i < get_last_index(node); ++i) {
			const I next_sum = sum + get_info(node->children[i]);
			if (comp < next_sum) break;
			sum = next_sum;
		}
		return i;
	}
	template <class N> static void recompute_info(std::size_t depth, N* node) {
		node->info = I();
		for (auto& child: node->children) {
			node->info = node->info + get_info(child);
		}
	}

	// free
	static void free(std::size_t depth, Leaf* node) {
		if (node->previous_leaf) node->previous_leaf->next_leaf = node->next_leaf;
		if (node->next_leaf) node->next_leaf->previous_leaf = node->previous_leaf;
		delete node;
	}
	static void free(std::size_t depth, INode* node) {
		for (Node* child: node->children) {
			free(depth - 1, child);
		}
		delete node;
	}
	static void free(std::size_t depth, Node* node) {
		if (depth > 0)
			free(depth, static_cast<INode*>(node));
		else
			free(depth, static_cast<Leaf*>(node));
	}

	// get
	template <class C> static Iterator get(std::size_t depth, Leaf* node, I& sum, C comp) {
		const std::size_t i = get_index(depth, node, sum, comp);
		return Iterator(node, i);
	}
	template <class C> static Iterator get(std::size_t depth, INode* node, I& sum, C comp) {
		const std::size_t i = get_index(depth, node, sum, comp);
		return get(depth - 1, node->children[i], sum, comp);
	}
	template <class C> static Iterator get(std::size_t depth, Node* node, I& sum, C comp) {
		if (depth > 0)
			return get(depth, static_cast<INode*>(node), sum, comp);
		else
			return get(depth, static_cast<Leaf*>(node), sum, comp);
	}

	// insert
	template <class C> static Node* insert(std::size_t depth, Leaf* node, I sum, C comp, const T& t) {
		const std::size_t index = get_index(depth, node, sum, comp);
		node->children.insert(index, t);
		if (node->children.get_size() == Leaf::SIZE) {
			Leaf* next_node = new Leaf();
			next_node->previous_leaf = node;
			next_node->next_leaf = node->next_leaf;
			if (node->next_leaf) node->next_leaf->previous_leaf = next_node;
			node->next_leaf = next_node;
			node->children.balance_out(next_node->children, Leaf::SIZE/2);
			recompute_info(depth, node);
			recompute_info(depth, next_node);
			return next_node;
		}
		recompute_info(depth, node);
		return nullptr;
	}
	template <class C> static Node* insert(std::size_t depth, INode* node, I sum, C comp, const T& t) {
		std::size_t i = get_index(depth, node, sum, comp);
		Node* new_child = insert(depth - 1, node->children[i], sum, comp, t);
		if (new_child) {
			node->children.insert(i + 1, new_child);
			if (node->children.get_size() == INode::SIZE) {
				INode* next_node = new INode();
				node->children.balance_out(next_node->children, INode::SIZE/2);
				recompute_info(depth, node);
				recompute_info(depth, next_node);
				return next_node;
			}
		}
		recompute_info(depth, node);
		return nullptr;
	}
	template <class C> static Node* insert(std::size_t depth, Node* node, I sum, C comp, const T& t) {
		if (depth > 0)
			return insert(depth, static_cast<INode*>(node), sum, comp, t);
		else
			return insert(depth, static_cast<Leaf*>(node), sum, comp, t);
	}

	// balance
	template <class N> static bool balance(std::size_t depth, N* left, N* right) {
		if (left->children.get_size() + right->children.get_size() < N::SIZE) {
			left->children.balance_in(right->children, right->children.get_size());
			recompute_info(depth, left);
			//recompute_info(depth, right);
			return true;
		}
		else {
			if (left->children.get_size() < N::SIZE/2) {
				left->children.balance_in(right->children, N::SIZE/2 - left->children.get_size());
			}
			else {
				assert(right->children.get_size() < N::SIZE/2);
				left->children.balance_out(right->children, N::SIZE/2 - right->children.get_size());
			}
			assert(left->children.get_size() >= N::SIZE/2 && right->children.get_size() >= N::SIZE/2);
			recompute_info(depth, left);
			recompute_info(depth, right);
			return false;
		}
	}
	static bool balance(std::size_t depth, Node* left, Node* right) {
		if (depth > 0)
			return balance(depth, static_cast<INode*>(left), static_cast<INode*>(right));
		else
			return balance(depth, static_cast<Leaf*>(left), static_cast<Leaf*>(right));
	}

	// append
	template <class Iter> static Node* append(std::size_t depth, Leaf* node, Iter& first, Iter last) {
		while (node->children.get_size() < Leaf::SIZE && first != last) {
			node->children.insert(*first);
			++first;
		}
		if (node->children.get_size() == Leaf::SIZE) {
			Leaf* next_node = new Leaf();
			next_node->previous_leaf = node;
			next_node->next_leaf = node->next_leaf;
			if (node->next_leaf) node->next_leaf->previous_leaf = next_node;
			node->next_leaf = next_node;
			node->children.balance_out(next_node->children, 1);
			while (next_node->children.get_size() < Leaf::SIZE - 1 && first != last) {
				next_node->children.insert(*first);
				++first;
			}
			if (next_node->children.get_size() < Leaf::SIZE/2) {
				balance(depth, node, next_node);
			}
			recompute_info(depth, node);
			recompute_info(depth, next_node);
			return next_node;
		}
		recompute_info(depth, node);
		return nullptr;
	}
	template <class Iter> static Node* append(std::size_t depth, INode* node, Iter& first, Iter last) {
		while (node->children.get_size() < INode::SIZE && first != last) {
			Node* new_child = append(depth - 1, node->children.get(), first, last);
			if (new_child) node->children.insert(new_child);
		}
		if (node->children.get_size() == INode::SIZE) {
			INode* next_node = new INode();
			node->children.balance_out(next_node->children, 1);
			while (next_node->children.get_size() < INode::SIZE - 1 && first != last) {
				Node* new_child = append(depth - 1, next_node->children.get(), first, last);
				if (new_child) next_node->children.insert(new_child);
			}
			if (next_node->children.get_size() < INode::SIZE/2) {
				balance(depth, node, next_node);
			}
			recompute_info(depth, node);
			recompute_info(depth, next_node);
			return next_node;
		}
		recompute_info(depth, node);
		return nullptr;
	}
	template <class Iter> static Node* append(std::size_t depth, Node* node, Iter& first, Iter last) {
		if (depth > 0)
			return append(depth, static_cast<INode*>(node), first, last);
		else
			return append(depth, static_cast<Leaf*>(node), first, last);
	}

	// remove
	template <class C> static bool remove(std::size_t depth, Leaf* node, I sum, C comp) {
		const std::size_t i = get_index(depth, node, sum, comp);
		node->children.remove(i);
		recompute_info(depth, node);
		return node->children.get_size() < Leaf::SIZE/2;
	}
	template <class C> static bool remove(std::size_t depth, INode* node, I sum, C comp) {
		std::size_t i = get_index(depth, node, sum, comp);
		if (remove(depth - 1, node->children[i], sum, comp)) {
			if (i == 0) ++i;
			assert(i < node->children.get_size());
			if (balance(depth - 1, node->children[i - 1], node->children[i])) {
				free(depth - 1, node->children[i]);
				node->children.remove(i);
			}
		}
		recompute_info(depth, node);
		return node->children.get_size() < INode::SIZE/2;
	}
	template <class C> static bool remove(std::size_t depth, Node* node, I sum, C comp) {
		if (depth > 0)
			return remove(depth, static_cast<INode*>(node), sum, comp);
		else
			return remove(depth, static_cast<Leaf*>(node), sum, comp);
	}

	std::size_t depth;
	Node* root;
public:
	Tree(): depth(0), root(new Leaf()) {}
	~Tree() {
		free(depth, root);
	}
	I get_info() const {
		return root->info;
	}
	template <class C> Iterator get(C comp) const {
		I sum;
		return get(depth, root, sum, comp);
	}
	template <class C> I get_sum(C comp) const {
		if (!(comp < get_info())) {
			return get_info();
		}
		I sum;
		get(depth, root, sum, comp);
		return sum;
	}
	template <class C> void insert(C comp, const T& t) {
		Node* new_child = insert(depth, root, I(), comp, t);
		if (new_child) {
			++depth;
			INode* new_root = new INode();
			new_root->children.insert(root);
			new_root->children.insert(new_child);
			recompute_info(depth, new_root);
			root = new_root;
		}
	}
	void append(const T& t) {
		insert(tree_end(), t);
	}
	template <class Iter> void append(Iter first, Iter last) {
		while (first != last) {
			Node* new_child = append(depth, root, first, last);
			if (new_child) {
				++depth;
				INode* new_root = new INode();
				new_root->children.insert(root);
				new_root->children.insert(new_child);
				recompute_info(depth, new_root);
				root = new_root;
			}
		}
	}
	template <class C> void remove(C comp) {
		remove(depth, root, I(), comp);
		if (depth > 0) {
			INode* node = static_cast<INode*>(root);
			if (node->children.get_size() == 1) {
				root = node->children[0];
				delete node;
				--depth;
			}
		}
	}
	Iterator begin() const {
		return get(tree_begin());
	}
	Iterator end() const {
		return get(tree_end());
	}
};
