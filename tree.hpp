#pragma once

#include <cstddef>
#include <utility>
#include <type_traits>
#include <new>
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
	void split(StaticVector& v) {
		// split from this into v
		assert(size == N && v.size == 0 && N % 2 == 0);
		for (std::size_t i = 0; i < N/2; ++i) {
			new (v.data + i) T(std::move(get(i + N/2)));
			get(i + N/2).~T();
		}
		size = N/2;
		v.size = N/2;
	}
	void merge(StaticVector& v) {
		// merge v into this
		assert(size + v.size < N);
		for (std::size_t i = 0; i < v.size; ++i) {
			new (data + size + i) T(std::move(v.get(i)));
			v.get(i).~T();
		}
		size = size + v.size;
		v.size = 0;
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
	};
	struct INode: Node {
		static constexpr std::size_t SIZE = 16;
		StaticVector<Node*, SIZE> children;
	};

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
	template <class C> static T get(std::size_t depth, Leaf* node, I& sum, C comp) {
		const std::size_t i = get_index(depth, node, sum, comp);
		return node->children[i];
	}
	template <class C> static T get(std::size_t depth, INode* node, I& sum, C comp) {
		const std::size_t i = get_index(depth, node, sum, comp);
		return get(depth - 1, node->children[i], sum, comp);
	}
	template <class C> static T get(std::size_t depth, Node* node, I& sum, C comp) {
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
			node->children.split(next_node->children);
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
				node->children.split(next_node->children);
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
			left->children.merge(right->children);
			recompute_info(depth, left);
			//recompute_info(depth, right);
			return true;
		}
		else {
			if (left->children.get_size() < N::SIZE/2) {
				left->children.insert(right->children.get(0));
				right->children.remove(0);
			}
			else {
				assert(right->children.get_size() < N::SIZE/2);
				right->children.insert(0, left->children.get());
				left->children.remove();
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
	template <class C> T get(C comp) const {
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
};
